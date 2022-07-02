#ifndef TINYRPC_RAFT_RAFT_SERVER_H
#define TINYRPC_RAFT_RAFT_SERVER_H

#include <vector>
#include "charon/pb/charon.pb.h"


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

 // common state of all server
 public:
  // Persistence state, you need store these states before rpc response 
  int m_current_term {0};
  int m_voted_for_id {0};
  std::vector<LogEntry> m_logs;


  // volatile state

  // the highest commit log index
  int m_commit_index {0};
  // the highest apply log index, which means log's cmd have already excute
  int m_last_applied_index {0};

 // only leader state
 public:
  std::vector<int> m_next_indexs;
  std::vector<int> m_match_indexs;



 private:
  RaftServerState m_state;
};

}


#endif