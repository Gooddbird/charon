#ifndef CHARON_COMM_UTIL_H
#define CHARON_COMM_UTIL_H


namespace charon {

// template<typename... Args>
// std::string formatString(const char* str, Args&&... args);


template<typename... Args>
std::string formatString(const char* str, Args&&... args) {
  char buf[1024] = {0};
  sprintf(buf, str, std::forward<Args>(args)...);
  return std::string(buf);
}


}


#endif