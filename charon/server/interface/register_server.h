#ifndef CHARON_SERVER_INTERFACE_REGISTER_SERVER_H
#define CHARON_SERVER_INTERFACE_REGISTER_SERVER_H

#include "charon/pb/charon.pb.h"

namespace charon {

class RegisterServerImpl {
 public:
  RegisterServerImpl(const RegisterRequest* request, RegisterResponse* response);

  ~RegisterServerImpl();

  void run();

  void checkInput();

  void execute();

 private:
  const RegisterRequest* m_request {NULL};
  RegisterResponse* m_response {NULL};
  std::string m_addr_value;

};


}


#endif