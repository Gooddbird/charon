#ifndef CHARON_SERVER_DATA_DATA_MANAGER_H
#define CHARON_SERVER_DATA_DATA_MANAGER_H


#include <vector>
#include <map>
#include "charon/server/data/data_container.h"
#include "tinyrpc/comm/log.h"
#include "tinyrpc/net/tcp/io_thread.h"


namespace charon {

int GetThreadHash();

class DataManager {
 public:
  DataManager();

  ~DataManager();

  static DataManager* GetDataManager();

  int getHashIndex(const std::string& key);

  bool getValue(const std::string& key, std::string& value);

  Node* getNode(const std::string& key);

  // expire_time, ms
  // this key-value will be delete when expire_time arrive
  void setNode(const std::string& key, const std::string& value, int64_t expire_time = 0);

  bool isKeyExist(const std::string& key);

 public:
  std::vector<DataContainer> m_datalist;

 private:
  int m_size;
};


}


#endif