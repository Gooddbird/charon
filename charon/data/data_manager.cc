#include <atomic>
#include <assert.h>
#include "charon/data/data_container.h"
#include "charon/data/data_manager.h"
#include "tinyrpc/comm/log.h"
#include "tinyrpc/comm/start.h"
#include "tinyrpc/net/tcp/io_thread.h"
#include "tinyrpc/net/reactor.h"

namespace charon {

static std::atomic<int> g_hash_index {0};

static thread_local int t_current_hash = -1;

static DataManager* g_data_manager = nullptr;

int GetThreadHash() {
  if (t_current_hash == -1) {
    t_current_hash = g_hash_index;
    g_hash_index++;
  }
  return t_current_hash;
}


DataManager* DataManager::GetDataManager() {
  if (g_data_manager != nullptr) {
    return g_data_manager;
  }
  g_data_manager = new DataManager();
  assert(g_data_manager != nullptr);
  return g_data_manager;
}

DataManager::DataManager() {
  m_size = tinyrpc::GetServer()->getIOThreadPool()->getIOThreadPoolSize();
  for (int i = 0; i < m_size; ++i) {
    m_datalist.push_back(DataContainer());
  }
}

DataManager::~DataManager() {

}

int DataManager::GetHashIndexOfKey(const std::string& key) {
  return ((int)(key[0]) + (int)key[key.length() - 1]) % tinyrpc::GetIOThreadPoolSize();
}

bool DataManager::CheckKeyHash(const std::string& key, int& cur_hash, int& to_hash) {
  cur_hash = tinyrpc::IOThread::GetCurrentIOThread()->getThreadIndex();
  to_hash = GetHashIndexOfKey(key);
  return cur_hash == to_hash;
}

bool DataManager::getValue(const std::string& key, std::string& value) {
  Node* node = getNode(key);
  if (node && node->is_able) {
    value = node->value;
    AppDebugLog << "get k-v success, key=" << key << ", v=" << value;
    return true;
  }
  AppDebugLog << "get k-v faled, key=" << key;
  return false;
}

Node* DataManager::getNode(const std::string& key) {
  if (key.empty()) {
    AppErrorLog << "get k-v error, key is empty()!";
    return NULL;
  }
  Node* re = NULL;
  int cur_hash = 0;
  int hash = 0;

  bool flag = CheckKeyHash(key, cur_hash, hash);

  AppDebugLog << "key=" << key << ", current thread hash=" << cur_hash << ", data thread hash=" << hash;
  if (flag) {
    re = m_datalist[hash].getNode(key);
  } else {
    tinyrpc::Coroutine* cor = tinyrpc::Coroutine::GetCurrentCoroutine();
    auto resume = [cor]() {
      tinyrpc::Coroutine::Resume(cor);
    };

    auto func = [hash, this, key, &re, resume, cur_hash] () {
      re = this->m_datalist[hash].getNode(key);
      tinyrpc::GetServer()->getIOThreadPool()->addTaskByIndex(cur_hash, resume);
    };

    tinyrpc::GetServer()->getIOThreadPool()->addTaskByIndex(hash, func);
    tinyrpc::Coroutine::Yield();
  }

  AppDebugLog << "get Node success, key=" << key << ", hash io thread=" << hash;
  return re;

}

// expire_time, ms
// this key-value will be delete when expire_time arrive
void DataManager::setNode(const std::string& key, const std::string& value, int64_t expire_time /* = 0*/) {
  if (key.empty()) {
    AppErrorLog << "set k-v error, key is empty()!";
  }

  int cur_hash = 0;
  int hash = 0;

  bool flag = CheckKeyHash(key, cur_hash, hash);

  AppDebugLog << "key=" << key << ", current thread hash=" << cur_hash << ", data thread hash=" << hash;

  if (flag) {
    this->m_datalist[hash].setNode(key, value, expire_time);
  } else {
    tinyrpc::Coroutine* cor = tinyrpc::Coroutine::GetCurrentCoroutine();

    auto resume = [cor]() {
      tinyrpc::Coroutine::Resume(cor);
    };

    auto func = [hash, this, key, value, expire_time, resume, cur_hash] () {
      this->m_datalist[hash].setNode(key, value, expire_time);
      tinyrpc::GetServer()->getIOThreadPool()->addTaskByIndex(cur_hash, resume);
    };

    tinyrpc::GetServer()->getIOThreadPool()->addTaskByIndex(hash, func);
    tinyrpc::Coroutine::Yield();
  }

  if (expire_time == 0) {
    AppDebugLog << "get k-v success, key=" << key << ", v=" << value << ", hash io thread=" << hash;
  } else {
    AppDebugLog << "get k-v success, key=" << key << ", v=" << value << ", expire_time=" << expire_time <<", hash io thread=" << hash;
  }
}

bool DataManager::isKeyExist(const std::string& key) {
  if (key.empty()) {
    AppErrorLog << "get k-v error, key is empty()!";
    return false;
  }

  int cur_hash = 0;
  int hash = 0;

  bool flag = CheckKeyHash(key, cur_hash, hash);

  AppDebugLog << "key=" << key << ", current thread hash=" << cur_hash << ", data thread hash=" << hash;
  bool re = false;
  if (flag) {
    re = this->m_datalist[hash].isKeyExist(key);
  } else {
    tinyrpc::Coroutine* cor = tinyrpc::Coroutine::GetCurrentCoroutine();

    auto resume = [cor]() {
      tinyrpc::Coroutine::Resume(cor);
    };

    auto func = [hash, this, key, resume, cur_hash, &re] () {
      re = this->m_datalist[hash].isKeyExist(key);
      tinyrpc::GetServer()->getIOThreadPool()->addTaskByIndex(cur_hash, resume);
    };

    tinyrpc::GetServer()->getIOThreadPool()->addTaskByIndex(hash, func);
    tinyrpc::Coroutine::Yield();
  }

  AppDebugLog << "key=" << key << " is not exist";
  return re;

}




}