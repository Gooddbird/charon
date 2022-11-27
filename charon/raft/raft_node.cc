
#include "charon/raft/raft_partition.h"
#include "charon/raft/raft_node.h"
#include "charon/comm/util.h"
#include "charon/comm/errcode.h"
#include "charon/comm/business_exception.h"
#include "tinyrpc/comm/start.h"

namespace charon {

static RaftNode* g_raft_node = NULL;

RaftNode::RaftNode() {
  // index 0 is not use, just placeholder 
  m_server_nodes.push_back(ServerNode());

  // default part_count is 1
  m_part_count = 1;
  for (int i = 0; i < m_part_count; ++i) {
    m_partitions.push_back(new RaftPartition());
  }

  // add self' addr
  // KVMap raft_node;
  ServerNode node;
  m_node_count = 1;

  std::string addr = tinyrpc::GetServer()->getLocalAddr()->toString();
  node.set_addr(addr);

  node.set_name("root");
  node.set_id(m_node_count);
  node.set_partition_count(m_part_count);
  node.set_lstate(EN_RAFT_LSTATE_ACTIVE);

  AppInfoLog << "init root raft server node " << RaftServerNodeToString(node);

  m_server_nodes.emplace_back(node);
}

RaftNode::~RaftNode() {

}

std::string RaftNode::StateToString(const RAFT_STATE& state) {
  if (state == EN_RAFT_STATE_LEADER) {
    return "Leader";
  }
  if (state == EN_RAFT_STATE_FOLLOWER) {
    return "Follower";
  }
  if (state == EN_RAFT_STATE_CANDIDATE) {
    return "Candidate";
  }
  return "";
}

std::string RaftNode::LogEntryToString(const LogEntry& log) {
  return formatString("[term: %d, index: %d, cmd: %s]", log.term(), log.index(), log.cmd().c_str());
}

std::string RaftNode::LStateToString(const RAFT_SERVER_LSTATE& state) {
  if (state == EN_RAFT_LSTATE_ACTIVE) {
    return "Active";
  } else if (state == EN_RAFT_LSTATE_DELETED) {
    return "Deleted";
  } else {
    return "Undefine error state";
  }
}

std::string RaftNode::RaftServerNodeToString(const ServerNode& node) {
  std::stringstream ss;
  ss << "{\"node_id\": " << node.id()
    << ", \"node_name\": \"" << node.name() << "\""
    << ", \"node_addr\": \"" << node.addr() << "\"" 
    << ", \"node_state\": \"" << LStateToString(node.lstate()) << "\"" 
    << ", \"partition_count\": " << node.partition_count()
    << "}";

  return ss.str();
}

RaftNode* RaftNode::GetRaftNode() {
  if (g_raft_node) {
    return g_raft_node;
  }
  g_raft_node = new RaftNode();
  return g_raft_node;
}


RaftPartition* RaftNode::getRaftPartition(int hash_id) {
  return m_partitions[0];
}


int RaftNode::getNodeCount() {
  return m_node_count;
}

// get more than half nodes count
int RaftNode::getMostNodeCount() {
  return (m_node_count)/2 + 1;
}

const ServerNode& RaftNode::getServerNode(int i) {
  return m_server_nodes[i];
}

void RaftNode::addRaftServerNode(ServerNode& node) {
  if (node.name().empty()) {
    throw BusinessException(ERR_PARAM_INPUT, "invalid empty node name", __FILE__, __LINE__);
  }

  m_node_count++;
  node.set_id(m_node_count);
  m_server_nodes.push_back(node);
  AppInfoLog << "succ add raft server node: " << RaftServerNodeToString(node);

}


void RaftNode::updateRaftServerNode(ServerNode& node) {
  size_t id = node.id();
  if (id < 1 || id >= m_server_nodes.size()) {
    throw BusinessException(ERR_PARAM_INPUT, "invalid server node id:" + std::to_string(id), __FILE__, __LINE__);
  }

  if ((int)id != node.id() || m_server_nodes[id].id() != node.id()) {
    throw BusinessException(ERR_PARAM_INPUT, "change node's id", __FILE__, __LINE__);
  }

  m_server_nodes[id].set_name(node.name());
  m_server_nodes[id].set_addr(node.addr());
  m_server_nodes[id].set_lstate(node.lstate());

  AppInfoLog << "succ update raft server node: " << RaftServerNodeToString(m_server_nodes[id]); 
}

void RaftNode::deleteRaftServerNode(ServerNode& node) {
  size_t id = node.id();
  if (id < 1 || id >= m_server_nodes.size()) {
    throw BusinessException(ERR_PARAM_INPUT, "invalid server node id:" + std::to_string(id), __FILE__, __LINE__);
  }
  m_server_nodes[id].set_lstate(EN_RAFT_LSTATE_DELETED);
  AppInfoLog << "succ delete raft server node: " << RaftServerNodeToString(m_server_nodes[id]);

}

void RaftNode::queryRaftServerNode(ServerNode& node) {
  size_t id = node.id();
  if (id < 1 || id >= m_server_nodes.size()) {
    throw BusinessException(ERR_PARAM_INPUT, "invalid server node id:" + std::to_string(id), __FILE__, __LINE__);
  }
  if (m_server_nodes[id].id() != node.id()) {
    throw BusinessException(ERR_PARAM_INPUT, "node's id not equal to request node's id", __FILE__, __LINE__);
  }
  node = m_server_nodes[id];

  AppInfoLog << "succ query raft server" << RaftServerNodeToString(node);
}


void RaftNode::queryAllRaftServerNode(std::vector<ServerNode>& node_list) {
  node_list.clear();
  node_list = m_server_nodes;

  AppInfoLog << "succ query all raft server node list ";
}


}