#ifndef CHARON_SERVER_INTERFACE_DISCOVER_SERVER_H
#define CHARON_SERVER_INTERFACE_DISCOVER_SERVER_H

#include "charon/server/interface/abstract_interface.h"


namespace charon {

class DiscoverServerImpl : public AbstractInterface {
 public:
  DiscoverServerImpl() = default;

  ~DiscoverServerImpl() = default;

  void run(const google::protobuf::Message* request, google::protobuf::Message* response);

};


}


#endif