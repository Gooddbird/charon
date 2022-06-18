#ifndef CHARON_SERVER_DATA_DATA_CONTAINER_H
#define CHARON_SERVER_DATA_DATA_CONTAINER_H

#include <map>
#include <string>

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
  
  // expire_time, ms
  // this key-value will be delete when expire_time arrive
  void setValue(const std::string& key, const std::string& value, int64_t expire_time = 0);

  bool isKeyExist(const std::string& key);

 public:
  // all key-values
  std::map<std::string, Node> m_db;
};

}

#endif