#include <google/protobuf/service.h>
#include "charon/server/service/charon.h"

namespace charon {

void Charon::DiscoverServer(::google::protobuf::RpcController* controller,
                      const ::DiscoverRequest* request,
                      ::DiscoverResponse* response,
                      ::google::protobuf::Closure* done) {


}

void Charon::RegisterServer(::google::protobuf::RpcController* controller,
                      const ::RegisterRequest* request,
                      ::RegisterResponse* response,
                      ::google::protobuf::Closure* done) {


}


}