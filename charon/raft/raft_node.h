#ifndef CHARON_RAFT_RAFT_NODE_H
#define CHARON_RAFT_RAFT_NODE_H

#include <vector>
#include <map>
#include <string>
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

  const ServerNode& getServerNode(int i);

  void addRaftServerNode(ServerNode& node);

  void updateRaftServerNode(ServerNode& node);

  void deleteRaftServerNode(ServerNode& node);

  void queryRaftServerNode(ServerNode& node);

  void queryAllRaftServerNode(std::vector<ServerNode>& node_list);


 public:

  static RaftNode* GetRaftNode();

  static std::string LogEntryToString(const LogEntry& log);

  static std::string StateToString(const RAFT_STATE& state);

  static std::string LStateToString(const RAFT_SERVER_LSTATE& state);

  static std::string RaftServerNodeToString(const ServerNode& node);


 private:
  std::vector<RaftPartition*> m_partitions;
  int m_part_count {0};

  // node info about all distributed server 
  // map is used to describe node info
  // key1: "addr" , value1: "127.0.0.1:19999"
  // key2: "name", value2: "xxx"
  // key3: "id", value3: "xxx"
  // key4: "lstate", value4: 1-active, otherwise deleted
  // index 0 is undefine, so the first raft server node index is 1
  std::vector<ServerNode> m_server_nodes;

  int m_node_count {0};

};

}

#endif  // CHARON_RAFT_RAFT_NODE_H