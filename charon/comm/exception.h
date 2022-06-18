#include <exception>
#include <string>


namespace charon {

class CharonException : public std::exception {
 public:

  CharonException(long long code, const std::string& errinfo) : m_error_code(code), m_error_info(errinfo) {}

  ~CharonException() {}

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
  long long m_error_code = 0;
  std::string m_error_info;

};

}