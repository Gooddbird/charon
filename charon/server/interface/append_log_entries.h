#ifndef CHARON_SERVER_INTERFACE_APPEND_LOG_ENTRIES_IMPL_H
#define CHARON_SERVER_INTERFACE_APPEND_LOG_ENTRIES_IMPL_H 

#include "charon/pb/raft.pb.h"
#include "charon/raft/raft_node.h"

namespace charon {

class AppendLogEntriesImpl {
 public:
  AppendLogEntriesImpl(const AppendLogEntriesRequest* request, AppendLogEntriesResponse* response);

  ~AppendLogEntriesImpl();

  void run();

  bool executeInCurrentThread();

  void checkInput();

  void execute();

  void checkTerm();

 private:
  const AppendLogEntriesRequest* m_request {NULL};
  AppendLogEntriesResponse* m_response {NULL};
  int m_current_thread_hash {-1};
  int m_to_thread_hash {-1};

};


}


#endif