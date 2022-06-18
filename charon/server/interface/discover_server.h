#ifndef CHARON_SERVER_INTERFACE_DISCOVER_SERVER_H
#define CHARON_SERVER_INTERFACE_DISCOVER_SERVER_H

#include "charon/pb/charon.pb.h"

namespace charon {

class DiscoverServerImpl {
 public:
  DiscoverServerImpl() = default;

  ~DiscoverServerImpl() = default;

  std::string findAddrByTag(const std::string& tag);

  void run(const DiscoverRequest* request, DiscoverResponse* response);
 private:
  std::string m_addr_value;
};


}


#endif