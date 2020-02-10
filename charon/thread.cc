#include "thread.h"
#include "util.h"
#include "log.h"

namespace charon {

static thread_local Thread* t_thread = nullptr;

static thread_local std::string t_thread_name = "UNKNOW";

static charon::Logger::ptr g_logger = CHARON_LOG_ROOT();


Thread::Thread(std::function<void()> cb, const std::string& name) : m_cb(cb), m_name(name) {
    if(m_name.empty()) {
        m_name = "UNKNOW";
    }
    if(pthread_create(&m_thread, nullptr, &Thread::run, this)) {
        CHARON_LOG_ERROR(g_logger) << "pthread_create error";
    }
    m_semphore.wait();
}

Thread::~Thread() {
    if(m_thread) {
        pthread_detach(m_thread);
    }
}

void Thread::join() {
    if(m_thread) {
        if(pthread_join(m_thread, nullptr)) {
            CHARON_LOG_ERROR(g_logger) << "pthread_join error";
        }
        m_thread = 0;
    }
}

Thread* Thread::GetThis() {
    return t_thread;
}

const std::string& Thread::GetName() {
    return t_thread_name;
}


void Thread::SetName(const std::string& name) {
    if(t_thread) {
        t_thread->m_name = name;
    }
    t_thread_name = name;
}

void* Thread::run(void* arg) {
    Thread* thread = (Thread*)arg;
    t_thread = thread;
    t_thread_name = thread->getName();
    thread->m_id = charon::GetThreadId();
    pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());
    std::function<void()> cb;
    cb.swap(thread->m_cb);
    thread->m_semphore.post();
    cb();
    return nullptr;
}


}