#include <vector>
#include "tinyrpc/comm/log.h" 

#include "charon/raft/raft_server.h"
#include "charon/pb/raft.pb.h"
#include "charon/comm/util.h"


namespace charon {

static thread_local RaftServer* t_raft_server = NULL;

RaftServer* RaftServer::GetRaftServer() {
  if (!t_raft_server) {
    return g_raft_server;
  }
  t_raft_server = new RaftServer();
  return t_raft_server;
}


RaftServer::FollewerToCandidate() {


}

void RaftServer::handleAskVote(const AskVoteRequest& request, AskVoteResponse& response) {
  response.set_term(m_current_term);
  response.set_vote_result(0);

  if (request.candidate_term() < m_current_term) {
    response.set_vote_result(0);
    response.set_allocated_vote_fail_reason(formatString("AskVote failed, your's term is less than my term"));
    return;
  }
  if (m_voted_for_id == 0 || m_voted_for_id == request.candidate_id()) {
    if (request.last_log_index() >= m_logs.size()) {
      response.set_vote_result(1);
      AppInfoLog << "AskVote succ";
    }
  }
}

void RaftServer::handleAppendLogEntries(const AppendLogEntriesRequest& request, AppendLogEntriesResponse& response) {
  response.set_term(m_current_term);
  response.set_append_result(0);
  if (request.leader_term() < m_current_term) {
    // return false when leader's term less than current server's term
    response.set_append_result(0);
    response.set_append_fail_reason("AppendLogEntries failed, leader's term is less than current term");

    return;
  }

  int prev_log_index = request.prev_log_index();
  if (prev_log_index >= m_logs.size() || m_logs[prev_log_index].term != request.prev_log_term()) {

    // return false when server can't find log match(prev_log_index, prev_log_term) 
    response.set_append_result(0);
    response.set_append_fail_reason(
      formatString("AppendLogEntries failed, server can't match prev_log_index[%d], prev_log_term[%d]",
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
    m_commit_index = std::min(request.leader_commit_index(), m_logs.size());
  }

  response.set_append_result(1);

}

}


