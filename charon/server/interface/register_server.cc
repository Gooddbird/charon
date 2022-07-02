#include "charon/server/interface/register_server.h"
#include "charon/pb/charon.pb.h"
#include "charon/server/data/server_addr_util.h"
#include "charon/comm/errcode.h"
#include "charon/comm/exception.h"

namespace charon {

RegisterServerImpl::RegisterServerImpl(const RegisterRequest* request, RegisterResponse* response)
  : m_request(request), m_response(response) {

}

RegisterServerImpl::~RegisterServerImpl() {


}


void RegisterServerImpl::run() {

  checkInput();

  execute();

}

void RegisterServerImpl::checkInput() {
  if (m_request->server_name().empty()) {
    throw CharonException(ERROR_KEY_EMPTY, "failed to register server, server name is empty");
  }
  if (m_request->addr().empty()) {
    throw CharonException(ERROR_REGISTER_SERVER_ADDR_EMPTY, "failed to discover server, server addr is empty");
  }
  if (!ServerAddrUtil::CheckDisvcoverTagValid(m_request->tag())) {
    throw CharonException(ERROR_REGISTER_SERVER_TAG_INVALID, "failed to discover server, server addr is empty");
  }

}

void RegisterServerImpl::execute() {

}


}