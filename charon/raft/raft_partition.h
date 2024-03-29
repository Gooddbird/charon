#ifndef CHARON_RAFT_RAFT_PARTTITION_H
#define CHARON_RAFT_RAFT_PARTTITION_H 

#include <vector>
#include <map>
#include "charon/pb/charon.pb.h"
#include "charon/comm/util.h"
#include "tinyrpc/net/mutex.h"
#include "tinyrpc/net/timer.h"

namespace charon {

class RaftNode;

class RaftPartition {

 public:

  RaftPartition();

  ~RaftPartition();

 public:
  void resetElectionTimer();

  void startAppendLogHeart();

  void stopAppendLogHeart();

  int execute(const std::string& cmd);

  void initNodeInfo(const int& self_id, const std::string& name, const std::string& addr);

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

  int AskVoteRPCs(std::vector<std::pair<std::shared_ptr<AskVoteRequest>, std::shared_ptr<AskVoteResponse>>>& rpc_list);

  int AppendLogEntriesRPCs(std::vector<std::pair<std::shared_ptr<AppendLogEntriesRequest>, std::shared_ptr<AppendLogEntriesResponse>>>& rpc_list);

  RAFT_STATE getState();

  void setState(RAFT_STATE state);

  void updateNextIndex(const int& node_id, const int& v);

  void updateMatchIndex(const int& node_id, const int& v);

  void toFollower(int term);

  tinyrpc::TimerEvent::ptr getElectionTimerEvent();

  tinyrpc::TimerEvent::ptr getAppendLogTimerEvent();

 public:
  void lock();

  void unlock();


 // common state of all node
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
  // leader should send log's index for every node
  std::vector<int> m_next_indexs;
  // every raft node have already apply log'index
  std::vector<int> m_match_indexs;

 private:
  RAFT_STATE m_state;

  int m_self_id {0};
  std::string m_name;
  std::string m_addr;

  int m_elect_overtime {0};     // ms, the overtime of election progress
  int m_heart_interval {0};     // ms, the interval of leader to send appendLogEntries RPC

 private:
  tinyrpc::CoroutineMutex m_coroutine_mutex;

  tinyrpc::TimerEvent::ptr m_election_event {nullptr};
  tinyrpc::TimerEvent::ptr m_appendlog_event {nullptr};

};


}


#endif