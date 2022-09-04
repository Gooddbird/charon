#ifndef TINYRPC_RAFT_RAFT_NODE_H
#define TINYRPC_RAFT_RAFT_NODE_H

#include <vector>
#include <map>
#include "charon/pb/raft.pb.h"
#include "tinyrpc/net/mutex.h"


// raft errcode define
#define ERR_TERM_MORE_THAN_LEADER 80000001
#define ERR_NOT_MATCH_PREINDEX 80000002


namespace charon {

enum RaftNodeState {
  FOLLOWER = 1,
  CANDIDATE = 2,
  LEADER = 3
};


class RaftNode {

 public:
  typedef std::map<std::string, std::string> KVMap;

  RaftNode();

  ~RaftNode();

  void FollewerToCandidate();

 public:
  static RaftNode* GetRaftNode();

  void init();

  void execute(const std::string& cmd);

 public:
  // deal askVote RPC
  void handleAskVote(const AskVoteRequest& request, AskVoteResponse& response);

  // deal appendLogEntries RPC
  void handleAppendLogEntries(const AppendLogEntriesRequest& request, AppendLogEntriesResponse& response);

  // all node execute 
  void commonHandler();

  // only leader execute
  void leaderHandler();

  // only follower execute
  void followerHandler();

  // only candidate execute
  void candidateHandler();


 public:
  // apply log to state Machine
  int applyToStateMachine(const LogEntry& logs);

  int askVote();

  int appendLogEntries();

  int getNodeCount();
  
  // get more than half nodes count
  int getMostNodeCount();

  RaftNodeState getState();

  void setState(RaftNodeState state);

  void updateNextIndex(const int& node_id, const int& v);
  void updateMatchIndex(const int& node_id, const int& v);

 // common state of all node
 private:
  // Persistence state, you need store these states before rpc response 
  int m_current_term {0};
  int m_voted_for_id {0};
  // all logs have already apply to state machine
  std::vector<LogEntry> m_logs;
  
  // node info about all distributed server 
  // map is used to describe node info
  // key1: "addr" , value1: "127.0.0.1:19999"
  // key2: "name", value2: "xxx"
  // key3: "id", value3: "xxx"
  std::vector<KVMap> m_nodes;

 private:
  // volatile state

  // the highest commit log index
  int m_commit_index {0};
  // the highest apply log index, which means log's cmd have already excute
  int m_last_applied_index {0};

 // only leader state
 private:
  // leader should send log's index for every node
  std::vector<int> m_next_indexs;
  // every raft node have already apply log'index
  std::vector<int> m_match_indexs;

 private:
  RaftNodeState m_state;
  int m_node_count {0};

  int m_node_id {0};
  std::string m_node_name;
  std::string m_node_addr;

 private:
  tinyrpc::CoroutineMutex m_coroutine_mutex;

};


class RaftNodeContainer {
 public:

  RaftNodeContainer();
  ~RaftNodeContainer();

 private:
  std::vector<RaftNode*> m_container;
};

}


#endif