#include <string>
#include "charon/comm/util.h"

namespace charon {

template<typename... Args>
std::string formatString(const char* str, Args&&... args) {
  std::string re;
  sprintf(re.c_str(), str, std::forward<Args>(args)...);
  return re;
}

}
