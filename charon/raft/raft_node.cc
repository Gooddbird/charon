
#include "charon/raft/raft_partition.h"
#include "charon/raft/raft_node.h"
#include "charon/comm/util.h"

namespace charon {

static RaftNode* g_raft_node = NULL;

RaftNode::RaftNode() {
  m_size = 1;
  for (int i = 0; i < m_size; ++i) {
    m_partitions.push_back(new RaftPartition());
  }
}

RaftNode::~RaftNode() {

}

RaftPartition* RaftNode::getRaftPartition(int hash_id) {
  return m_partitions[0];
}

RaftNode* RaftNode::GetRaftNode() {
  if (g_raft_node) {
    return g_raft_node;
  }
  g_raft_node = new RaftNode();
  return g_raft_node;
}

int RaftNode::getNodeCount() {
  return m_node_count;
}

// get more than half nodes count
int RaftNode::getMostNodeCount() {
  return (m_node_count)/2 + 1;
}


std::string RaftNode::StateToString(const RaftState& state) {
  if (state == RAFT_LEADER_STATE) {
    return "Leader";
  }
  if (state == RAFT_FOLLOWER_STATE) {
    return "Follower";
  }
  if (state == RAFT_CANDIDATE_STATE) {
    return "Candidate";
  }
  return "";
}

std::string RaftNode::LogEntryToString(const LogEntry& log) {
  return formatString("[term: %d, index: %d, cmd: %s]", log.term(), log.index(), log.cmd().c_str());
}


KVMap RaftNode::getServerNode(int i) {
  return m_server_nodes[i];
}

}