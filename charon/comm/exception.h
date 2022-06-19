#include <exception>
#include <string>


namespace charon {

class CharonException : public std::exception {
 public:

  CharonException(long long code, const std::string& errinfo);
  ~CharonException();

  const char* what();

  std::string error();

  long long code();

 private:
  long long m_error_code = 0;
  std::string m_error_info;

};

}