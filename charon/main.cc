#include <google/protobuf/service.h>
#include "tinyrpc/comm/start.h"
#include "tinyrpc/net/tcp/io_thread.h"
#include "charon/server/service/charon.h"
#include "charon/raft/raft_node.h"

int main(int argc, char* argv[]) {
  if (argc != 2) {
    printf("Start charon server error, input argc is not 2!");
    printf("Start charon server like this: \n");
    printf("./charon xxx.xml\n");
    return 0;
  }

  tinyrpc::InitConfig(argv[1]);

  REGISTER_SERVICE(charon::Raft);

  auto cb = []() {
    charon::RaftNodeContainer::GetRaftNodeContainer()->getRaftNode(0)->resetElectionTimer();
  };
  tinyrpc::GetServer()->getIOThreadPool()->addCoroutineToEachThread(cb);

  tinyrpc::StartRpcServer();
  
  return 0;
}