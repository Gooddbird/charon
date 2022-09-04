#include <vector>
#include <tinyxml/tinyxml.h>
#include <string>
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


namespace charon {

static thread_local RaftNode* t_raft_node = NULL;

RaftNode::RaftNode() {
  // log at index 0 is not use
  m_logs.emplace_back(LogEntry());
  m_nodes.resize(100);
  m_match_indexs.resize(100);
  m_next_indexs.resize(100);

  m_current_term = 0;
  m_state = FOLLOWER;
  std::string local_addr = tinyrpc::GetServer()->getLocalAddr()->toString();

  TiXmlElement* node =  tinyrpc::GetConfig()->getXmlNode("raft");
  assert(node != NULL);
  TiXmlElement* groups_node = node->FirstChildElement("rafe_servers"); 
  int id = 1;
  for (TiXmlElement* xmlnode = groups_node->FirstChildElement(); xmlnode!= NULL; xmlnode = xmlnode->NextSiblingElement()) {
    m_node_count++;
    KVMap node;
    std::string addr = std::string(xmlnode->Attribute("addr"));
    node["addr"] = addr;

    // int id = std::atoi(xmlnode->Attribute("id"));
    std::string node_name = std::to_string(id) + "-" + std::to_string(tinyrpc::IOThread::GetCurrentIOThread()->getThreadIndex() + 1);
    node["name"] =  node_name;
    node["id"] = id;

    InfoLog << "read raft server conf[" << addr << ":" << node_name << "]";

    if (addr == local_addr) {
      m_node_id = id;
      m_node_addr = addr;
      m_node_name = node_name;
      InfoLog << "read raft server conf[" << addr << ":" << node_name << "]";
    }
    m_nodes[id] = node;
    m_match_indexs[id] = 0;
    m_next_indexs[id] = 0;
    id++;

  }

}

RaftNode::~RaftNode() {
}

RaftNode* RaftNode::GetRaftNode() {
  if (t_raft_node) {
    return t_raft_node;
  }
  t_raft_node = new RaftNode();
  return t_raft_node;
}

void RaftNode::FollewerToCandidate() {


}

void RaftNode::handleAskVote(const AskVoteRequest& request, AskVoteResponse& response) {
  response.set_term(m_current_term);
  response.set_vote_result(VOTE_FAILED);

  if (request.candidate_term() < m_current_term) {
    response.set_vote_result(VOTE_FAILED);
    response.set_vote_fail_reason(formatString("AskVote failed, your's term is less than my term"));
    return;
  }
  if (m_voted_for_id == 0 || m_voted_for_id == request.candidate_id()) {
    if (request.last_log_index() >= (int32_t)m_logs.size()) {
      AppInfoLog << formatString("AskVote to [%s - %d - %s] succ",
          request.node_addr().c_str(), request.node_id(), request.node_name().c_str());
      m_voted_for_id = request.node_id();
      response.set_vote_result(VOTE_SUCC);
      m_state = CANDIDATE;
      return;
    }
  }
}

void RaftNode::handleAppendLogEntries(const AppendLogEntriesRequest& request, AppendLogEntriesResponse& response) {
  response.set_term(m_current_term);
  response.set_append_result(APPEND_FAILED);
  if (request.leader_term() < m_current_term) {
    // return false when leader's term less than current Node's term
    response.set_append_result(APPEND_FAILED);
    response.set_append_fail_reason("AppendLogEntries failed, leader's term is less than current term");

    return;
  }

  int prev_log_index = request.prev_log_index();
  if ((prev_log_index >= (int)m_logs.size())
      || (m_logs[prev_log_index].term() != request.prev_log_term())) {

    // return false when Node can't find log match(prev_log_index, prev_log_term) 
    response.set_append_result(APPEND_FAILED);
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

  response.set_append_result(APPEND_SUCC);

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
  if (m_state != CANDIDATE) {
    AppErrorLog << "current raft node [" << m_node_name << "] state isn't CANDIDATE, can't askVote";
    return -1;
  }

  RaftNode* currentiRaftNode = this;
  tinyrpc::Coroutine* cur_cor = tinyrpc::Coroutine::GetCurrentCoroutine();

  // total get votes count, first is 1 (beacsuse give a vote to self)
  int get_votes = 1;

  // give response node's count
  int res_nodes = 0;

  for (size_t i = 1; i < m_nodes.size(); ++i) {
    if ((int)i == m_node_id) {
      continue;
    }

    std::shared_ptr<AskVoteRequest> request = std::make_shared<AskVoteRequest>();
    std::shared_ptr<AskVoteResponse> response = std::make_shared<AskVoteResponse>();

    tinyrpc::TinyPbRpcAsyncChannel::ptr rpc_channel = 
      std::make_shared<tinyrpc::TinyPbRpcAsyncChannel>(std::make_shared<tinyrpc::IPAddress>(m_nodes[i]["addr"]));

    tinyrpc::TinyPbRpcController::ptr rpc_controller = std::make_shared<tinyrpc::TinyPbRpcController>();

    tinyrpc::TinyPbRpcClosure::ptr closure = std::make_shared<tinyrpc::TinyPbRpcClosure>(
      [&get_votes, &res_nodes, request, response, rpc_controller, cur_cor, currentiRaftNode]() mutable {
        res_nodes++;
        // if call resume, there's request will not deconstruct, it cause memory leak
        // we just need get normal point there
        auto req = request.get();
        request.reset();
        auto res = response.get();
        response.reset();
        auto controller = rpc_controller.get();
        rpc_controller.reset();

        if (controller->ErrorCode() ==0 &&
          res->ret_code() == 0 && res->vote_result() == VOTE_SUCC) {
          AppInfoLog << formatString("[Term: %d][%s - %d - %s] receive a vote from [%s - %d - %s]",
            req->candidate_term(), req->node_addr().c_str(), req->node_id(), req->node_name().c_str(),
            res->node_addr().c_str(), res->node_id(), res->node_name().c_str());

          get_votes++;
          if (get_votes >= currentiRaftNode->getMostNodeCount()) {
            AppInfoLog << formatString("[Term: %d][%s - %d - %s] receive more than half votes, resume ",
              req->candidate_term(), req->node_addr().c_str(), req->node_id(), req->node_name().c_str());

            tinyrpc::Coroutine::Resume(cur_cor);
            return;
          }
        }
        if (res_nodes + 1 == currentiRaftNode->getNodeCount()) {
          // the last node response, also resume
          tinyrpc::Coroutine::Resume(cur_cor);
        }
      }
    );

    rpc_controller->SetTimeout(5000);

    request->set_candidate_id(m_node_id);
    request->set_candidate_term(m_current_term);
    request->set_last_log_index(m_logs.size() - 1);
    request->set_last_log_term(m_logs[m_logs.size() - 1].term());
    request->set_node_id(m_node_id);
    request->set_node_addr(m_node_addr);
    request->set_node_name(m_node_name);

    rpc_channel->saveCallee(rpc_controller, request, response, closure);

    RaftService_Stub stub(rpc_channel.get());
    AppInfoLog << "now askVote request to raft node[" << m_nodes[i]["addr"] << ", " << m_nodes[i]["name"] << "]";
    stub.AskVote(rpc_controller.get(), request.get(), response.get(), closure.get());
  }

  // pending, will be resume by two ways
  // 1. more than half node give votes to this raft node
  // 2. all nodes already response
  // 3. receive AppendLogEntries req from higher term raft node
  tinyrpc::Coroutine::Yield();


  if (get_votes >= GetRaftNode()->getMostNodeCount()) {
    AppInfoLog << formatString("[Term: %d][%s - %d - %s] receive most node votes, now resume coroutine, ready to become leader node",
      m_current_term, m_node_addr.c_str(), m_node_id, m_node_name.c_str());
    return 0;
  }

  return -1;

}

int RaftNode::appendLogEntries() {
  if (m_state != LEADER) {
    AppErrorLog << formatString("[Term: %d][%s - %d - %s] current raft node state isn't LEADER, can't appendLogEntries",
      m_current_term, m_node_addr.c_str(), m_node_id, m_node_name.c_str());
    return -1;
  }

  RaftNode* currentiRaftNode = this;
  tinyrpc::Coroutine* cur_cor = tinyrpc::Coroutine::GetCurrentCoroutine();

  // total get votes count, first is 1 (beacsuse give a vote to self)
  int appsucc_counts = 1;

  // give response node's count
  int res_nodes = 0;

  for (size_t i = 1; i < m_nodes.size(); ++i) {
    if ((int)i == m_node_id) {
      continue;
    }

    std::shared_ptr<AppendLogEntriesRequest> request = std::make_shared<AppendLogEntriesRequest>();
    std::shared_ptr<AppendLogEntriesResponse> response = std::make_shared<AppendLogEntriesResponse>();

    tinyrpc::TinyPbRpcAsyncChannel::ptr rpc_channel = 
      std::make_shared<tinyrpc::TinyPbRpcAsyncChannel>(std::make_shared<tinyrpc::IPAddress>(m_nodes[i]["addr"]));

    tinyrpc::TinyPbRpcController::ptr rpc_controller = std::make_shared<tinyrpc::TinyPbRpcController>();

    rpc_controller->SetTimeout(5000);


    tinyrpc::TinyPbRpcClosure::ptr closure = std::make_shared<tinyrpc::TinyPbRpcClosure>(
      [&appsucc_counts, &res_nodes, request, response, rpc_controller, cur_cor, currentiRaftNode, i]() mutable {
        res_nodes++;
        // if call resume, there's request will not deconstruct, it cause memory leak
        // we just need get normal point there
        auto req = request.get();
        request.reset();
        auto res = response.get();
        response.reset();
        auto controller = rpc_controller.get();
        rpc_controller.reset();

        if (controller->ErrorCode() == 0 && res->ret_code() == 0) {
          if (res->append_result() == APPEND_SUCC) {

            AppInfoLog << formatString("[Term: %d][%s - %d - %s] receive a success append Log from [%s - %d - %s]",
              req->leader_term(), req->node_addr().c_str(), req->node_id(), req->node_name().c_str(),
              res->node_addr().c_str(), res->node_id(), res->node_name().c_str());

            if (req->last_log_index() != 0) {
              currentiRaftNode->updateNextIndex(i, req->last_log_index() + 1);
              currentiRaftNode->updateMatchIndex(i, req->last_log_index());
            }
            appsucc_counts++;
            if (appsucc_counts >= currentiRaftNode->getMostNodeCount()) {
              AppInfoLog << formatString("[Term: %d][%s - %d - %s] receive more than half append Log, resume ",
                req->leader_term(), req->node_addr().c_str(), req->node_id(), req->node_name().c_str());

              tinyrpc::Coroutine::Resume(cur_cor);
              return;
            }
          } else if (res->append_result() == APPEND_FAILED) {

            AppInfoLog << formatString("[Term: %d][%s - %d - %s] receive a failed append Log from [%s - %d - %s], fail_reason[%d: %s]",
              req->leader_term(), req->node_addr().c_str(), req->node_id(), req->node_name().c_str(),
              res->node_addr().c_str(), res->node_id(), res->node_name().c_str(), res->append_fail_code(), res->append_fail_reason().c_str());

            // if follower reply can't match preindex
            if (res->append_fail_code() == ERR_NOT_MATCH_PREINDEX) {
              currentiRaftNode->updateNextIndex(i, res->need_index());
            } else if (res->append_fail_code() == ERR_TERM_MORE_THAN_LEADER) {
              // should resume coroutine, because this node maybe not real leader, a new leader have be elected
              tinyrpc::Coroutine::Resume(cur_cor);
            }
          }
        }
        if (res_nodes + 1 == currentiRaftNode->getNodeCount()) {
          // the last node response, also resume
          tinyrpc::Coroutine::Resume(cur_cor);
        }
      }
    );

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

    rpc_channel->saveCallee(rpc_controller, request, response, closure);
    RaftService_Stub stub(rpc_channel.get());
    AppInfoLog << "now appendLogEntries request to raft node[" << m_nodes[i]["addr"] << ", " << m_nodes[i]["name"] << "]";
    stub.AppendLogEntries(rpc_controller.get(), request.get(), response.get(), closure.get());
  }

  // pending, will be resume by two ways
  // 1. more than half node give votes to this raft node
  // 2. all nodes already response
  // 3. receive AppendLogEntries req from higher term raft node
  tinyrpc::Coroutine::Yield();


  if (appsucc_counts >= getMostNodeCount()) {
    AppInfoLog << formatString("[Term: %d][%s - %d - %s] receive most node append log success, now can commit log",
      m_current_term, m_node_addr.c_str(), m_node_id, m_node_name.c_str());
    return 0;
  }

  return -1;
}


int RaftNode::getNodeCount() {
  return m_node_count;
}

// get more than half nodes count
int RaftNode::getMostNodeCount() {
  return m_node_count/2 + 1;
}


RaftNodeState RaftNode::getState() {
  return m_state;
}

void RaftNode::setState(RaftNodeState state) {
  m_state = state;
}


void RaftNode::init() {
  charon::RaftNode* node = charon::RaftNode::GetRaftNode();
  m_state = FOLLOWER;

  // first add current term
  m_current_term++;
  // give vote to self
  m_voted_for_id = m_node_id; 

  AppDebugLog << formatString("[%s - %d - %s] [term: %d] begin to ask vote",
            m_node_addr.c_str(), m_node_id, m_node_name.c_str(), m_current_term);

  node->askVote();

}


void RaftNode::execute(const std::string& cmd) {

  m_coroutine_mutex.lock();
  if (m_state != LEADER) {
    // redirect to leader
  }
  LogEntry log;
  int last_index = m_logs.size() - 1;
  last_index++;
  log.set_term(m_current_term);
  log.set_index(last_index);
  m_logs.push_back(std::move(log));

  int rt = appendLogEntries();
  if (rt != 0) {

  }

  m_coroutine_mutex.unlock();
  return;
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

}


