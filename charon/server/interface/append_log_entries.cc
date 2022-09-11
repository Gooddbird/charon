#include "tinyrpc/net/tcp/io_thread.h"
#include "tinyrpc/comm/start.h"
#include "charon/pb/raft.pb.h"
#include "charon/server/interface/append_log_entries.h"
#include "charon/server/data/data_manager.h"
#include "charon/comm/errcode.h"
#include "charon/comm/exception.h"
#include "charon/raft/raft_node.h"

namespace charon {

AppendLogEntriesImpl::AppendLogEntriesImpl(const AppendLogEntriesRequest* request, AppendLogEntriesResponse* response) {

}

AppendLogEntriesImpl::~AppendLogEntriesImpl() {

}

void AppendLogEntriesImpl::run() {

  checkInput();

  execute();

}


void AppendLogEntriesImpl::checkInput() {

}

void AppendLogEntriesImpl::execute() {
  RaftNodeContainer::GetRaftNodeContainer()->getRaftNode(0)->handleAppendLogEntries(*m_request, *m_response);
}


}