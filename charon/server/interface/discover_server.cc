#ifndef CHARON_SERVER_DISCOVER_SERVER_H
#define CHARON_SERVER_DISCOVER_SERVER_H

#include <google/protobuf/service.h>
#include <time.h>
#include <stdlib.h>
#include "charon/comm/exception.h"
#include "charon/comm/errcode.h"
#include "charon/server/interface/discover_server.h"
#include "charon/server/data/data_manager.h"
#include "charon/server/data/server_addr_util.h"
#include "charon/pb/charon.pb.h"


namespace charon {

DiscoverServerImpl::DiscoverServerImpl(const DiscoverRequest* request, DiscoverResponse* response) 
  : m_request(request), m_response(response) {

}

DiscoverServerImpl::~DiscoverServerImpl() {

}


void DiscoverServerImpl::run() {

  checkInput();

  execute();

}

void DiscoverServerImpl::checkInput() {
  if (m_request->server_name().empty()) {
    throw CharonException(ERROR_KEY_EMPTY, "failed to discover server, server name is empty");
  }
}


void DiscoverServerImpl::execute() {
  std::string value;
  bool re = DataManager::GetDataManager()->getValue(m_request->server_name(), value);
  if (!re) {
    throw CharonException(ERROR_SERVER_ADDR_EMPTY, "faild to discover server, server don't have able addr");
  }
  if (value.empty()) {
    throw CharonException(ERROR_SERVER_ADDR_EMPTY, "faild to discover server, server don't have able addr");
  }

  m_addr_value = std::move(value);

  std::string tagstr = ServerAddrUtil::GetTagString(m_request->tag());
  std::string res_tag_str;
  DiscoverTag res_tag;

  std::string addr = ServerAddrUtil::GetAddrByTag(m_addr_value, m_request->tag(), res_tag);
  if (addr.empty()) {
    throw CharonException(ERROR_SERVER_ADDR_EMPTY, "faild to discover server, server don't have able addr");
  }
  std::string ip;
  int port = 0;

  if(!ServerAddrUtil::GetIPPortFromAddr(addr, ip, port)) {
    throw CharonException(ERROR_SERVER_ADDR_INVALID, "faild to discover server, server addr is invalid");
  }

  m_response->set_addr(addr);
  m_response->set_ip(ip);
  m_response->set_port(port);
  m_response->mutable_tag()->set_tag1(res_tag.tag1());
  m_response->mutable_tag()->set_tag2(res_tag.tag2());
  m_response->mutable_tag()->set_tag3(res_tag.tag3());

  AppInfoLog << "discover server success, " << "servername=" << m_request->server_name() << ", req tags=" << tagstr << ", res addr=" << addr << ", res tags=" << res_tag_str;
}

}


#endif