#include <exception>
#include <string>


namespace charon {

class CharonException : public std::exception {
 public:

  template<typename... Args>
  CharonException(long long code, const char* str, Args&&... args); 
  ~CharonException();

  const char* what();

  std::string error();

  long long code();

 private:
  long long m_error_code = 0;
  std::string m_error_info;

};

}