#include <exception>
#include <string>
#include "charon/comm/exception.h"
#include "tinyrpc/comm/log.h"


namespace charon {

template<typename... Args>
CharonException::CharonException(long long code, const char* str, Args&&... args) {
  m_error_code = code;
  sprintf(m_error_info.c_str(), str, std::forward<Args>(args)...);
  AppInfoLog << "throw CharonException: {code: " << m_error_code <<", errinfo:" << m_error_info << "}"; 
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