#ifndef CHARON_SERVER_DATA_DATA_CONTAINER_H
#define CHARON_SERVER_DATA_DATA_CONTAINER_H

#include <map>
#include <string>
#include "charon/server/data/node.h"

namespace charon {

class DataContainer {
 public:


 public:
  // all key-values
  // key: servername://tag1/tag2/tag3
  std::map<std::string, Node> m_db;

};

};

#endif