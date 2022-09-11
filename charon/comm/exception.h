#ifndef CHARON_COMM_EXCEPTION_H
#define CHARON_COMM_EXCEPTION_H

#include <exception>
#include <string>
#include <memory>
#include "tinyrpc/comm/log.h"


namespace charon {

class CharonException : public std::exception {
 public:

  CharonException(long long code, const std::string& err_info) {
    m_error_code = code;
    AppInfoLog << "throw CharonException: {code: " << m_error_code << ", errinfo:" << m_error_info << "}";
  }

  ~CharonException() {
  
  }

  const char* what() {
    return m_error_info.c_str();
  }

  std::string error() {
    return m_error_info;
  }

  long long code() {
    return m_error_code;
  }

 private:
  long long m_error_code {0};
  std::string m_error_info;

};

}

#endif