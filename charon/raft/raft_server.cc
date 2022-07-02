#include <vector>
#include "charon/raft/raft_server.h"


namespace charon {

static thread_local RaftServer* t_raft_server = nullptr;

RaftServer* RaftServer::GetRaftServer() {
  if (!t_raft_server) {
    return g_raft_server;
  }
  t_raft_server = new RaftServer();
  return t_raft_server;
}


RaftServer::FollewerToCandidate() {


}

}


