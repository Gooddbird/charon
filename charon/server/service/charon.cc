#include <google/protobuf/service.h>
#include "charon/server/service/charon.h"
#include "charon/server/interface/discover_server.h"

#define CALL_CHARON_INTERFACE(name) \
  name impl; \
  impl.run(static_cast<const google::protobuf::Message*>(request), static_cast<google::protobuf::Message*>(response)); \
  done->Run();  \

namespace charon {

void Charon::DiscoverServer(::google::protobuf::RpcController* controller,
                      const ::DiscoverRequest* request,
                      ::DiscoverResponse* response,
                      ::google::protobuf::Closure* done) {

  CALL_CHARON_INTERFACE(DiscoverServerImpl);
}

void Charon::RegisterServer(::google::protobuf::RpcController* controller,
                      const ::RegisterRequest* request,
                      ::RegisterResponse* response,
                      ::google::protobuf::Closure* done) {


}


}