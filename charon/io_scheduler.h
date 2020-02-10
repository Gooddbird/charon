#ifndef __CHARON_IO_SCHEDULER_H__
#define __CHARON_IO_SCHEDULER_H__

#include <memory>
#include <vector>
#include "scheduler.h"
#include "coroutine.h"
#include "mutex.h"

namespace charon {

enum Event {

    NONE = 0X0,

    READ = 0X1,

    WRITE = 0X4,
};


class FdContext {
friend class IO_Scheduler;
public:
    typedef charon::Mutex MutexType;
    FdContext() {};
    ~FdContext() {};

private:
    
    int m_fd = 0;
    Event m_events = NONE;
    MutexType m_mutex;
    Coroutine::ptr m_readCor;
    Coroutine::ptr m_writeCor;
    Scheduler* m_scheduler = nullptr;   

};

class IO_Scheduler : public Scheduler {

public:
    typedef std::shared_ptr<IO_Scheduler> ptr;
    typedef charon::RWMutex RWMutexType;
    IO_Scheduler(size_t threads, const std::string& name);
    ~IO_Scheduler();
    
    void resizeFdContexts(size_t size);

    bool addEvent(int fd, Event event, Coroutine::ptr coroutine);
    bool delEvent(int fd, Event);

public:
    static IO_Scheduler* GetIO_Scheduler();

protected:
    void idle() override;
    bool stopping() override;
    void tickle() override;

private:

    int m_epfd = 0;
    int m_ticklePipe[2];
    std::atomic<size_t> m_tasks {0};
    std::vector<FdContext*> m_fdContexts;
    RWMutexType m_mutex;

};




}

#endif