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

#include "charon/raft/raft_node.h"
#include "charon/pb/raft.pb.h"
#include "charon/comm/util.h"


#define CALL_RAFT_RPCS(REQUEST, RESPONSE, METHOD)                                                                 \
  tinyrpc::Coroutine* cur_cor = tinyrpc::Coroutine::GetCurrentCoroutine();                                        \
  std::atomic_int res_nodes {1};                                                                                  \
  std::atomic_bool need_resume {true};                                                                            \
  if (rpc_list.size() + 1 != m_nodes.size() - 1) {                                                                \
    AppErrorLog << "CALL_RAFT_RPCS Error, rpc_list size is not all nodes count - 1";                              \
    return;                                                                                                       \
  }                                                                                                               \
  for (size_t i = 0; i < rpc_list.size(); ++i) {                                                                  \
    std::shared_ptr<REQUEST> request = rpc_list[i].first;                                                         \
    std::shared_ptr<RESPONSE> response = rpc_list[i].second;                                                      \
    tinyrpc::TinyPbRpcAsyncChannel::ptr rpc_channel =                                                             \
      std::make_shared<tinyrpc::TinyPbRpcAsyncChannel>(std::make_shared<tinyrpc::IPAddress>(m_nodes[i]["addr"])); \
    tinyrpc::TinyPbRpcController::ptr rpc_controller = std::make_shared<tinyrpc::TinyPbRpcController>();          \
    tinyrpc::TinyPbRpcClosure::ptr closure = std::make_shared<tinyrpc::TinyPbRpcClosure>(                         \
      [&res_nodes, cur_cor, &need_resume]() mutable {                                                             \
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
    AppInfoLog << #METHOD << " request to raft node[" << m_nodes[i]["addr"] << ", " << m_nodes[i]["name"] << "]"; \
    stub.METHOD(rpc_controller.get(), request.get(), response.get(), closure.get());                              \
  }                                                                                                               \
  tinyrpc::Coroutine::Yield();                                                                                    \


namespace charon {

static thread_local RaftNode* t_raft_node = NULL;

RaftNode* RaftNode::GetRaftNode() {
  if (t_raft_node) {
    return t_raft_node;
  }
  t_raft_node = new RaftNode();
  return t_raft_node;
}

RaftNode::RaftNode() {
  // log at index 0 is not use
  m_logs.emplace_back(LogEntry());
  // m_nodes.resize(100);
  m_nodes.push_back(-1);
  m_match_indexs.push_back(-1);
  m_next_indexs.push_back(-1);

  m_current_term = 0;
  m_state = RAFT_FOLLOWER_STATE;
  std::string local_addr = tinyrpc::GetServer()->getLocalAddr()->toString();

  TiXmlElement* node =  tinyrpc::GetConfig()->getXmlNode("raft");
  assert(node != NULL);
  TiXmlElement* groups_node = node->FirstChildElement("rafe_servers"); 
  int id = 1;
  for (TiXmlElement* xmlnode = groups_node->FirstChildElement(); xmlnode!= NULL; xmlnode = xmlnode->NextSiblingElement()) {
    KVMap raft_node;
    std::string addr = std::string(xmlnode->Attribute("addr"));
    raft_node["addr"] = addr;

    // int id = std::atoi(xmlnode->Attribute("id"));
    std::string node_name = std::to_string(id) + "-" + std::to_string(tinyrpc::IOThread::GetCurrentIOThread()->getThreadIndex() + 1);
    raft_node["name"] =  node_name;
    raft_node["id"] = id;

    InfoLog << "read raft server conf[" << addr << ":" << node_name << "]";

    if (addr == local_addr) {
      m_node_id = id;
      m_node_addr = addr;
      m_node_name = node_name;
      InfoLog << "read raft server conf[" << addr << ":" << node_name << "]";
    }
    m_nodes.push_back(std::move(raft_node));
    m_match_indexs.push_back(0);
    m_next_indexs.push_back(0);
    id++;
    m_node_count++;
  }

}

RaftNode::~RaftNode() {

}



void RaftNode::FollewerToCandidate() {


}

void RaftNode::handleAskVote(const AskVoteRequest& request, AskVoteResponse& response) {
  response.set_term(m_current_term);
  response.set_accept_result(ACCEPT_FAIL);

  if (request.candidate_term() < m_current_term) {
    response.set_accept_result(ACCEPT_FAIL);
    response.set_vote_fail_reason(formatString("AskVote failed, your's term is less than my term"));
    return;
  }
  if (m_voted_for_id == 0 || m_voted_for_id == request.candidate_id()) {
    if (request.last_log_index() >= (int32_t)m_logs.size()) {
      AppInfoLog << formatString("AskVote to [%s - %d - %s] succ",
          request.node_addr().c_str(), request.node_id(), request.node_name().c_str());
      m_voted_for_id = request.node_id();
      response.set_accept_result(ACCEPT_SUCC);
      m_state = RAFT_CANDIDATE_STATE;
      return;
    }
  }
}

void RaftNode::handleAppendLogEntries(const AppendLogEntriesRequest& request, AppendLogEntriesResponse& response) {
  response.set_term(m_current_term);
  response.set_accept_result(ACCEPT_FAIL);
  if (request.leader_term() < m_current_term) {
    // return false when leader's term less than current Node's term
    response.set_accept_result(ACCEPT_FAIL);
    response.set_append_fail_reason("AppendLogEntries failed, leader's term is less than current term");

    return;
  }

  int prev_log_index = request.prev_log_index();
  if ((prev_log_index >= (int)m_logs.size())
      || (m_logs[prev_log_index].term() != request.prev_log_term())) {

    // return false when Node can't find log match(prev_log_index, prev_log_term) 
    response.set_accept_result(ACCEPT_FAIL);
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
    m_commit_index = std::min(request.leader_commit_index(), (int32_t)m_logs.size());
  }

  response.set_accept_result(ACCEPT_SUCC);

}

// all Node execute 
void RaftNode::commonHandler() {
  for(int i = m_last_applied_index + 1; i <= m_commit_index; ++i) {
    applyToStateMachine(m_logs[i]);
    m_last_applied_index++;
  }
}

// only leader execute
void RaftNode::leaderHandler() {

}

// only follower execute
void RaftNode::followerHandler() {

}

// only candidate execute
void RaftNode::candidateHandler() {

}



int RaftNode::applyToStateMachine(const LogEntry& logs) {
  return 0;
}

int RaftNode::askVote() {
  if (m_state != RAFT_CANDIDATE_STATE) {
    AppErrorLog << formatString("[%s name: %s, term: %d, state: %s] state isn't CANDIDATE, can't askVote",
      m_node_addr.c_str(), m_node_name.c_str(), m_current_term, StateToString(m_state).c_str());
    return -1;
  }

  std::vector<std::pair<std::shared_ptr<AskVoteRequest>, std::shared_ptr<AskVoteResponse>>> rpc_list;
  for (size_t i = 1; i < m_nodes.size(); ++i) {
    if ((int)i == m_node_id) {
      continue;
    }
    std::shared_ptr<AskVoteRequest> request;
    std::shared_ptr<AskVoteResponse> response;

    request->set_candidate_id(m_node_id);
    request->set_candidate_term(m_current_term);
    request->set_last_log_index(m_logs.size() - 1);
    request->set_last_log_term(m_logs[m_logs.size() - 1].term());
    request->set_node_id(m_node_id);
    request->set_node_addr(m_node_addr);
    request->set_node_name(m_node_name);

    rpc_list.emplace_back(std::pair<std::shared_ptr<AskVoteRequest>, std::shared_ptr<AskVoteResponse>>(request, response));
  }

  AskVoteRPCs(rpc_list);

  int succ_count = 1;
  for (auto i : rpc_list) {
    if (i.second->accept_result() == ACCEPT_SUCC) {
      succ_count++;
      AppInfoLog << formatString("[%s name: %s, term: %d, state: %s] succ get vote from node[%s name: %s, term: %s, state: %s]",
        m_node_addr.c_str(), m_node_name.c_str(), m_current_term, StateToString(m_state),
        i.second->node_addr().c_str(), i.second->node_name().c_str(), i.second->term(), StateToString(i.second->state()));

      if (succ_count >= getMostNodeCount()) {
        AppInfoLog << "appendLogEntries succ";
        return 0;
      }
    } else {
      if (i.second->accept_result() == ACCEPT_FAIL) {
        AppErrorLog << formatString("[%s name: %s, term: %d, state: %s] failed apply log[%s] to node[%s name: %s, term: %s, state: %s], faild reason[%d, %s]",
          m_node_addr.c_str(), m_node_name.c_str(), m_current_term, StateToString(m_state),
          LogEntryToString(m_logs[match_index]).c_str(), 
          i.second->node_addr().c_str(), i.second->node_name().c_str(), i.second->term(), StateToString(i.second->state()).c_str(),
          i.second->append_fail_code(), i.second->append_fail_reason().c_str());

        if (i.second->append_fail_code() == ERR_TERM_MORE_THAN_LEADER && m_current_term < i.second->term()) {
          // TODO: become to follewer
          becomeFollower(i.second->term());
          return -1;
        }
      
      }
    }
  }

  return -1;

}

int RaftNode::appendLogEntries() {
  if (m_state != RAFT_LEADER_STATE) {
    AppErrorLog << formatString("[Term: %d][%s - %d - %s] current raft node state isn't LEADER, can't appendLogEntries",
      m_current_term, m_node_addr.c_str(), m_node_id, m_node_name.c_str());
    return -1;
  }
  std::vector<std::pair<std::shared_ptr<AppendLogEntriesRequest>, std::shared_ptr<AppendLogEntriesResponse>>> rpc_list;
  for (size_t i = 1; i < m_nodes.size(); ++i) {
    if ((int)i == m_node_id) {
      continue;
    }
    std::shared_ptr<AppendLogEntriesRequest> request;
    std::shared_ptr<AppendLogEntriesResponse> response;

    request->set_leader_term(m_current_term);
    request->set_leader_id(m_node_id);

    int last_index = m_logs.size() - 1;
 
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

  AppendLogEntriesRPCs(rpc_list);
  int succ_count = 1;

  for (auto i : rpc_list) {
    int match_index = i.first->last_log_index();
    if (i.second->accept_result() == ACCEPT_SUCC) {
      succ_count++;
      updateMatchIndex(i.first->node_id(), match_index);
      updateNextIndex(i.first->node_id(), match_index + 1);
      AppInfoLog << formatString("[%s name: %s, term: %d, state: %s] succ apply log[%s] to node[%s name: %s, term: %s, state: %s]",
        m_node_addr.c_str(), m_node_name.c_str(), m_current_term, StateToString(m_state),
        LogEntryToString(m_logs[match_index]).c_str(), 
        i.second->node_addr().c_str(), i.second->node_name().c_str(), i.second->term(), StateToString(i.second->state()));
    } else {

      if (i.second->accept_result() == ACCEPT_FAIL) {
        AppErrorLog << formatString("[%s name: %s, term: %d, state: %s] failed apply log[%s] to node[%s name: %s, term: %s, state: %s], faild reason[%d, %s]",
          m_node_addr.c_str(), m_node_name.c_str(), m_current_term, StateToString(m_state),
          LogEntryToString(m_logs[match_index]).c_str(), 
          i.second->node_addr().c_str(), i.second->node_name().c_str(), i.second->term(), StateToString(i.second->state()).c_str(),
          i.second->append_fail_code(), i.second->append_fail_reason().c_str());

        if (i.second->append_fail_code() == ERR_NOT_MATCH_PREINDEX) {
          updateNextIndex(i.first->node_id(), i.second->need_index());
        } else if (i.second->append_fail_code() == ERR_TERM_MORE_THAN_LEADER && m_current_term < i.second->term()) {
          // TODO: become to follewer
          becomeFollower(i.second->term());
          return -1;
        }
      }

    }
    
  }

  std::vector<int> tmp(m_match_indexs.begin() + 1, m_match_indexs.end());
  std::sort(tmp.begin(), tmp.end());
  

  if (succ_count >= getMostNodeCount()) {
    AppInfoLog << "appendLogEntries succ";
    return 0;
  }

  return -1;
}

void RaftNode::becomeFollower(int term) {
  AppErrorLog << formatString("[Term: %d][%s - %d - %s] raft node become to follewer [Term: %d, state: %s]",
    m_current_term, m_node_addr.c_str(), m_node_id, m_node_name.c_str(), m_current_term, StateToString(RAFT_FOLLOWER_STATE).c_str());
  m_state = RAFT_FOLLOWER_STATE;
  m_current_term = term;
}

void RaftNode::AskVoteRPCs(std::vector<std::pair<std::shared_ptr<AskVoteRequest>, std::shared_ptr<AskVoteResponse>>>& rpc_list) {
  CALL_RAFT_RPCS(AskVoteRequest, AskVoteResponse, AskVote);
}

void RaftNode::AppendLogEntriesRPCs(std::vector<std::pair<std::shared_ptr<AppendLogEntriesRequest>, std::shared_ptr<AppendLogEntriesResponse>>>& rpc_list) {
  CALL_RAFT_RPCS(AppendLogEntriesRequest, AppendLogEntriesResponse, AppendLogEntries);
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


void RaftNode::init() {
  charon::RaftNode* node = charon::RaftNode::GetRaftNode();
  m_state = RAFT_CANDIDATE_STATE;

  // first add current term
  m_current_term++;
  // give vote to self
  m_voted_for_id = m_node_id; 

  AppDebugLog << formatString("[%s name: %s, term: %s, state: %s] succ apply log[last log index: %d] to node[%s name: %s, term: %s, state: %s]",
    m_node_addr.c_str(), m_node_name.c_str(), m_current_term, StateToString(m_state));

  node->askVote();

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

}


