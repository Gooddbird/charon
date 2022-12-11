
#include "charon/raft/raft_partition.h"
#include "charon/raft/raft_node.h"
#include "charon/comm/util.h"
#include "charon/comm/errcode.h"
#include "charon/comm/business_exception.h"
#include "tinyrpc/comm/start.h"

namespace charon {

static RaftNode* g_raft_node = NULL;

RaftNode::RaftNode() {

  // default part_count is 1
  m_part_count = 1;
  m_self_id = 1;

  ServerNode node;
  m_active_node_count = 1;

  std::string addr = tinyrpc::GetServer()->getLocalAddr()->toString();
  std::string name = "root";
  node.set_addr(addr);

  node.set_name(name);
  node.set_id(1);
  node.set_partition_count(m_part_count);
  node.set_lstate(EN_RAFT_LSTATE_ACTIVE);
  node.set_sync_state(EN_SYNC_STATE_SYNC_FINISHED);

  AppInfoLog << "init root raft server node " << RaftServerNodeToString(node);

  m_server_nodes.emplace_back(node);

  for (int i = 0; i < m_part_count; ++i) {
    RaftPartition* partition = new RaftPartition();
    partition->initNodeInfo(m_self_id, name, addr);
    m_partitions.push_back(partition);
  }

}

RaftNode::~RaftNode() {
  freePartitions();
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
  if (!g_raft_node) {
    g_raft_node = new RaftNode();
  }
  return g_raft_node;
}


RaftPartition* RaftNode::getRaftPartition(int hash_id) {
  return m_partitions[0];
}


int RaftNode::getNodeCount() {
  return m_active_node_count;
}

// get more than half nodes count
int RaftNode::getMostNodeCount() {
  return (m_active_node_count)/2 + 1;
}

const ServerNode& RaftNode::getServerNode(int i) {
  return m_server_nodes[i - 1];
}

const ServerNode& RaftNode::getSelfNode() {
  return m_server_nodes[m_self_id - 1];
}

void RaftNode::addRaftServerNode(ServerNode& node) {
  if (node.name().empty()) {
    throw BusinessException(ERR_PARAM_INPUT, "invalid empty node name", __FILE__, __LINE__);
  }
  if (node.lstate() == EN_RAFT_LSTATE_ACTIVE) {
    m_active_node_count++;
  }
  node.set_id(m_server_nodes.size() + 1);
  m_server_nodes.push_back(node);
  AppInfoLog << "succ add raft server node: " << RaftServerNodeToString(node);

}


void RaftNode::updateRaftServerNode(ServerNode& node) {
  size_t id = node.id();
  if (id < 1 || id > m_server_nodes.size()) {
    throw BusinessException(ERR_PARAM_INPUT, "invalid server node id:" + std::to_string(id), __FILE__, __LINE__);
  }

  if (m_server_nodes[id - 1].id() != node.id()) {
    throw BusinessException(ERR_PARAM_INPUT, "change node's id", __FILE__, __LINE__);
  }

  if (m_server_nodes[id - 1].lstate() != EN_RAFT_LSTATE_ACTIVE && node.lstate() == EN_RAFT_LSTATE_ACTIVE) {
    m_active_node_count++;
  }

  if (m_server_nodes[id - 1].lstate() == EN_RAFT_LSTATE_ACTIVE && node.lstate() == EN_RAFT_LSTATE_DELETED) {
    m_active_node_count--;
  }

  m_server_nodes[id - 1].set_name(node.name());
  m_server_nodes[id - 1].set_addr(node.addr());
  m_server_nodes[id - 1].set_lstate(node.lstate());


  AppInfoLog << "succ update raft server node: " << RaftServerNodeToString(m_server_nodes[id]); 
}

void RaftNode::deleteRaftServerNode(ServerNode& node) {
  size_t id = node.id();
  if (id < 1 || id >= m_server_nodes.size()) {
    throw BusinessException(ERR_PARAM_INPUT, "invalid server node id:" + std::to_string(id), __FILE__, __LINE__);
  }
  m_server_nodes[id].set_lstate(EN_RAFT_LSTATE_DELETED);
  m_active_node_count--;
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


void RaftNode::queryAllRaftServerNode(const ::QueryAllRaftServerNodeRequest& request, std::vector<ServerNode>& node_list) {
  node_list.clear();
  RAFT_SERVER_LSTATE lstate = request.lstate();
  RAFT_SERVER_SYNC_STATE state = request.sync_state();

  for (size_t i = 0; i < m_server_nodes.size(); ++i) {
    if (lstate != EN_RAFT_LSTATE_UNDEFINE && m_server_nodes[i].lstate() != lstate) {
      continue;
    }

    if (state != EN_SYNC_STATE_UNDEFINE && m_server_nodes[i].sync_state() != state) {
      continue;
    }
    node_list.push_back(m_server_nodes[i]);
  }

  AppInfoLog << "succ query all raft server node list ";
}


void RaftNode::resetNodes(std::vector<ServerNode>& new_node_list) {
  m_server_nodes.swap(new_node_list);
  if (new_node_list.empty()) {
    throw BusinessException(ERR_PARAM_INPUT, "sync node list is empty", __FILE__, __LINE__);
  }
  setPartitions(new_node_list[0].partition_count());
}

void RaftNode::setSelfId(int id) {
  m_self_id = id;
  const ServerNode& node = getServerNode(m_self_id);
  for (int i = 0; i < m_part_count; ++i) {
    m_partitions[i]->initNodeInfo(m_self_id, node.name(), node.addr());
  }
}

void RaftNode::setPartitions(int partition_count) {
  freePartitions();
  for (int i = 0; i < partition_count; ++i) {
    m_partitions.push_back(new RaftPartition());
  }
  m_part_count = partition_count;
}


void RaftNode::freePartitions() {
  for (size_t i = 0; i < m_partitions.size(); ++i) {
    free(m_partitions[i]);
    m_partitions[i] = NULL;
  }
  m_partitions.clear();
}

}