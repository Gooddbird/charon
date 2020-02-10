#include "scheduler.h"
#include "log.h"
#include <assert.h>



namespace charon {

static charon::Logger::ptr g_logger = CHARON_LOG_ROOT();

static thread_local Scheduler* t_Scheduler = nullptr;

static thread_local Coroutine* t_MainCoroutine = nullptr;


Scheduler::Scheduler(size_t threads, const std::string& name) : m_name(name), m_threadCounts(threads) {


}

Scheduler::~Scheduler() {
    
    assert(m_stopping);
    t_Scheduler = nullptr;

}

void Scheduler::start() {

    MutexType::Lock lock(m_mutex);
    m_stopping = false;
    assert(m_threads.empty());
    m_threads.resize(m_threadCounts);
    for(size_t i = 0; i < m_threadCounts; ++i) {
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this), "name_" + std::to_string(i)));
    }

}


void Scheduler::stop() {

    m_stopping = true;
    std::vector<Thread::ptr> thrs;
    {
        MutexType::Lock lock(m_mutex);
        thrs.swap(m_threads);
    }

    for(auto& i : thrs) {
        i->join();
    }
}

void Scheduler::run() {

    CHARON_LOG_INFO(g_logger) << "Scheduler " << m_name << "  run";
    setThis();
    t_MainCoroutine = Coroutine::GetCoroutine().get();
    Coroutine::ptr idle_coroutine(new Coroutine(std::bind(&Scheduler::idle, this)));
    Coroutine::ptr task;
    while(true) {

        bool is_active = false;
        bool do_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            auto it = m_coroutineList.begin();
            while(it != m_coroutineList.end()) {
                if(*it && (*it)->getState() == Coroutine::EXEC) {
                    ++it;
                    continue;
                }
                task = *it;
                m_coroutineList.erase(it++);
                ++m_activeThreadCounts;
                is_active = true;
                break;
            }
            do_tickle |= it != m_coroutineList.end();
        }
        if(do_tickle) {
            tickle();
        }
        if(task && (task->getState() != Coroutine::END && task->getState() != Coroutine::EXCEPT)) {
            task->swapToExec();
            --m_activeThreadCounts;
            if(task->getState() == Coroutine::READY) {
                schedule(task);
            } else if(task->getState() != Coroutine::END && task->getState() != Coroutine::EXCEPT) {
                task->m_state = Coroutine::HOLD;
            }
            task.reset();
        } else {
            if(is_active) {
                --m_activeThreadCounts;
                continue;
            }
            if(idle_coroutine->getState() == Coroutine::END) {
                CHARON_LOG_INFO(g_logger) << "ilde stop";
                break;
            }
            ++m_idleThreadCounts;
            idle_coroutine->swapToExec();
            --m_idleThreadCounts;
            if(idle_coroutine->getState() != Coroutine::END && idle_coroutine->getState() != Coroutine::EXCEPT) {
                idle_coroutine->m_state = Coroutine::HOLD;
            }
        }
    }
}

void Scheduler::setThis() {

    t_Scheduler = this;
}

void Scheduler::schedule(Coroutine::ptr coroutine) {

    MutexType::Lock lock(m_mutex);
    bool do_tickle = m_coroutineList.empty();
    m_coroutineList.push_back(coroutine);
    lock.unlock();
    if(do_tickle) {
        tickle();
    }
}


void Scheduler::schedule(std::vector<Coroutine::ptr> vec) {

    MutexType::Lock lock(m_mutex);
    bool do_tickle = m_coroutineList.empty();
    for(size_t i = 0; i < vec.size(); ++i) {
        m_coroutineList.push_back(vec[i]);
    }
    vec.clear();
    lock.unlock();
    if(do_tickle) {
        tickle();
    }
}

Scheduler* Scheduler::GetScheduler() {

    return t_Scheduler;
}

Coroutine* Scheduler::GetMainCoroutine() {

    return t_MainCoroutine;
}

void Scheduler::tickle() {

    CHARON_LOG_INFO(g_logger) << "tickle()";

}

void Scheduler::idle() {

    CHARON_LOG_INFO(g_logger) << "idle begin";
    while(!stopping()) {
        charon::Coroutine::ToHold();
    }
    CHARON_LOG_INFO(g_logger) << "idle end";

}

bool Scheduler::stopping() {

    MutexType::Lock lock(m_mutex);
    return m_stopping && m_coroutineList.empty() && m_activeThreadCounts == 0;
}


}