#include <vector>
#include <tinyxml/tinyxml.h>
#include <string>
#include "tinyrpc/comm/log.h"
#include "tinyrpc/comm/start.h"
#include "tinyrpc/net/tinypb/tinypb_rpc_async_channel.h"
#include "tinyrpc/net/tinypb/tinypb_rpc_controller.h"
#include "tinyrpc/net/tinypb/tinypb_rpc_closure.h"
#include "tinyrpc/net/tcp/io_thread.h"

#include "charon/raft/raft_node.h"
#include "charon/pb/raft.pb.h"
#include "charon/comm/util.h"


namespace charon {

static thread_local RaftNode* t_raft_node = NULL;

RaftNode::RaftNode() {
  m_current_term = 0;
  m_state = FOLLOWER;
  m_nodes.resize(100);

  std::string local_addr = tinyrpc::GetServer()->getLocalAddr()->toString();

  TiXmlElement* node =  tinyrpc::GetConfig()->getXmlNode("raft");
  assert(node != NULL);
  TiXmlElement* groups_node = node->FirstChildElement("rafe_servers"); 

  for (TiXmlElement* xmlnode = groups_node->FirstChildElement(); xmlnode!= NULL; xmlnode = xmlnode->NextSiblingElement()) {
    m_node_count++;
    KVMap node;
    std::string addr = std::string(xmlnode->Attribute("addr"));
    node["addr"] = addr;

    int id = std::atoi(xmlnode->Attribute("id"));
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
          AppDebugLog << formatString("[Term: %d][%s - %d - %s] receive a vote from [%s - %d - %s]",
            req->candidate_term(), req->node_addr().c_str(), req->node_id(), req->node_name().c_str(),
            res->node_addr().c_str(), res->node_id(), res->node_name().c_str());

          get_votes++;
          if (get_votes >= currentiRaftNode->getMostNodeCount()) {
            AppInfoLog <<  formatString("[Term: %d][%s - %d - %s] receive more than half votes, resume ",
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

    rpc_channel->saveCallee(rpc_controller, request, response, nullptr);

    RaftService_Stub stub(rpc_channel.get());
    AppInfoLog << "now askVote request to raft node[" << m_nodes[i]["addr"] << ", " << m_nodes[i]["name"] << "]";
    stub.AskVote(rpc_controller.get(), request.get(), response.get(), nullptr);
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
  return 0;
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

// RaftNodeContainer::RaftNodeContainer() {
//   int size = tinyrpc::GetIOThreadPoolSize();
//   for (int i =0; i < size; ++i) {
//     m_container.push_back(new RaftNode());
//   }
// }

// RaftNodeContainer:: ~RaftNodeContainer() {

// }

}


