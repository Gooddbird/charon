#include <atomic>
#include <assert.h>
#include "charon/server/data/data_container.h"
#include "charon/server/data/data_manager.h"
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

int DataManager::getHashIndex(const std::string& key) {
  return ((int)(key[0]) + (int)key[key.length() - 1]) % m_size;
}


bool DataManager::getValue(const std::string& key, std::string& value) {
  if (key.empty()) {
    AppErrorLog << "get k-v error, key is empty()!";
    return false;
  }
  int cur_hash = GetThreadHash();
  int hash = getHashIndex(key);

  AppDebugLog << "key=" << key << ", current thread hash=" << cur_hash << ", data thread hash=" << hash;
  bool re = false;
  if (cur_hash == hash) {
    re = m_datalist[hash].getValue(key, value);
  } else {
    tinyrpc::Coroutine* cor = tinyrpc::Coroutine::GetCurrentCoroutine();
    auto resume = [cor]() {
      tinyrpc::Coroutine::Resume(cor);
    };

    auto func = [hash, this, key, &value, &re, resume, cur_hash] () {
      re = this->m_datalist[hash].getValue(key, value);
      tinyrpc::GetServer()->getIOThreadPool()->addTask(cur_hash, resume);
    };

    tinyrpc::GetServer()->getIOThreadPool()->addTask(hash, func);
    tinyrpc::Coroutine::Yield();
  }

  AppDebugLog << "get k-v success, key=" << key << ", v=" << value << ", hash io thread=" << hash;
  return re;

}

// expire_time, ms
// this key-value will be delete when expire_time arrive
void DataManager::setValue(const std::string& key, const std::string& value, int64_t expire_time /* = 0*/) {
  if (key.empty()) {
    AppErrorLog << "set k-v error, key is empty()!";
  }

  int cur_hash = GetThreadHash();
  int hash = getHashIndex(key);
  AppDebugLog << "key=" << key << ", current thread hash=" << cur_hash << ", data thread hash=" << hash;

  if (cur_hash == hash) {
    this->m_datalist[hash].setValue(key, value, expire_time);
  } else {
    tinyrpc::Coroutine* cor = tinyrpc::Coroutine::GetCurrentCoroutine();

    auto resume = [cor]() {
      tinyrpc::Coroutine::Resume(cor);
    };

    auto func = [hash, this, key, value, expire_time, resume, cur_hash] () {
      this->m_datalist[hash].setValue(key, value, expire_time);
      tinyrpc::GetServer()->getIOThreadPool()->addTask(cur_hash, resume);
    };

    tinyrpc::GetServer()->getIOThreadPool()->addTask(hash, func);
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

  int cur_hash = GetThreadHash();
  int hash = getHashIndex(key);
  AppDebugLog << "key=" << key << ", current thread hash=" << cur_hash << ", data thread hash=" << hash;
  bool re = false;
  if (cur_hash == hash) {
    re = this->m_datalist[hash].isKeyExist(key);
  } else {
    tinyrpc::Coroutine* cor = tinyrpc::Coroutine::GetCurrentCoroutine();

    auto resume = [cor]() {
      tinyrpc::Coroutine::Resume(cor);
    };

    auto func = [hash, this, key, resume, cur_hash, &re] () {
      re = this->m_datalist[hash].isKeyExist(key);
      tinyrpc::GetServer()->getIOThreadPool()->addTask(cur_hash, resume);
    };

    tinyrpc::GetServer()->getIOThreadPool()->addTask(hash, func);
    tinyrpc::Coroutine::Yield();
  }

  AppDebugLog << "key=" << key << " is not exist";
  return re;

}




}