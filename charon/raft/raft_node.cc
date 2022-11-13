#include <vector>
#include <tinyxml/tinyxml.h>
#include <string>
#include <atomic>
#include <algorithm>
#include "tinyrpc/comm/log.h"
#include "tinyrpc/comm/start.h"
#include "tinyrpc/net/tinypb/tinypb_rpc_async_channel.h"
#include "tinyrpc/net/tinypb/tinypb_rpc_controller.h"
#include "tinyrpc/net/tinypb/tinypb_rpc_closure.h"
#include "tinyrpc/net/tcp/io_thread.h"
#include "tinyrpc/net/mutex.h"
#include "tinyrpc/net/timer.h"
#include "tinyrpc/coroutine/coroutine_pool.h"

#include "charon/raft/raft_node.h"
#include "charon/pb/charon.pb.h"
#include "charon/comm/util.h"


namespace charon {

#define CALL_RAFT_RPCS(REQUEST, RESPONSE, METHOD)                                                                 \
  tinyrpc::Coroutine* cur_cor = tinyrpc::Coroutine::GetCurrentCoroutine();                                        \
  std::atomic_int res_nodes {1};                                                                                  \
  std::atomic_bool need_resume {true};                                                                            \
  if ((int)(rpc_list.size()) + 1 != m_node_count) {                                                               \
    AppErrorLog << "CALL_RAFT_RPCS Error, rpc_list size is not all nodes count - 1";                              \
    return;                                                                                                       \
  }                                                                                                               \
  for (size_t i = 0; i < rpc_list.size(); ++i) {                                                                  \
    std::shared_ptr<REQUEST> request = rpc_list[i].first;                                                         \
    std::shared_ptr<RESPONSE> response = rpc_list[i].second;                                                      \
    int id = request->peer_node_id();                                                                                  \
    printf("id=%d, addr is %s \n", id, m_nodes[id]["addr"].c_str());                                                                 \
    tinyrpc::IPAddress::ptr addr = std::make_shared<tinyrpc::IPAddress>(m_nodes[id]["addr"]);                     \
    tinyrpc::TinyPbRpcAsyncChannel::ptr rpc_channel =                                                             \
      std::make_shared<tinyrpc::TinyPbRpcAsyncChannel>(addr);                                                     \
    tinyrpc::TinyPbRpcController::ptr rpc_controller = std::make_shared<tinyrpc::TinyPbRpcController>();          \
    tinyrpc::TinyPbRpcClosure::ptr closure = std::make_shared<tinyrpc::TinyPbRpcClosure>(                         \
      [&res_nodes, cur_cor, &need_resume, this]() mutable {                                                       \
        res_nodes++;                                                                                              \
        if (res_nodes >= getNodeCount()) {                                                                        \
          if (need_resume.exchange(false)) {                                                                      \
            tinyrpc::Coroutine::Resume(cur_cor);                                                                  \
          }                                                                                                       \
        }                                                                                                         \
      }                                                                                                           \
    );                                                                                                            \
    rpc_controller->SetTimeout(2000);                                                                             \
    rpc_channel->saveCallee(rpc_controller, request, response, closure);                                          \
    RaftService_Stub stub(rpc_channel.get());                                                                     \
    AppInfoLog << #METHOD <<" request to raft node["<< m_nodes[id]["addr"] << ", " << m_nodes[id]["name"] << "]"; \
    stub.METHOD(rpc_controller.get(), request.get(), response.get(), closure.get());                              \
  }                                                                                                               \
  tinyrpc::Coroutine::Yield();                                                                                    \



static RaftNodeContainer* g_raft_container = NULL;

RaftNode::RaftNode() {
  // log at index 0 is not use
  m_logs.emplace_back(LogEntry());
  m_nodes.resize(100);
  m_match_indexs.push_back(-1);
  m_next_indexs.push_back(-1);

  m_current_term = 0;
  m_voted_for_id = 0;
  m_state = RAFT_FOLLOWER_STATE;
  std::string local_addr = tinyrpc::GetServer()->getLocalAddr()->toString();

  TiXmlElement* node =  tinyrpc::GetConfig()->getXmlNode("raft");
  assert(node != NULL);
  TiXmlElement* groups_node = node->FirstChildElement("raft_servers"); 
  int id = 1;
  for (TiXmlElement* xmlnode = groups_node->FirstChildElement(); xmlnode!= NULL; xmlnode = xmlnode->NextSiblingElement()) {
    KVMap raft_node;
    std::string addr = std::string(xmlnode->Attribute("addr"));
    raft_node["addr"] = addr;

    std::string node_name = std::to_string(id) + "-" + std::to_string(tinyrpc::IOThread::GetCurrentIOThread()->getThreadIndex() + 1);
    raft_node["name"] =  node_name;
    raft_node["id"] = id;

    InfoLog << "read raft server conf[" << addr << ", " << node_name << "]";

    if (addr == local_addr) {
      m_node_id = id;
      m_node_addr = addr;
      m_node_name = node_name;
      InfoLog << "my local raft node conf[" << addr << ", " << node_name << "]";
    }
    m_nodes[id].swap(raft_node);
    m_match_indexs.push_back(0);
    m_next_indexs.push_back(0);
    id++;
    m_node_count++;
  }
  TiXmlElement* conf_node = node->FirstChildElement("raft_conf");
  assert(conf_node);
  m_elect_overtime = std::atoi(conf_node->FirstChildElement("elect_timeout")->GetText());
  m_heart_interval = std::atoi(conf_node->FirstChildElement("heart_interval")->GetText());

}

RaftNode::~RaftNode() {

}

void RaftNode::handleAskVote(const AskVoteRequest& request, AskVoteResponse& response) {
  response.set_term(m_current_term);
  response.set_accept_result(ACCEPT_FAIL);
  response.set_vote_fail_code(0);
  response.set_node_id(m_node_id);
  response.set_node_name(m_node_name);
  response.set_node_addr(m_node_addr);
  response.set_state(m_state);

  if (request.candidate_term() < m_current_term) {
    response.set_accept_result(ACCEPT_FAIL);
    response.set_vote_fail_reason(formatString("AskVote failed, your's term[%d] is less than my term[%d]", 
      request.candidate_term(), m_current_term));
    response.set_vote_fail_code(ERR_TERM_MORE_THAN_CANDICATE);
    return;
  }

  if (request.candidate_term() > m_current_term) {
    AppInfoLog << formatString("[Term: %d, state: %s, addr: %s, name: %s] receive high term AskVote request, now to incrase term, change to [term: %d, state: %s]",
      m_current_term, StateToString(m_state).c_str(), m_node_addr.c_str(), m_node_name.c_str(),
      request.candidate_term(), StateToString(RAFT_FOLLOWER_STATE).c_str());

    toFollower(request.candidate_term());
    resetElectionTimer();
  }

  bool is_vote = false;
  if (m_voted_for_id == 0) {
    LogEntry last_log = m_logs[m_logs.size() - 1];
    if (request.last_log_term() > last_log.term()) {
      is_vote = true;
      AppInfoLog << formatString("AskVote req's last_log_term[%d] > current last_log_term[%d], vote to it",
        request.last_log_term(), last_log.term());

    } else if (request.last_log_term() == last_log.term() && request.last_log_index() >= last_log.index()) {
      AppInfoLog << formatString("AskVote req's last_log_index[%d] >= current last_log_index[%d], vote to it",
        request.last_log_index(), last_log.index());
      is_vote = true;

    } else {
      response.set_accept_result(ACCEPT_FAIL);
      response.set_vote_fail_code(ERR_LOG_MORE_THAN_CANDIDATE);
      std::string reason = formatString("current log[term: %d, index: %d] is new than yours log[term: %d, index: %d]", 
        last_log.term(), last_log.index(), request.last_log_term(), request.last_log_index());
      AppInfoLog << reason;
      response.set_vote_fail_reason(reason);

      return;
    }

  } else if (m_voted_for_id == request.candidate_id()) {
    is_vote = true;
  } else if (m_voted_for_id == m_node_id) {
    std::string reason = formatString("[Term: %d, state: %s, addr: %s, name: %s] has already vote to self",
      m_current_term, StateToString(m_state).c_str(), m_node_addr.c_str(), m_node_name.c_str());

    AppInfoLog << reason;
    response.set_accept_result(ACCEPT_FAIL);
    response.set_vote_fail_code(ERR_LOG_MORE_THAN_CANDIDATE);
    response.set_vote_fail_reason(reason);
  } else {
    // exception case
  }
  if (is_vote) {
    m_voted_for_id = request.node_id();
    response.set_accept_result(ACCEPT_SUCC);
    m_state = RAFT_CANDIDATE_STATE;
    resetElectionTimer();
  }
}

void RaftNode::handleAppendLogEntries(const AppendLogEntriesRequest& request, AppendLogEntriesResponse& response) {
  response.set_term(m_current_term);
  response.set_accept_result(ACCEPT_FAIL);
  response.set_term(m_current_term);
  response.set_append_fail_code(0);
  response.set_node_id(m_node_id);
  response.set_node_name(m_node_name);
  response.set_node_addr(m_node_addr);
  response.set_state(m_state);

  if (request.leader_term() < m_current_term) {
    // return false when leader's term less than current Node's term
    response.set_accept_result(ACCEPT_FAIL);
    response.set_append_fail_code(ERR_TERM_MORE_THAN_LEADER);
    response.set_append_fail_reason(formatString("AppendLogEntries failed, leader's term[%d] is less than current term[%d]",
      request.leader_term(), m_current_term));

    return;
  }

  if (request.leader_term() > m_current_term) {
    AppErrorLog << formatString("[Term: %d, state: %s, addr: %s, name: %s] receive high term AppendLogEntriesRequest request, now to incrase term, change to [term: %d, state: %s]",
      m_current_term, StateToString(m_state).c_str(), m_node_addr.c_str(), m_node_name.c_str(),
      request.leader_term(), StateToString(RAFT_FOLLOWER_STATE).c_str());

    toFollower(request.leader_term());
  }
  resetElectionTimer();

  int prev_log_index = request.prev_log_index();
  if ((prev_log_index >= (int)m_logs.size())
      || (m_logs[prev_log_index].term() != request.prev_log_term())
      || (m_logs[prev_log_index].index() != request.prev_log_index())) {

    // return false when Node can't find log match(prev_log_index, prev_log_term) 
    response.set_accept_result(ACCEPT_FAIL);
    response.set_append_fail_code(ERR_NOT_MATCH_PREINDEX);
    response.set_append_fail_reason(
      formatString("AppendLogEntries failed, Node can't match prev_log_index[%d], prev_log_term[%d]",
        prev_log_index, request.prev_log_term()));

    return;
  }

  if (request.log_entries_size() != 0) {
    std::vector<LogEntry> tmp(m_logs.begin(), m_logs.begin() + prev_log_index + 1);
    
    for (int i = 0; i < request.log_entries_size(); ++i) {
      tmp.push_back(request.log_entries().Get(i));
    }
    m_logs.swap(tmp);
  }

  if (request.leader_commit_index() > m_commit_index) {
    m_commit_index = std::min(request.leader_commit_index(), m_logs[m_logs.size() - 1].index());
  }

  response.set_accept_result(ACCEPT_SUCC);

}


int RaftNode::applyToStateMachine(const LogEntry& logs) {
  return 0;
}

int RaftNode::appendLogEntries() {
  if (m_state != RAFT_LEADER_STATE) {
    AppErrorLog << formatString("[Term: %d][%s - %d - %s] current raft node state isn't LEADER, can't appendLogEntries",
      m_current_term, m_node_addr.c_str(), m_node_id, m_node_name.c_str());
    return -1;
  }
  std::vector<std::pair<std::shared_ptr<AppendLogEntriesRequest>, std::shared_ptr<AppendLogEntriesResponse>>> rpc_list;
  for (int i = 1; i <= m_node_count; ++i) {
    if ((int)i == m_node_id) {
      continue;
    }
    std::shared_ptr<AppendLogEntriesRequest> request = std::make_shared<AppendLogEntriesRequest>();
    std::shared_ptr<AppendLogEntriesResponse> response = std::make_shared<AppendLogEntriesResponse>();

    request->set_leader_term(m_current_term);
    request->set_leader_id(m_node_id);

    int last_index = m_logs.size() - 1;
 
    request->set_peer_node_id(i);
    request->set_leader_commit_index(m_commit_index);
    request->set_node_id(m_node_id);
    request->set_node_addr(m_node_addr);
    request->set_node_name(m_node_name);

    if (m_logs[last_index].term() != m_current_term) {
      // if the last log is't current term, can't directlt commit
      // add a empty_log at current term
      last_index++;
      LogEntry empty_log;
      empty_log.set_term(m_current_term);
      empty_log.set_index(last_index);
      empty_log.set_cmd("");
      m_logs.push_back(std::move(empty_log));
    }

    int s = m_next_indexs[i];
    if (s >= 1 && s < (int)m_logs.size()) {
      if (s > 1) {
        request->set_prev_log_index(m_logs[s - 1].index());
        request->set_prev_log_term(m_logs[s - 1].term());
      } else {
        request->set_prev_log_index(0);
        request->set_prev_log_term(0);
      }
      request->mutable_log_entries()->CopyFrom({m_logs.begin() + s, m_logs.end()});
      request->set_last_log_index(m_logs[last_index].index());
    }
    rpc_list.emplace_back(std::pair<std::shared_ptr<AppendLogEntriesRequest>, std::shared_ptr<AppendLogEntriesResponse>>(request, response));
  }

  int rt = AppendLogEntriesRPCs(rpc_list);

  lock();
  std::vector<int> tmp(m_match_indexs.begin() + 1, m_match_indexs.end());
  std::sort(tmp.begin(), tmp.end());
  int t = tmp[getMostNodeCount() - 1];
  if (t > m_commit_index) {
    m_commit_index = t;
  }
  unlock();

  if (rt != 0) {
    AppInfoLog << "appendLogEntries failed";
    return -1;
  }

  AppInfoLog << "appendLogEntries succ";
  return 0;
}

void RaftNode::toFollower(int term) {
  m_coroutine_mutex.lock();
  AppErrorLog << formatString("[Term: %d, state: %s, addr: %s, name: %s] raft node become to follewer [Term: %d, state: %s]",
    m_current_term, m_node_addr.c_str(), m_node_id, m_node_name.c_str(), term, StateToString(RAFT_FOLLOWER_STATE).c_str());
  m_state = RAFT_FOLLOWER_STATE;
  m_current_term = term;
  m_voted_for_id = 0;

  m_coroutine_mutex.unlock();

  stopAppendLogHeart();
  resetElectionTimer();
}

int RaftNode::AskVoteRPCs(std::vector<std::pair<std::shared_ptr<AskVoteRequest>, std::shared_ptr<AskVoteResponse>>>& rpc_list) {
  tinyrpc::Coroutine* cur_cor = tinyrpc::Coroutine::GetCurrentCoroutine();
  std::atomic_int res_nodes {1};
  std::atomic_int succ_count {1};
  std::atomic_bool need_resume {true};

  if ((int)(rpc_list.size()) + 1 != m_node_count) {
    AppErrorLog << "AskVoteRPCs Error, rpc_list size is not all nodes count - 1";
    // return;
  }
  for (size_t i = 0; i < rpc_list.size(); ++i) {
    std::shared_ptr<AskVoteRequest> request = rpc_list[i].first;
    std::shared_ptr<AskVoteResponse> response = rpc_list[i].second;
    int id = request->peer_node_id();
    printf("id=%d, addr is %s \n", id, m_nodes[id]["addr"].c_str());
    tinyrpc::IPAddress::ptr addr = std::make_shared<tinyrpc::IPAddress>(m_nodes[id]["addr"]);
    tinyrpc::TinyPbRpcAsyncChannel::ptr rpc_channel =
      std::make_shared<tinyrpc::TinyPbRpcAsyncChannel>(addr);
    tinyrpc::TinyPbRpcController::ptr rpc_controller = std::make_shared<tinyrpc::TinyPbRpcController>();
    tinyrpc::TinyPbRpcClosure::ptr closure = std::make_shared<tinyrpc::TinyPbRpcClosure>(
      [&res_nodes, cur_cor, &need_resume, this, request, response, &succ_count]() mutable {
        res_nodes++;

        this->lock();
        if (this->getState() == RAFT_FOLLOWER_STATE) {
          // give up current election
          // donothing
          this->unlock();
          return;
        }
        if (response->accept_result() == ACCEPT_SUCC) {
          succ_count++;
          AppInfoLog << formatString("AskVoteRPCs [%s name: %s, term: %d, state: %s] succ get vote from node[%s name: %s, term: %d, state: %s]",
            m_node_addr.c_str(), m_node_name.c_str(), m_current_term, StateToString(m_state).c_str(), response->node_addr().c_str(), 
            response->node_name().c_str(), response->term(), StateToString(response->state()).c_str());

        } else {
          AppErrorLog << formatString("AskVoteRPCs [%s name: %s, term: %d, state: %s] failed get vote from node[%s name: %s, term: %s, state: %s], faild reason[%d, %s]",
            m_node_addr.c_str(), m_node_name.c_str(), m_current_term, StateToString(m_state).c_str(),
            response->node_addr().c_str(), response->node_name().c_str(), response->term(), StateToString(response->state()).c_str(),
            response->vote_fail_code(), response->vote_fail_reason().c_str());

          if (response->vote_fail_code() == ERR_TERM_MORE_THAN_LEADER && m_current_term < response->term()) {
            // TODO: become to follewer
            toFollower(response->term());
            return;
          }
          
        }
        this->unlock();
        if (succ_count >= this->getMostNodeCount() && need_resume.exchange(false)) {
            tinyrpc::Coroutine::Resume(cur_cor);
          }
        }
    );
    rpc_controller->SetTimeout(2000);
    rpc_channel->saveCallee(rpc_controller, request, response, closure);
    CharonService_Stub stub(rpc_channel.get());
    AppInfoLog << "AskVote request to raft node["<< m_nodes[id]["addr"] << ", " << m_nodes[id]["name"] << "]";
    stub.AskVote(rpc_controller.get(), request.get(), response.get(), closure.get());
  }
  tinyrpc::Coroutine::Yield();
  if (succ_count >= this->getMostNodeCount()) {
    return 0;
  }
  return -1;
}

int RaftNode::AppendLogEntriesRPCs(std::vector<std::pair<std::shared_ptr<AppendLogEntriesRequest>, std::shared_ptr<AppendLogEntriesResponse>>>& rpc_list) {
  tinyrpc::Coroutine* cur_cor = tinyrpc::Coroutine::GetCurrentCoroutine();

  std::atomic_int res_nodes {1};
  std::atomic_int succ_count {1};
  std::atomic_bool need_resume {true};

  if ((int)(rpc_list.size()) + 1 != m_node_count) {
    AppErrorLog << "AppendLogEntries Error, rpc_list size is not all nodes count - 1";
    // return -1;
  }
  for (size_t i = 0; i < rpc_list.size(); ++i) {
    std::shared_ptr<AppendLogEntriesRequest> request = rpc_list[i].first;
    std::shared_ptr<AppendLogEntriesResponse> response = rpc_list[i].second;
    int id = request->peer_node_id();
    printf("id=%d, addr is %s \n", id, m_nodes[id]["addr"].c_str());
    tinyrpc::IPAddress::ptr addr = std::make_shared<tinyrpc::IPAddress>(m_nodes[id]["addr"]);
    tinyrpc::TinyPbRpcAsyncChannel::ptr rpc_channel =
      std::make_shared<tinyrpc::TinyPbRpcAsyncChannel>(addr);
    tinyrpc::TinyPbRpcController::ptr rpc_controller = std::make_shared<tinyrpc::TinyPbRpcController>();
    tinyrpc::TinyPbRpcClosure::ptr closure = std::make_shared<tinyrpc::TinyPbRpcClosure>(
      [&res_nodes, cur_cor, &need_resume, this, request, response, &succ_count]() mutable {
        res_nodes++;

        this->lock();
        if (this->getState() == RAFT_FOLLOWER_STATE) {
          // give up current election
          // donothing
          this->unlock();
          return;
        }
        int match_index = request->last_log_index();
        if (response->accept_result() == ACCEPT_SUCC) {
          succ_count++;
          AppInfoLog << formatString("AppendLogEntries [%s name: %s, term: %d, state: %s] succ apply log to node[%s name: %s, term: %d, state: %s]",
            m_node_addr.c_str(), m_node_name.c_str(), m_current_term, StateToString(m_state).c_str(), response->node_addr().c_str(), 
            response->node_name().c_str(), response->term(), StateToString(response->state()).c_str());

          updateMatchIndex(request->node_id(), match_index);
          updateNextIndex(request->node_id(), match_index + 1);

        } else {
          AppErrorLog << formatString("AppendLogEntries [%s name: %s, term: %d, state: %s] failed get apply log to node[%s name: %s, term: %s, state: %s], faild reason[%d, %s]",
            m_node_addr.c_str(), m_node_name.c_str(), m_current_term, StateToString(m_state).c_str(),
            response->node_addr().c_str(), response->node_name().c_str(), response->term(), StateToString(response->state()).c_str(),
            response->append_fail_code(), response->append_fail_reason().c_str());

          if (response->append_fail_code() == ERR_NOT_MATCH_PREINDEX) {
            updateNextIndex(request->node_id(), response->need_index());
          } else if (response->append_fail_code() == ERR_TERM_MORE_THAN_LEADER && m_current_term < response->term()) {
            toFollower(response->term());
            return;
          }
        }

        this->unlock();
        if (succ_count >= this->getMostNodeCount() && need_resume.exchange(false)) {
            tinyrpc::Coroutine::Resume(cur_cor);
          }
        }
    );
    rpc_controller->SetTimeout(2000);
    rpc_channel->saveCallee(rpc_controller, request, response, closure);
    CharonService_Stub stub(rpc_channel.get());
    AppInfoLog << "AppendLogEntries request to raft node["<< m_nodes[id]["addr"] << ", " << m_nodes[id]["name"] << "]";
    stub.AppendLogEntries(rpc_controller.get(), request.get(), response.get(), closure.get());
  }
  tinyrpc::Coroutine::Yield();
  if (succ_count >= this->getMostNodeCount()) {
    return 0;
  }
  return -1;

}

int RaftNode::getNodeCount() {
  return m_node_count;
}

// get more than half nodes count
int RaftNode::getMostNodeCount() {
  return (m_node_count)/2 + 1;
}


RaftNodeState RaftNode::getState() {
  return m_state;
}

void RaftNode::setState(RaftNodeState state) {
  m_state = state;
}


void RaftNode::resetElectionTimer() {
  if (!m_election_event) {
    tinyrpc::Coroutine::ptr cor = tinyrpc::GetCoroutinePool()->getCoroutineInstanse();
    cor->setCallBack(
      [this]() {
          election();
      }
    );
    m_election_event = 
      std::make_shared<tinyrpc::TimerEvent>(m_elect_overtime, false, [cor]() mutable {
        auto t = cor.get();
        cor.reset();
        tinyrpc::Coroutine::Resume(t);
      });

    tinyrpc::Reactor::GetReactor()->getTimer()->addTimerEvent(m_election_event);
  } else {
    m_election_event->cancle();
    m_election_event->resetTime();
  }
  
}

void RaftNode::startAppendLogHeart() {
  if (!m_appendlog_event) {
    tinyrpc::Coroutine::ptr cor = tinyrpc::GetCoroutinePool()->getCoroutineInstanse();
    cor->setCallBack(
      [this]() {
        appendLogEntries();
      }
    );

    m_appendlog_event = 
      std::make_shared<tinyrpc::TimerEvent>(m_heart_interval, true, [cor]() mutable {
        auto t = cor.get();
        cor.reset();
        tinyrpc::Coroutine::Resume(t);
      });

    tinyrpc::Reactor::GetReactor()->getTimer()->addTimerEvent(m_appendlog_event);
  }
  m_appendlog_event->wake();
}

void RaftNode::stopAppendLogHeart() {
  if (m_appendlog_event) {
    m_appendlog_event->cancle();
  }
}

void RaftNode::election() {

  m_coroutine_mutex.lock();
  m_state = RAFT_CANDIDATE_STATE;
  // first add current term
  m_current_term++;
  // give vote to self
  m_voted_for_id = m_node_id;

  RaftNode* tmp = this;
  m_coroutine_mutex.unlock();

  std::vector<std::pair<std::shared_ptr<AskVoteRequest>, std::shared_ptr<AskVoteResponse>>> rpc_list;
  for (int i = 1; i <= m_node_count; ++i) {
    if ((int)i == m_node_id) {
      continue;
    }
    std::shared_ptr<AskVoteRequest> request = std::make_shared<AskVoteRequest>();
    std::shared_ptr<AskVoteResponse> response = std::make_shared<AskVoteResponse>();

    request->set_candidate_id(tmp->m_node_id);
    request->set_candidate_term(tmp->m_current_term);
    request->set_last_log_index(tmp->m_logs.size() - 1);
    request->set_last_log_term(tmp->m_logs[tmp->m_logs.size() - 1].term());
    request->set_node_id(tmp->m_node_id);
    request->set_node_addr(tmp->m_node_addr);
    request->set_node_name(tmp->m_node_name);
    request->set_peer_node_id(i);

    rpc_list.emplace_back(std::pair<std::shared_ptr<AskVoteRequest>, std::shared_ptr<AskVoteResponse>>(request, response));
  }
  int rt = AskVoteRPCs(rpc_list);

  if (rt != 0) {
    // TODO:
    AppInfoLog << formatString("[Term: %d, state: %s, addr: %s, name: %s] failed get most node vote",
      m_current_term, StateToString(m_state).c_str(), m_node_addr.c_str(), m_node_name.c_str());
    return;
  }

  m_coroutine_mutex.lock();
  m_state = RAFT_LEADER_STATE;
  AppInfoLog << formatString("[Term: %d, state: %s, addr: %s, name: %s] succ get most node vote, elected to leader node",
    m_current_term, StateToString(m_state).c_str(), m_node_addr.c_str(), m_node_name.c_str());
  m_coroutine_mutex.unlock();

  startAppendLogHeart();
}


int RaftNode::execute(const std::string& cmd) {
  m_coroutine_mutex.lock();
  if (m_state != RAFT_LEADER_STATE) {
    // redirect to leader
  }
  LogEntry log;
  int last_index = m_logs.size() - 1;
  last_index++;
  log.set_term(m_current_term);
  log.set_index(last_index);
  log.set_cmd(cmd);
  m_logs.push_back(std::move(log));
  m_match_indexs[m_node_id] = log.index();

  int rt = appendLogEntries();
  if (rt != 0) {
    AppErrorLog << formatString("appendLogEntries log[%s] failed", LogEntryToString(log));
  }

  AppInfoLog << formatString("appendLogEntries log[%s] succ", LogEntryToString(log));
  m_coroutine_mutex.unlock();
  return rt;
}

void RaftNode::updateNextIndex(const int& node_id, const int& v) {
  if (node_id >= 1 && node_id < (int)m_next_indexs.size()) {
    m_next_indexs[node_id] = v;
  }
}


void RaftNode::updateMatchIndex(const int& node_id, const int& v) {
  if (node_id >= 1 && node_id < (int)m_match_indexs.size()) {
    m_match_indexs[node_id] = v;
  }
}

void RaftNode::lock() {
  m_coroutine_mutex.lock();
}

void RaftNode::unlock() {
  m_coroutine_mutex.unlock();
} 



std::string StateToString(const RaftNodeState& state) {
  if (state == RAFT_LEADER_STATE) {
    return "Leader";
  }
  if (state == RAFT_FOLLOWER_STATE) {
    return "Follower";
  }
  if (state == RAFT_CANDIDATE_STATE) {
    return "Candidate";
  }
  return "";
}

std::string LogEntryToString(const LogEntry& log) {
  return formatString("[term: %d, index: %d, cmd: %s]", log.term(), log.index(), log.cmd().c_str());
}



RaftNodeContainer::RaftNodeContainer() {
  m_size = 1;
  for (int i = 0; i < m_size; ++i) {
    m_container.push_back(new RaftNode());
  }
}

RaftNodeContainer::~RaftNodeContainer() {

}

RaftNode* RaftNodeContainer::getRaftNode(int hash_id) {
  return m_container[0];
}

RaftNodeContainer* RaftNodeContainer::GetRaftNodeContainer() {
  if (g_raft_container) {
    return g_raft_container;
  }
  g_raft_container = new RaftNodeContainer();
  return g_raft_container;
}

}


