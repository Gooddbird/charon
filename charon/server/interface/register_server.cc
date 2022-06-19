#include "charon/server/interface/register_server.h"
#include "charon/pb/charon.pb.h"

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

}

void RegisterServerImpl::execute() {

}


}