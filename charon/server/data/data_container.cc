
#include "charon/server/data/data_container.h"
#include "tinyrpc/net/timer.h"
#include "tinyrpc/net/reactor.h"
#include "tinyrpc/comm/log.h"


namespace charon {

DataContainer::DataContainer() {

}

DataContainer::~DataContainer() {

}

Node* DataContainer::getNode(const std::string& key) {
  auto it = m_db.find(key);
  if (it != m_db.end()) {
    return NULL;
  }
  return &(it->second);
}
  
void DataContainer::setNode(const std::string& key, const std::string& value, int64_t expire_time /*=0*/) {
  Node node;
  node.value = value;
  node.expire_time = expire_time;
  node.is_able = true;

  m_db[key.c_str()] = node;
  if(expire_time != 0) {
    auto callback = [this, key] () {
      auto it = this->m_db.find(key);
      if (it != m_db.end()) {
        it->second.is_able = false;
      }
      AppDebugLog << "this k-v has already expire, set disabled, key=" << key;
    };
    tinyrpc::TimerEvent::ptr event = std::make_shared<tinyrpc::TimerEvent>(expire_time, false, callback);
  }
}

bool DataContainer::isKeyExist(const std::string& key) {
  auto it = m_db.find(key);
  if (it == m_db.end() || it->second.is_able == false) {
    return false;
  }
  return true;
}


}