#ifndef CHARON_SERVER_CHARON_H
#define CHARON_SERVER_CHARON_H

#include <google/protobuf/service.h>
#include "charon/pb/charon.pb.h"

namespace charon {

class Charon : public CharonService {

 public:
  Charon() = default;

  ~Charon() = default;

  void DiscoverServer(::google::protobuf::RpcController* controller,
                       const ::DiscoverRequest* request,
                       ::DiscoverResponse* response,
                       ::google::protobuf::Closure* done);
  
  void RegisterServer(::google::protobuf::RpcController* controller,
                       const ::RegisterRequest* request,
                       ::RegisterResponse* response,
                       ::google::protobuf::Closure* done);
};


}


#endif