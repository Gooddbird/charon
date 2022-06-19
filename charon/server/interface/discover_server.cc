#ifndef CHARON_SERVER_DISCOVER_SERVER_H
#define CHARON_SERVER_DISCOVER_SERVER_H

#include <google/protobuf/service.h>
#include <time.h>
#include <stdlib.h>
#include "charon/comm/exception.h"
#include "charon/comm/errcode.h"
#include "charon/server/interface/discover_server.h"
#include "charon/server/data/data_manager.h"
#include "charon/pb/charon.pb.h"
#include "tinyrpc/comm/string_util.h"

namespace charon {

void DiscoverServerImpl::run(const DiscoverRequest* request, DiscoverResponse* response) {
  std::string key = request->server_name();
  if (key.empty()) {
    throw CharonException(ERROR_KEY_EMPTY, "failed to discover server, server name is empty");
  }

  std::string value;
  bool re = DataManager::GetDataManager()->getValue(key, value);
  if (!re) {
    throw CharonException(ERROR_SERVER_ADDR_EMPTY, "faild to discover server, server don't have able addr");
  }
  if (value.empty()) {
    throw CharonException(ERROR_SERVER_ADDR_EMPTY, "faild to discover server, server don't have able addr");
  }

  m_addr_value = std::move(value);

  std::string tagstr = genTagString(request->tag());
  std::string re_tag_str;

  std::string addr = findAddrByTag(tagstr, re_tag_str);
  if (addr.empty()) {
    throw CharonException(ERROR_SERVER_ADDR_EMPTY, "faild to discover server, server don't have able addr");
  }
  std::string ip;
  int port = 0;

  if(!checkIPAddr(addr, ip, port)) {
    throw CharonException(ERROR_SERVER_ADDR_INVALID, "faild to discover server, server addr is invalid");
  }

  response->set_addr(addr);
  response->set_ip(ip);
  response->set_port(port);
  response->mutable_tag()->set_tag1(getTagByIndex(re_tag_str, 1));
  response->mutable_tag()->set_tag2(getTagByIndex(re_tag_str, 2));
  response->mutable_tag()->set_tag3(getTagByIndex(re_tag_str, 3));

  AppInfoLog << "discover server success, " << "servername=" << key << ", req tags=" << tagstr << ", res addr=" << addr << ", res tags=" << re_tag_str;

}

std::string DiscoverServerImpl::findAddrByTag(const std::string& tag, std::string& re_tag) {
  std::vector<std::string> addr_list;
  tinyrpc::StringUtil::SplitStrToVector(m_addr_value, "|", addr_list);
  if (addr_list.empty() || addr_list.size() == 1) {
    throw CharonException(ERROR_SERVER_ADDR_EMPTY, "faild to discover server, server don't have able addr");
  }

  if (tag == "* * *") {
    std::string re;
    int n = 5;
    while(re.empty() && n--) {
      srand(time(0));
      int index = rand()%(addr_list.size() - 1);  
      re = getAddrFromSingleValue(addr_list[index + 1], re_tag); 
    }
    return re;
    // rand back an addr when not set tag
  }
  return "";
}

std::string DiscoverServerImpl::getAddrFromSingleValue(const std::string& value, std::string& tag) {
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

bool DiscoverServerImpl::checkIPAddr(const std::string& addr, std::string& ip, int& port) {
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

std::string DiscoverServerImpl::genTagString(const DiscoverTag& tag) {
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


DiscoverTag DiscoverServerImpl::genDiscoverTag(const std::string& tag_str) {
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

std::string DiscoverServerImpl::getTagByIndex(const std::string& tag_str, int index /*=1*/) {
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

}


#endif