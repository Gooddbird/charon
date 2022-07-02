#include <google/protobuf/service.h>
#include "tinyrpc/comm/start.h"
#include "charon/server/service/charon.h"

int main(int argc, char* argv[]) {

  tinyrpc::InitConfig("../conf/charon.xml");

  tinyrpc::GetServer()->registerService(std::make_shared<charon::Charon>());
  tinyrpc::GetServer()->registerService(std::make_shared<charon::Charon>());

  tinyrpc::StartRpcServer();
  
  return 0;
}