#include <google/protobuf/service.h>
#include <sstream>
#include <atomic>
#include "tinyrpc/comm/start.h"
#include "charon/server/service/charon.h"

int main(int argc, char* argv[]) {
  if (argc != 2) {
    printf("Start charon server error, input argc is not 2!");
    printf("Start charon server like this: \n");
    printf("./charon a.xml\n");
    return 0;
  }

  tinyrpc::InitConfig(argv[1]);

  tinyrpc::GetServer()->registerService(std::make_shared<charon::Charon>());

  tinyrpc::StartRpcServer();
  
  return 0;
}