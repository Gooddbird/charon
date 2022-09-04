#include <google/protobuf/service.h>
#include "tinyrpc/comm/start.h"
#include "tinyrpc/net/tcp/io_thread.h"
#include "charon/server/service/charon.h"
#include "charon/raft/raft_node.h"

int main(int argc, char* argv[]) {

  tinyrpc::InitConfig("../conf/charon.xml");

  // tinyrpc::GetServer()->registerService(std::make_shared<charon::Charon>());
  tinyrpc::GetServer()->registerService(std::make_shared<charon::Raft>());
  auto cb = []() {

  };
  tinyrpc::GetServer()->getIOThreadPool()->addCoroutineToEachThread(cb);

  tinyrpc::StartRpcServer();
  
  return 0;
}