#ifndef CHARON_SERVER_INTERFACE_DISCOVER_SERVER_H
#define CHARON_SERVER_INTERFACE_DISCOVER_SERVER_H

#include "charon/pb/charon.pb.h"
#include "tinyrpc/comm/string_util.h"

namespace charon {

class DiscoverServerImpl {
 public:
  DiscoverServerImpl(const DiscoverRequest* request, DiscoverResponse* response);

  ~DiscoverServerImpl();

  void run();

  void checkInput();

  void execute();

 private:
  const DiscoverRequest* m_request {NULL};
  DiscoverResponse* m_response {NULL};

  std::string m_addr_value;

};


}


#endif