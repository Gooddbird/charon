#ifndef __CHARON_SCHEDULER_H__
#define __CHARON_SCHEDULER_H__

#include <memory>
#include <atomic>
#include <vector>
#include <list>
#include "thread.h"
#include "coroutine.h"
#include "mutex.h"

namespace charon {


class Scheduler {

public:

    typedef std::shared_ptr<Scheduler> ptr;
    typedef charon::Mutex MutexType;

    Scheduler(size_t threads = 1, const std::string& name = "Default scheduler");

    virtual ~Scheduler();

    const std::string& getName() const { return m_name; }

    void start();

    void stop();

    void run();

    void setThis();

    void schedule(Coroutine::ptr coroutine);

    void schedule(std::vector<Coroutine::ptr> vec);

    bool hasIdleThread() { return m_idleThreadCounts > 0; }

protected:

    virtual void tickle();

    virtual void idle();

    virtual bool stopping();

public:
    static Scheduler* GetScheduler();

    static Coroutine* GetMainCoroutine(); 
private:

    MutexType m_mutex;
    std::vector<Thread::ptr> m_threads;
    std::list<Coroutine::ptr> m_coroutineList;
    std::string m_name;

protected:
    std::atomic<size_t> m_threadCounts = {0};
    std::atomic<size_t> m_activeThreadCounts = {0};
    std::atomic<size_t> m_idleThreadCounts = {0};
    bool m_stopping = true;
    // bool m_autoStop = false;
};


}

#endif