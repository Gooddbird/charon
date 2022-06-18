#ifndef CHARON_SERVER_DISCOVER_SERVER_H
#define CHARON_SERVER_DISCOVER_SERVER_H

#include <google/protobuf/service.h>
#include "charon/comm/exception.h"
#include "charon/comm/errcode.h"
#include "charon/server/interface/discover_server.h"
#include "charon/server/data/data_manager.h"
#include "charon/pb/charon.pb.h"

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

  DiscoverTag tag = request->tag();

  std::string tags = tag.tag1() + tag.tag2() + tag.tag3();

  return findAddrByTag(tag);

}

std::string DiscoverServerImpl::findAddrByTag(const std::string& tag) {

  if (tag.empty()) {

  }
}

}


#endif