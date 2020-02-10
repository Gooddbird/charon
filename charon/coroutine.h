#ifndef __CHARON_COROUTINE_H__
#define __CHARON_COROUTINE_H__

#include <memory>
#include <ucontext.h>
#include <functional>

namespace charon {

class Coroutine :  public std::enable_shared_from_this<Coroutine> {
friend class Scheduler;
friend class IO_Scheduler;
public:
    typedef std::shared_ptr<Coroutine> ptr;

    enum State {
        // 初始化
        INIT,
        // 暂停
        HOLD,
        // 执行中
        EXEC,
        // 执行结束
        END,
        // 可执行状态
        READY,
        // 异常
        EXCEPT
    };

public:

    Coroutine(std::function<void()> cb, uint32_t stacksize = 128 * 1024);

    ~Coroutine();

    void swapToExec();

    void swapToBack();

    State getState() const { return m_state; }

    uint32_t getId() const { return m_id; }
public:
    static void SetCoroutine(Coroutine* v);

    static Coroutine::ptr GetCoroutine();

    static uint32_t GetCoroutineCounts();

    static uint32_t GetCoroutineId();

    static void MainFunc();

    static void ToHold();

    static void ToReady();
private:
    Coroutine();


private:

    uint32_t m_id = 0;

    uint32_t m_stacksize = 0;

    void* m_stackptr = nullptr;

    State m_state = INIT;

    ucontext_t m_ctx;

    std::function<void()> m_cb;


};

}

#endif