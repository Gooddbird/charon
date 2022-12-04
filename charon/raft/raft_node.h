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

  const ServerNode& getSelfNode();

  void addRaftServerNode(ServerNode& node);

  void updateRaftServerNode(ServerNode& node);

  void deleteRaftServerNode(ServerNode& node);

  void queryRaftServerNode(ServerNode& node);

  void queryAllRaftServerNode(const ::QueryAllRaftServerNodeRequest& request, std::vector<ServerNode>& node_list);

  void resetNodes(std::vector<ServerNode>& new_node_list);

  void setSelfId(int id);

  void setPartitions(int partition_count);

 private:
  void freePartitions();


 public:

  static RaftNode* GetRaftNode();

  static std::string LogEntryToString(const LogEntry& log);

  static std::string StateToString(const RAFT_STATE& state);

  static std::string LStateToString(const RAFT_SERVER_LSTATE& state);

  static std::string RaftServerNodeToString(const ServerNode& node);


 private:
  std::vector<ServerNode> m_server_nodes;

  std::vector<RaftPartition*> m_partitions;

  int m_active_node_count {0};
  int m_part_count {0};
  int m_self_id {0};

};

}

#endif  // CHARON_RAFT_RAFT_NODE_H