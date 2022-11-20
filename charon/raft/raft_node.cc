
#include "charon/raft/raft_partition.h"
#include "charon/raft/raft_node.h"
#include "charon/comm/util.h"
#include "charon/comm/errcode.h"
#include "charon/comm/business_exception.h"
#include "tinyrpc/comm/start.h"

namespace charon {

static RaftNode* g_raft_node = NULL;

RaftNode::RaftNode() {
  m_part_count = 1;
  for (int i = 0; i < m_part_count; ++i) {
    m_partitions.push_back(new RaftPartition());
  }

  // add self' addr
  KVMap raft_node;
  m_node_count++;

  std::string addr = tinyrpc::GetServer()->getLocalAddr()->toString();
  raft_node["addr"] = addr;

  std::string node_name = std::to_string(m_node_count);
  raft_node["name"] = "root";
  raft_node["id"] = std::to_string(m_node_count);
  raft_node["partition_count"] = m_part_count;
  raft_node["lstate"] = "1";

  InfoLog << "read raft server conf[" << addr << ", " << node_name << "]";
  m_server_nodes[m_node_count].swap(raft_node);
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


const KVMap& RaftNode::getServerNode(int i) {
  return m_server_nodes[i];
}


RAFT_SERVER_LSTATE RaftNode::StringToRaftLState(const std::string& state) {
  if (state == "0") {
    return EN_RAFT_LSTATE_UNDEFINE;
  } else if (state == "1") {
    return EN_RAFT_LSTATE_ACTIVE;
  } else if (state == "2") {
    return EN_RAFT_LSTATE_DELETED;
  } else {
    throw BusinessException(ERR_PARAM_INPUT, "invalid lstate:" + state);
  }
}


void RaftNode::addRaftServerNode(ServerNode& node) {
  if (node.name().empty()) {
    throw BusinessException(ERR_PARAM_INPUT, "invalid empty node name");
  }

  KVMap raft_node;
  m_node_count++;

  std::string addr = node.addr();
  raft_node["addr"] = addr;

  raft_node["name"] = node.name();
  raft_node["id"] = std::to_string(m_node_count);
  if (node.lstate() != EN_RAFT_LSTATE_UNDEFINE) {
    raft_node["lstate"] = std::to_string(node.lstate());
  } else {
    raft_node["lstate"] = "1";
  }

  InfoLog << "succ add raft server conf[addr: " << addr << ", name:" << raft_node["name"] << ", id:"<< raft_node["id"] << "]";
  m_server_nodes[m_node_count].swap(raft_node);

  node.set_id(m_node_count);
}


void RaftNode::updateRaftServerNode(ServerNode& node) {
  size_t id = node.id();
  if (id < 1 || id >= m_server_nodes.size()) {
    throw BusinessException(ERR_PARAM_INPUT, "invalid server node id:" + std::to_string(id));
  }

  m_server_nodes[id]["name"] = node.name();
  m_server_nodes[id]["id"] = node.id();
  m_server_nodes[id]["addr"] = node.addr();
  m_server_nodes[id]["lstate"] = std::to_string(node.lstate());

}

void RaftNode::deleteRaftServerNode(ServerNode& node) {
  size_t id = node.id();
  if (id < 1 || id >= m_server_nodes.size()) {
    throw BusinessException(ERR_PARAM_INPUT, "invalid server node id:" + std::to_string(id));
  }
  m_server_nodes[id]["lstate"] = "2";

}

void RaftNode::queryRaftServerNode(ServerNode& node) {
  size_t id = node.id();
  if (id < 1 || id >= m_server_nodes.size()) {
    throw BusinessException(ERR_PARAM_INPUT, "invalid server node id:" + std::to_string(id));
  }
  node.set_name(m_server_nodes[id]["name"]);
  node.set_addr(m_server_nodes[id]["addr"]);
  node.set_lstate(StringToRaftLState(m_server_nodes[id]["addr"]));
  node.set_id(id);
}


}