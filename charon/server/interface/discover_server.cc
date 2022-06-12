#ifndef CHARON_SERVER_DISCOVER_SERVER_H
#define CHARON_SERVER_DISCOVER_SERVER_H

#include <google/protobuf/service.h>
#include "charon/server/interface/discover_server.h"
#include "charon/pb/charon.pb.h"

namespace charon {

void DiscoverServerImpl::run(const google::protobuf::Message* request, google::protobuf::Message* response) {

  const DiscoverRequest* req = dynamic_cast<const DiscoverRequest*>(request);
  DiscoverResponse* res = dynamic_cast<DiscoverResponse*>(response);


}


}


#endif