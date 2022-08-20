#ifndef CHARON_COMM_UTIL_H
#define CHARON_COMM_UTIL_H


namespace charon {

template<typename... Args>
std::string formatString(const char* str, Args&&... args);


}


#endif