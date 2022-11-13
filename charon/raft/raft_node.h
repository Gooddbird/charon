#ifndef CHARON_RAFT_RAFT_NODE_H
#define CHARON_RAFT_RAFT_NODE_H

#include <vector>
#include <map>
#include "charon/pb/charon.pb.h"
#include "charon/comm/util.h"
#include "charon/raft/raft_partition.h"

namespace charon {


class RaftNode {
 public:
  RaftNode();
  ~RaftNode();

  RaftPartition* getRaftPartition(int hash_id);

  void addRaftNode();

  int getNodeCount();
  
  // get more than half nodes count
  int getMostNodeCount();

  KVMap getServerNode(int i);


 public:

  static std::string LogEntryToString(const LogEntry& log);

  static std::string StateToString(const RaftState& state);

  static RaftNode* GetRaftNode();


 private:
  std::vector<RaftPartition*> m_partitions;
  int m_size {0};

  // node info about all distributed server 
  // map is used to describe node info
  // key1: "addr" , value1: "127.0.0.1:19999"
  // key2: "name", value2: "xxx"
  // key3: "id", value3: "xxx"
  std::vector<KVMap> m_server_nodes;

  int m_node_count {0};

};

}

#endif  // CHARON_RAFT_RAFT_NODE_H