#include "coroutine.h"
#include "log.h"
#include "scheduler.h"
#include <atomic>
#include <assert.h>




namespace charon {

static Logger::ptr g_logger = CHARON_LOG_ROOT();

static std::atomic<uint32_t> CoroutineId {0};

static std::atomic<uint32_t> CoroutineCounts {0};

static thread_local Coroutine* t_Coroutine = nullptr;

static thread_local Coroutine::ptr t_MainCoroutine = nullptr;


Coroutine::Coroutine() {
   
    SetCoroutine(this);
    m_state = EXEC;
    if(getcontext(&m_ctx)) {
        CHARON_LOG_ERROR(g_logger) << "getcontext error, id=" << m_id;
    }
    ++CoroutineCounts;
    CHARON_LOG_DEBUG(g_logger) << "Coroutine::Coroutine()   MainCoroutine, id=" << m_id;
}


Coroutine::Coroutine(std::function<void()> cb, uint32_t stacksize) :m_id(++CoroutineId), m_stacksize(stacksize), m_cb(cb) {

    ++CoroutineCounts;
    m_stackptr = malloc(m_stacksize);
    if(getcontext(&m_ctx)) {
        CHARON_LOG_ERROR(g_logger) << "getcontext error, and id=" << m_id;
    }
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stackptr;
    m_ctx.uc_stack.ss_size = m_stacksize;
    makecontext(&m_ctx, &Coroutine::MainFunc, 0);        
    CHARON_LOG_DEBUG(g_logger) << "Coroutine::Coroutine(), and id=" << m_id;
}

Coroutine::~Coroutine() {

    --CoroutineCounts;
    if(m_stackptr) {
        assert(m_state == END || m_state == EXCEPT || m_state == INIT);
        free(m_stackptr);
    } else {
        assert(!m_cb);
        assert(m_state == EXEC);
        t_Coroutine = nullptr;
        t_MainCoroutine = nullptr;
    }
    CHARON_LOG_DEBUG(g_logger) << "Coroutine::~Coroutine(), and id=" << m_id << ", current CoroutineCounts=" << CoroutineCounts;
}
    
void Coroutine::swapToExec() {
    SetCoroutine(this);
    assert(m_state != EXEC);
    m_state = EXEC;
    if(swapcontext(&Scheduler::GetMainCoroutine()->m_ctx, &m_ctx)) {
        CHARON_LOG_ERROR(g_logger) << "swapcontext error, and id=" << m_id;
    }
}

void Coroutine::swapToBack() {
    SetCoroutine(Scheduler::GetMainCoroutine());
    if(swapcontext(&m_ctx, &Scheduler::GetMainCoroutine()->m_ctx)) {
        CHARON_LOG_ERROR(g_logger) << "swapcontext error, and id=" << m_id;
    }
}

void Coroutine::SetCoroutine(Coroutine* v) {
    t_Coroutine = v;
}

Coroutine::ptr Coroutine::GetCoroutine() {
    if(t_Coroutine) {
        return t_Coroutine->shared_from_this();
    }
    Coroutine::ptr main_coroutine(new Coroutine);
    assert(main_coroutine.get() == t_Coroutine);
    t_MainCoroutine = main_coroutine;
    return t_Coroutine->shared_from_this();
}

uint32_t Coroutine::GetCoroutineCounts() {
    return CoroutineCounts;
}

void Coroutine::ToHold() {
    Coroutine::ptr cur = GetCoroutine();
    assert(cur->m_state == EXEC);
    cur->m_state = HOLD;
    cur->swapToBack();
}

void Coroutine::ToReady() {
    Coroutine::ptr cur = GetCoroutine();
    assert(cur->m_state == EXEC);
    cur->m_state = READY;
    cur->swapToBack();
}

void Coroutine::MainFunc() {

    Coroutine::ptr cur = GetCoroutine(); 
    cur->m_cb();
    cur->m_cb = nullptr;
    cur->m_state = END;

    Coroutine* ptr = cur.get();
    cur.reset();
    ptr->swapToBack();
}

uint32_t Coroutine::GetCoroutineId() {
    if(t_Coroutine) {
        return t_Coroutine->getId();
    }
    return 0;
}



}