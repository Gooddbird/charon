#ifndef __CHARON_NONCOPYABLE_H__
#define __CHARON_NONCOPYABLE_H__


namespace charon {

class Noncopyable {

public:
    Noncopyable() = default;
    ~Noncopyable() = default;
    Noncopyable(const Noncopyable& ) = delete;
    Noncopyable& operator=(const Noncopyable&) = delete;
};

}

#endif