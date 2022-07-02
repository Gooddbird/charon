#include <map>
#include <string>
#include "charon/pb/charon.pb.h"
#include "charon/server/data/server_addr_util.h"
#include "tinyrpc/comm/log.h"
#include "tinyrpc/comm/string_util.h"

namespace charon {

#define CHECK_STR_IN_STRING(str, a) \
  if (str.find(a) != str.npos) { \
    return false; \
  } \




  /**
   **
   ** //tag1/tag2/tag3\t score \t 192.1.1.1:80|//tag1/tag2/tag3 \t score \t 192.1.1.2:81
   **
   **/

std::string ServerAddrUtil::GetAddrByTag(const std::string& value, const DiscoverTag& req_tag, DiscoverTag& res_tag) {


  if (!CheckDisvcoverTagValid(req_tag)) {
    AppErrorLog << "failed to discover server, invalid request tag format:" << req_tag.ShortDebugString(); 
  }


  //TODO: load balance, pending to finish
  if (CheckDisvcoverTagEmpty(req_tag)) {
    // rand back an addr when not set tag
    std::vector<std::string> addr_list;
    tinyrpc::StringUtil::SplitStrToVector(value, "|", addr_list);

    return GetRandomAddr(addr_list, res_tag);

  } else {
    std::string tag_str = GetTagString(req_tag);
 
    size_t begin = value.find_first_of(tag_str);
    size_t end = value.find_last_of(tag_str);
    if (begin == value.npos || end == value.npos) {
      AppErrorLog << "failed to discover server, can't find tag [" << req_tag.ShortDebugString() << "]";
      return "";
    }
    std::string tmp = value.substr(end, value.length() - end);
    size_t i = tmp.find_first_of("|");
    std::string addr_str;
    if (i == tmp.npos) {
      addr_str = value.substr(begin, value.length() - begin);
    } else {
      addr_str = value.substr(begin, end - begin + i - 3);
    }

    std::vector<std::string> addr_list;
    tinyrpc::StringUtil::SplitStrToVector(addr_str, "|", addr_list);
    return GetRandomAddr(addr_list, res_tag);

  }
  
}

std::string ServerAddrUtil::GetRandomAddr(std::vector<std::string>& addr_list, DiscoverTag& res_tag) {

  if (addr_list.empty()) {
    return "";
  }

  std::string re;
  int n = 5;
  while(re.empty() && n--) {
    srand(time(0));
    int index = rand()%(addr_list.size() - 1);  
    re = GetAddrFromSingleValue(addr_list[index + 1], res_tag); 
  }
  return re;

}

std::string ServerAddrUtil::GetAddrFromSingleValue(const std::string& value, DiscoverTag& res_tag) {
  size_t i = value.find_last_of("\t");
  if (i >= value.length()) {
    AppErrorLog << "failed to discover server, this addr conf is bad:" << value;
    return "";
  }
  size_t j = value.find_first_of("\t");
  if (j >= value.length()) {
    AppErrorLog << "failed to discover server, this addr conf is bad:" << value;
    return "";
  }
  std::string tag = value.substr(0, j);
  res_tag = GetDiscoverTagByString(tag);
  
  return value.substr(i + 1, value.length() - i - 1);
}

bool ServerAddrUtil::GetIPPortFromAddr(const std::string& addr, std::string& ip, int& port) {
  size_t i = addr.find_first_of(":");
  ip = addr.substr(0, i);
  std::string port_str = addr.substr(i + 1, addr.length() - i - 2);
  int port_i = std::atoi(port_str.c_str());
  if (port_i == 0) {
    AppErrorLog << "invalid ip address:" << addr;
    return false;
  }
  port = port_i;
  return true;
}

std::string ServerAddrUtil::GetTagString(const DiscoverTag& tag) {
  // tags string like: tag1 tag2 tag3, if tag is empty, it will elplaced by '*'
  // for example, if all tag is empty, it will be : * * *
  if (!CheckDisvcoverTagValid(tag)) {
    return "";
  }
  if (tag.tag1().empty()) {
    return "";
  } 
  std::string re = "//";
  re += tag.tag1();

  if (tag.tag2().empty()) {
    return re;
  } 
  re += "/";
  re += tag.tag2();

  if (tag.tag3().empty()) {
    return re;
  }
  re += "/";
  re += tag.tag3();

  return re;
}


DiscoverTag ServerAddrUtil::GetDiscoverTagByString(const std::string& tag_str) {
  DiscoverTag tag;
  if (tag_str.empty()) {
    return tag;
  }

  std::string tmp(std::move(tag_str));

  size_t i = tmp.find_first_of("//");
  if (i == tag_str.npos || tmp.length() <= 2) {
    return tag;
  }
  tmp = tmp.substr(i + 2, tmp.length() - i - 2);
  i = tmp.find_first_of("/");
  if (i == tmp.npos) {
    tag.set_tag1(tmp);
    return tag;
  }
  tag.set_tag1(tmp.substr(0, i));
  tmp = tmp.substr(i + 1, tmp.length() - i - 1);
  i = tmp.find_first_of("/");

  if (i == tmp.npos) {
    tag.set_tag2(tmp);
    return tag;
  }
  tag.set_tag2(tmp.substr(0, i));
  tag.set_tag3(tmp.substr(i + 1, tmp.length() - i - 1));

  return tag;

}

bool ServerAddrUtil::CheckDisvcoverTagValid(const DiscoverTag& tag) {
  std::string tmp = tag.tag1() + tag.tag2() + tag.tag3();
  if (tmp.empty()) {
    return true;
  }

  CHECK_STR_IN_STRING(tmp, " ");
  CHECK_STR_IN_STRING(tmp, "/");
  CHECK_STR_IN_STRING(tmp, "|");
  CHECK_STR_IN_STRING(tmp, "\t");

  if (!tag.tag2().empty()) {
    if (tag.tag1().empty()) {
      return false;
    }
  }

  if (!tag.tag3().empty()) {
    if (tag.tag2().empty()) {
      return false;
    }
  }

  return true;

}

/** 
 ** Charon support server name that can't contain these special charater
  ** ' '(space) , '*', '/'
  **/
bool ServerAddrUtil::CheckServerNameValid(const std::string& name) {
  if(name.empty()) {
    return false;
  }

  CHECK_STR_IN_STRING(name, " ");
  CHECK_STR_IN_STRING(name, "/");
  CHECK_STR_IN_STRING(name, "|");
  CHECK_STR_IN_STRING(name, "\t");

  return true;
}



bool ServerAddrUtil::CheckDisvcoverTagEmpty(const DiscoverTag& tag) {
  return tag.tag1().empty() && tag.tag2().empty() && tag.tag3().empty();
}

}