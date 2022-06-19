#ifndef CHARON_SERVER_INTERFACE_DISCOVER_SERVER_H
#define CHARON_SERVER_INTERFACE_DISCOVER_SERVER_H

#include "charon/pb/charon.pb.h"

namespace charon {

class DiscoverServerImpl {
 public:
  DiscoverServerImpl() = default;

  ~DiscoverServerImpl() = default;

  void run(const DiscoverRequest* request, DiscoverResponse* response);

  std::string findAddrByTag(const std::string& tag, std::string& re_tag);

  std::string getAddrFromSingleValue(const std::string& value, std::string& tag);

  std::string genTagString(const DiscoverTag& tag);

  DiscoverTag genDiscoverTag(const std::string& tag_str);

  std::string getTagByIndex(const std::string& tag_str, int index = 1);

  bool checkIPAddr(const std::string& addr, std::string& ip, int& port);

 private:
  std::string m_addr_value;

};


}


#endif