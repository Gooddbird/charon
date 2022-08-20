#ifndef TINYRPC_RAFT_RAFT_SERVER_H
#define TINYRPC_RAFT_RAFT_SERVER_H

#include <vector>
#include "charon/pb/raft.pb.h"


namespace charon {

enum RaftServerState {
  FOLLOWER = 1,
  CANDIDATE = 2,
  LEADER = 3
};

class RaftServer {

 public:
  RaftServer() = default;

  ~RaftServer() = default;

  void FollewerToCandidate();

 public:
  static RaftServer* GetRaftServer();

 public:
  void handleAskVote(const AskVoteRequest& request, AskVoteResponse& response);

  void handleAppendLogEntries(const AppendLogEntriesRequest& request, AppendLogEntriesResponse& response);

 // common state of all server
 private:
  // Persistence state, you need store these states before rpc response 
  int m_current_term {0};
  int m_voted_for_id {0};
  // all logs have already apply to state machine
  std::vector<LogEntry> m_logs;

 private:
  // volatile state

  // the highest commit log index
  int m_commit_index {0};
  // the highest apply log index, which means log's cmd have already excute
  int m_last_applied_index {0};

 // only leader state
 private:
  // leader should send log's index for every server
  std::vector<int> m_next_indexs;
  // every server have already apply log'index
  std::vector<int> m_match_indexs;


 private:
  RaftServerState m_state;
};

}


#endif