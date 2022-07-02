#include "tinyrpc/net/tcp/io_thread.h"
#include "tinyrpc/comm/start.h"
#include "charon/pb/raft.pb.h"
#include "charon/server/interface/append_log_entries.h"
#include "charon/server/data/data_manager.h"
#include "charon/comm/errcode.h"
#include "charon/comm/exception.h"

namespace charon {

AppendLogEntriesImpl::AppendLogEntriesImpl(const AppendLogEntriesRequest* request, AppendLogEntriesResponse* response) {

}

AppendLogEntriesImpl::~AppendLogEntriesImpl() {

}

void AppendLogEntriesImpl::run() {

  checkInput();

  execute();

}

bool AppendLogEntriesImpl::executeInCurrentThread() {
  m_current_thread_hash = tinyrpc::IOThread::GetCurrentIOThread()->getThreadIndex();
  m_to_thread_hash = m_request->thread_hash();
  return m_current_thread_hash == m_to_thread_hash;
}

void AppendLogEntriesImpl::checkInput() {
  int pool_size = tinyrpc::GetIOThreadPoolSize();
  if (m_to_thread_hash >= pool_size || m_to_thread_hash < 0) {
    throw CharonException(ERROR_INVALID_THREAD_HASH, "invalid thread hash {%d}", m_request->thread_hash());
  }
}

void AppendLogEntriesImpl::execute() {
  if (executeInCurrentThread()) {

  } else {

  }
}


void AppendLogEntriesImpl::checkTerm() {

}

}