#ifndef TINYRPC_RAFT_RAFT_NODE_H
#define TINYRPC_RAFT_RAFT_NODE_H

#include <vector>
#include <map>
#include "charon/pb/raft.pb.h"
#include "tinyrpc/net/mutex.h"
#include "tinyrpc/net/timer.h"


// raft errcode define
#define ERR_TERM_MORE_THAN_LEADER 80000001        // current term more than leader' term
#define ERR_NOT_MATCH_PREINDEX 80000002           // can't find a log match prev_log_index & prev_log_term
#define ERR_LOG_MORE_THAN_CANDIDATE 80000003      // current log is more new than candidate
#define ERR_TERM_MORE_THAN_CANDICATE 80000004     // current term more than leader' term
#define ERR_ALREADY_VOTED 80000005               // current term node has already voted 



namespace charon {

std::string LogEntryToString(const LogEntry& log);

std::string StateToString(const RaftNodeState& state);

class RaftNode {

 public:
  typedef std::map<std::string, std::string> KVMap;

  RaftNode();

  ~RaftNode();

  void FollewerToCandidate();

 public:
  void resetElectionTimer();

  void startAppendLogHeart();

  void stopAppendLogHeart();

  int execute(const std::string& cmd);

 public:
  // deal askVote RPC
  void handleAskVote(const AskVoteRequest& request, AskVoteResponse& response);

  // deal appendLogEntries RPC
  void handleAppendLogEntries(const AppendLogEntriesRequest& request, AppendLogEntriesResponse& response);


 public:
  // apply log to state Machine
  int applyToStateMachine(const LogEntry& logs);

  int appendLogEntries();

  void election();

  void AskVoteRPCs(std::vector<std::pair<std::shared_ptr<AskVoteRequest>, std::shared_ptr<AskVoteResponse>>>& rpc_list);

  void AppendLogEntriesRPCs(std::vector<std::pair<std::shared_ptr<AppendLogEntriesRequest>, std::shared_ptr<AppendLogEntriesResponse>>>& rpc_list);

  RaftNodeState getState();

  void setState(RaftNodeState state);

  void updateNextIndex(const int& node_id, const int& v);

  void updateMatchIndex(const int& node_id, const int& v);

  void toFollower(int term);

 public:
  int getNodeCount();
  
  // get more than half nodes count
  int getMostNodeCount();


 // common state of all node
 private:
  // Persistence state, you need store these states before rpc response 
  int m_current_term {0};
  int m_voted_for_id {0};
  // all logs have already apply to state machine
  std::vector<LogEntry> m_logs;
  

 private:
  // node info about all distributed server 
  // map is used to describe node info
  // key1: "addr" , value1: "127.0.0.1:19999"
  // key2: "name", value2: "xxx"
  // key3: "id", value3: "xxx"
  std::vector<KVMap> m_nodes;
  int m_node_count {0};

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
  // total raft node count

  int m_node_id {0};
  std::string m_node_name;
  std::string m_node_addr;

  int m_elect_overtime {0};     // ms, the overtime of election progress
  int m_heart_interval {0};     // ms, the interval of leader to send appendLogEntries RPC

 private:
  tinyrpc::CoroutineMutex m_coroutine_mutex;

  tinyrpc::TimerEvent::ptr m_election_event {nullptr};
  tinyrpc::TimerEvent::ptr m_appendlog_event {nullptr};

};


class RaftNodeContainer {
 public:

  RaftNodeContainer();
  ~RaftNodeContainer();

  RaftNode* getRaftNode(int hash_id);

  static RaftNodeContainer* GetRaftNodeContainer();

 private:
  std::vector<RaftNode*> m_container;
  int m_size {0};


};

}


#endif