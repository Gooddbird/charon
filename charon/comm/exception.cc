#include <exception>
#include <string>
#include "charon/comm/exception.h"


namespace charon {

CharonException::CharonException(long long code, const std::string& errinfo) : 
  m_error_code(code), m_error_info(errinfo) {

}

CharonException::~CharonException() {}

const char* CharonException::what() {
  return m_error_info.c_str();
}

std::string CharonException::error() {
  return m_error_info;
}

long long CharonException::code() {
  return m_error_code;
}

}