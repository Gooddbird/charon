#ifndef CHARON_SERVER_INTERFACE_ABSTRACT_INTERFACE_H
#define CHARON_SERVER_INTERFACE_ABSTRACT_INTERFACE_H

#include <google/protobuf/service.h>

namespace charon {

class AbstractInterface {
 public:
  AbstractInterface() = default;

  virtual ~AbstractInterface() = default;

  // abstract run function
  // pair.1 : ret_code, 0 -- succ, otherwise failed
  // pair.2 : res_info, OK -- succ, otherwise error details
  virtual void run(const google::protobuf::Message* request, google::protobuf::Message* response) = 0;

};


}



#endif