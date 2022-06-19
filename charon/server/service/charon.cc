#include <google/protobuf/service.h>
#include <exception>
#include "charon/server/service/charon.h"
#include "charon/server/interface/discover_server.h"
#include "charon/server/interface/register_server.h"
#include "charon/comm/exception.h"
#include "tinyrpc/comm/log.h"

#define CALL_CHARON_INTERFACE(type) \
  type impl(request, response); \
  response->set_ret_code(0); \
  response->set_res_info("OK"); \
  try { \
    impl.run(); \
  } catch (charon::CharonException& e) { \
    AppErrorLog << "occur CharonException, error code = " << e.code() << ", errinfo = " << e.error(); \
    response->set_ret_code(e.code()); \
    response->set_res_info(e.error()); \
  } catch (std::exception&) { \
    AppErrorLog << "occur std::exception, error code = -1, errorinfo = UnKnown error "; \
    response->set_ret_code(-1); \
    response->set_res_info("UnKnown error"); \
  } catch (...) { \
    AppErrorLog << "occur UnKnown exception, error code = -1, errorinfo = UnKnown error "; \
    response->set_ret_code(-1); \
    response->set_res_info("UnKnown error"); \
  } \
  if (done) { \
    done->Run(); \
  } \

namespace charon {

void Charon::DiscoverServer(::google::protobuf::RpcController* controller,
                      const ::DiscoverRequest* request,
                      ::DiscoverResponse* response,
                      ::google::protobuf::Closure* done) {

  CALL_CHARON_INTERFACE(DiscoverServerImpl);
}

void Charon::RegisterServer(::google::protobuf::RpcController* controller,
                      const ::RegisterRequest* request,
                      ::RegisterResponse* response,
                      ::google::protobuf::Closure* done) {

  CALL_CHARON_INTERFACE(RegisterServerImpl);
}


}