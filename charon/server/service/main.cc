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
    srand(time(0));
    int i = rand() % (5) + 3;
    sleep(i);

    printf("sleep %d s end, now to askVote\n", i);
    charon::RaftNode* node = charon::RaftNode::GetRaftNode();
    node->askVote();
  };
  tinyrpc::GetServer()->getIOThreadPool()->addCoroutineToEachThread(cb);

  tinyrpc::StartRpcServer();
  
  return 0;
}