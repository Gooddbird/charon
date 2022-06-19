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


std::string ServerAddrUtil::GetAddrByTag(const std::string& value, const std::string& tag, std::string& re_tag) {
  std::vector<std::string> addr_list;
  tinyrpc::StringUtil::SplitStrToVector(value, "|", addr_list);
  if (addr_list.empty() || addr_list.size() == 1) {
    return "";
  }

  if (tag == "* * *") {
    std::string re;
    int n = 5;
    while(re.empty() && n--) {
      srand(time(0));
      int index = rand()%(addr_list.size() - 1);  
      re = GetAddrFromSingleValue(addr_list[index + 1], re_tag); 
    }
    return re;
    // rand back an addr when not set tag
  }
  return "";
}

std::string ServerAddrUtil::GetAddrFromSingleValue(const std::string& value, std::string& tag) {
  size_t i = value.find_last_of("/");
  if (i >= value.length()) {
    AppErrorLog << "failed to discover server, this addr conf is bad:" << value;
    return "";
  }
  size_t j = value.find_first_of("/");
  if (j >= value.length()) {
    AppErrorLog << "failed to discover server, this addr conf is bad:" << value;
    return "";
  }
  tag = value.substr(0, j);
  return value.substr(i + 1, value.length() - i - 2);
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
  std::string re;
  if (!tag.tag1().empty()) {
    re += tag.tag1();
  } else {
    re += "*";
  }
  re += " ";

  if (!tag.tag2().empty()) {
    re += tag.tag2();
  } else {
    re += "*";
  }
  re += " ";

  if (!tag.tag3().empty()) {
    re += tag.tag3();
  } else {
    re += "*";
  }

  return re;
}


DiscoverTag ServerAddrUtil::GetDiscoverTagByString(const std::string& tag_str) {
  DiscoverTag tag;
  if (tag_str.empty()) {
    return tag;
  }

  std::string tmp(std::move(tag_str));

  size_t i = tmp.find_first_of(" ");
  tag.set_tag1(tmp.substr(0, i));
  tmp = tmp.substr(i + 1, tmp.length() - i - 2);

  i = tmp.find_first_of(" ");
  tag.set_tag2(tmp.substr(0, i));

  tag.set_tag3(tmp.substr(i + 1, tmp.length() - i - 2));
  return tag;

}

std::string ServerAddrUtil::GetTagByIndex(const std::string& tag_str, int index /*=1*/) {
  std::string tmp(std::move(tag_str));
  if (index == 1) {
    size_t i = tmp.find_first_of(" ");
    return tmp.substr(0, i);
  } else if (index == 2) {
    size_t i = tmp.find_first_of(" ");
    size_t j = tmp.find_last_of(" ");
    return tmp.substr(i + 1, j - 1 - i);
  } else if (index == 3) {
    size_t i = tmp.find_last_of(" ");
    return tmp.substr(i + 1, tmp.length() - i - 2);
  } else {
    return "";
  }
}

bool ServerAddrUtil::CheckDisvcoverTagValid(const DiscoverTag& tag) {
  std::string tmp = tag.tag1() + tag.tag2() + tag.tag3();
  CHECK_STR_IN_STRING(tmp, " ");
  CHECK_STR_IN_STRING(tmp, "/");
  CHECK_STR_IN_STRING(tmp, "|");

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
  CHECK_STR_IN_STRING(name, "*");
  CHECK_STR_IN_STRING(name, "/");
  CHECK_STR_IN_STRING(name, "|");

  return true;
}

}