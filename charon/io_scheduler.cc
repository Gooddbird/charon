#include "io_scheduler.h"
#include "log.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

namespace charon {

static charon::Logger::ptr g_logger = CHARON_LOG_ROOT();

IO_Scheduler::IO_Scheduler(size_t threads, const std::string& name) : Scheduler(threads, name) {

    m_epfd = epoll_create(5000);
    assert(m_epfd > 0);
    int rt = pipe(m_ticklePipe);
    assert(!rt);
    rt = fcntl(m_ticklePipe[0], F_SETFL, O_NONBLOCK);
    assert(!rt);
    epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = m_ticklePipe[0];
    rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_ticklePipe[0], &event);
    assert(!rt);
    resizeFdContexts(16);
    start();
}

IO_Scheduler::~IO_Scheduler() {
    stop();
    close(m_ticklePipe[0]);
    close(m_ticklePipe[1]);
    close(m_epfd);
    for(size_t i = 0; i < m_fdContexts.size(); ++i) {
        if(m_fdContexts[i]) {
            delete m_fdContexts[i];
        }
    }
}

void IO_Scheduler::resizeFdContexts(size_t size) {
    m_fdContexts.resize(size);
    for(size_t i = 0; i < size; ++i) {
        m_fdContexts[i] = new FdContext;
        m_fdContexts[i]->m_fd = i;
    }
}

bool IO_Scheduler::addEvent(int fd, Event event, Coroutine::ptr coroutine) {
    if(event != READ && event != WRITE) {
        CHARON_LOG_ERROR(g_logger) << "you can't add this event"; 
        return false;
    }
    FdContext* fdCtx;
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() > fd) {
        fdCtx = m_fdContexts[fd];
        lock.unlock();
    } else {
        RWMutexType::WriteLock lock2(m_mutex);
        resizeFdContexts((size_t)(1.5 * fd));
        fdCtx = m_fdContexts[fd];
    }
    MutexType::Lock lock3(fdCtx->m_mutex);
    if(fdCtx->m_events & event) {
        CHARON_LOG_ERROR(g_logger) << "repetitive event";
        return false;
    }
    if(event == READ) {
        fdCtx->m_readCor = coroutine;
    } else if(event == WRITE) {
        fdCtx->m_writeCor = coroutine;
    }
    fdCtx->m_scheduler = Scheduler::GetScheduler();
    int op = fdCtx->m_events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    epoll_event new_event;
    new_event.events = fdCtx->m_events | event | EPOLLET;
    new_event.data.fd = fd;
    new_event.data.ptr = fdCtx;
    int rt = epoll_ctl(m_epfd, op, fd, &new_event);
    if(rt) {
        CHARON_LOG_ERROR(g_logger) << "epoll_ctl error";
        return false;
    }
    ++m_tasks;
    fdCtx->m_events =(Event)(fdCtx->m_events | event);
    return true;
}


bool IO_Scheduler::delEvent(int fd, Event event) {
    if(event != READ && event != READ) {
        CHARON_LOG_ERROR(g_logger) << "you can't delete this event"; 
        return false;
    }
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() < fd) {
        CHARON_LOG_ERROR(g_logger) << fd << "not exist";
        return false;
    }
    FdContext* fdCtx = m_fdContexts[fd];
    lock.unlock();
    MutexType::Lock lock2(fdCtx->m_mutex);
    if(!(fdCtx->m_events & event)) {
        CHARON_LOG_ERROR(g_logger) << "not exist this event";
        return false;
    }
    fdCtx->m_events = (Event)(fdCtx->m_events & ~event);
    int op = fdCtx->m_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event new_event;
    new_event.events = fdCtx->m_events | EPOLLET;
    new_event.data.ptr = fdCtx;
    int rt = epoll_ctl(m_epfd, op, fd, &new_event);
    if(rt) {
        CHARON_LOG_ERROR(g_logger) << "epoll_ctl error";
        return false;
    }
    --m_tasks;
    if(event == READ) {
        fdCtx->m_readCor.reset();
    } else if(event == WRITE) {
        fdCtx->m_writeCor.reset();
    }
    if(fdCtx->m_readCor.get() == nullptr && fdCtx->m_writeCor.get() == nullptr) {
        fdCtx->m_scheduler = nullptr;
    }
    return true;
}


IO_Scheduler* IO_Scheduler::GetIO_Scheduler() {
    return (IO_Scheduler*)Scheduler::GetScheduler();
}

void IO_Scheduler::idle() {
    CHARON_LOG_DEBUG(g_logger) << "io_scheduler::idle";
    // Scheduler::idle();
    const uint64_t MAX_EVENT = 256;
    epoll_event* events = new epoll_event[256];
    std::shared_ptr<epoll_event> ptr(events, [](epoll_event* p) {
        delete[] p;
    });
    while(true) {
        if(stopping()) {
            CHARON_LOG_DEBUG(g_logger) << "idle stopping exit";
            break;
        }
        int rt = 0;
        do {
            static const int MAX_TIMEOUT = 5000;
            rt = epoll_wait(m_epfd, events, MAX_EVENT, MAX_TIMEOUT);
            if(rt < 0 && errno == EINTR) {

            } else {
                break;
            }
        } while(true);

        for(int i = 0; i < rt; ++i) {
            epoll_event& event = events[i];

            if(event.data.fd == m_ticklePipe[0]) {
                uint8_t res[256];
                while(read(event.data.fd, res, sizeof(res)) > 0);
                continue;
            }
            FdContext* fdCtx = (FdContext*)event.data.ptr;
            MutexType::Lock lock(fdCtx->m_mutex);
            if(event.events & (EPOLLERR | EPOLLHUP)) {
                event.events |= fdCtx->m_events & (EPOLLIN | EPOLLOUT);
            } 
            Event io_event = NONE;
            if(event.events & EPOLLIN) {
                io_event = (Event)(io_event | READ);
            }
            if(event.events & EPOLLOUT) {
                io_event = (Event)(io_event | WRITE);
            }
            if((io_event & fdCtx->m_events) == NONE) {
                continue;
            }
            if(io_event & READ) {
                fdCtx->m_events = (Event)(fdCtx->m_events & ~READ);
                fdCtx->m_scheduler->schedule(fdCtx->m_readCor);
                --m_tasks;
            }   
            if(io_event & WRITE) {
                fdCtx->m_events = (Event)(fdCtx->m_events & ~WRITE);
                fdCtx->m_scheduler->schedule(fdCtx->m_writeCor);
                --m_tasks;
            }
            
        }
        Coroutine::ptr cor = Coroutine::GetCoroutine();
        auto ptr = cor.get();
        cor.reset();
        ptr->swapToBack();
    }
    


}

void IO_Scheduler::tickle() {
    if(!Scheduler::hasIdleThread()) {
        return;
    }
    int rt = write(m_ticklePipe[1], "t", 1);
    assert(rt == 1);
}

bool IO_Scheduler::stopping() {
    return Scheduler::stopping() && m_tasks == 0;
}

}