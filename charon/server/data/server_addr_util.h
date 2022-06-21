#ifndef CHARON_SERVER_DATA_SERVER_ADDR_UTIL_H
#define CHARON_SERVER_DATA_SERVER_ADDR_UTIL_H

#include <map>
#include <string>
#include <vector>
#include "charon/pb/charon.pb.h"

namespace charon {

class ServerAddrUtil {
 public:

  /**
   **
   ** //tag1/tag2/tag3\t score \t 192.1.1.1:80|//tag1/tag2/tag3 \t score \t 192.1.1.2:81
   **
   **/
  ServerAddrUtil() = default;

  ~ServerAddrUtil() = default;

  static std::string GetAddrByTag(const std::string& value, const DiscoverTag& req_tag, DiscoverTag& res_tag);

  static std::string GetAddrFromSingleValue(const std::string& value, DiscoverTag& res_tag);

  static std::string GetTagString(const DiscoverTag& tag);

  static DiscoverTag GetDiscoverTagByString(const std::string& tag_str);

  static bool GetIPPortFromAddr(const std::string& addr, std::string& ip, int& port);

  /** 
   ** Charon support server name rule:
   ** 1. can't contain these special charater: ' '(space) , '*', '/', '|'
   ** 2. can't empty
   **/
  static bool CheckServerNameValid(const std::string& name);


  /** 
  ** Charon support Discover tag rule:
  ** 1. can't contain this special charater: ' '(space) , '*', '/', '|'
  ** 2. if you set tag, you must set higher tag. for excample, if you set tag2 is'not empty, you must sure tag1 is seted
  **/
  static bool CheckDisvcoverTagValid(const DiscoverTag& tag);

  static bool CheckDisvcoverTagEmpty(const DiscoverTag& tag);

 private:
  static std::string GetRandomAddr(std::vector<std::string>& addr_list, DiscoverTag& res_tag);
};

}

#endif