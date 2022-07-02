#ifndef CHARON_SERVER_DATA_DATA_CONTAINER_H
#define CHARON_SERVER_DATA_DATA_CONTAINER_H

#include <map>
#include <string>
#include "tinyrpc/net/mutex.h"

namespace charon {

struct Node {
  std::string value;
  int64_t expire_time {0};
  bool is_able {true};
};

class DataContainer {
 public:
  DataContainer();

  ~DataContainer();

  bool getValue(const std::string& key, std::string& value);

  Node* getNode(const std::string& key);
  
  // expire_time, ms
  // this key-value will be delete when expire_time arrive
  void setNode(const std::string& key, const std::string& value, int64_t expire_time = 0);

  bool isKeyExist(const std::string& key);

 public:
  // all key-values
  std::map<std::string, Node> m_db;
  tinyrpc::CoroutineMutex m_cor_mutex; 
};

}

#endif