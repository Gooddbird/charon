#include <string>
#include <map>
#include "charon/server/data/node.h"
#include "tinyrpc/comm/log.h"

namespace charon {

static int g_node_index = 1;


Tree::Tree() {
  m_root = new PathNode();
  m_root->name = "/";
  m_root->parent_node = NULL;
  m_nodes[m_root->name.c_str()] = *m_root;
}

Tree::~Tree() {
  if (m_root) {
    delete m_root;
  }
}


bool Tree::addNode(const std::string& path) {
  if (path.empty() || path[0] != '/') {
    AppErrorLog << "create normal node [" << path << "] error, node path error format, please insert like /root/a/b ";
    return false;
  }

  auto it = m_nodes.find(path);
  if (it != m_nodes.end()) {
    AppErrorLog << "create normal node [" << path << "] error, this node is exist";
    return false;
  }

  PathNode node;
  node.name = path;

}

bool Tree::addOrderNode(const std::string& path, std::string& real_path) {

  auto it = m_order_node_index.find(path);
  int index = 1;
  if (it != m_order_node_index.end()) {
    index = it->second;
    it->second += 1;
  } else {
    m_order_node_index[path] = index + 1;
  }
  std::string key = path + std::to_string(index);

  PathNode node;
  node.name = path;
}


}