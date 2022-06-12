#ifndef CHARON_SERVER_DATA_NODE_H
#define CHARON_SERVER_DATA_NODE_H

#include <string>

namespace charon {

struct DataNode {
  // node score, decide the weight to get same name node 
  int score {0};

  // the last call server succ ?
  bool last_succ {true};
};

struct PathNode {
  // all path name
  std::string name;

  PathNode* parent_node {NULL};

  std::map<std::string, PathNode*> child_nodes;

  DataNode* data_node {NULL};
};


class Tree {

 public:
  Tree();

  ~Tree();

  bool addNode(const std::string& path);

  bool addOrderNode(const std::string& path, std::string& real_path);


 public:
  PathNode* m_root;

  std::map<std::string, PathNode> m_nodes;

  std::map<std::string, int> m_order_node_index;

};

}

#endif