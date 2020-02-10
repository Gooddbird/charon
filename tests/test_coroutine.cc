#include "../charon/coroutine.h"
#include "../charon/log.h"

static charon::Logger::ptr g_logger = CHARON_LOG_ROOT();

void fun() {

    CHARON_LOG_INFO(g_logger) << "fun begin";    
    charon::Coroutine::ToHold(); 
    CHARON_LOG_INFO(g_logger) << "test_coroutine after hold";    
    CHARON_LOG_INFO(g_logger) << "fun end";    
}

void test_coroutine() {
    CHARON_LOG_INFO(g_logger) << "test_coroutine begin";    
    charon::Coroutine::GetCoroutine();
    charon::Coroutine::ptr cc(new charon::Coroutine(fun));
    cc->swapToExec();
    CHARON_LOG_INFO(g_logger) << "test_coroutine after swapToExec";
    cc->swapToExec();
    CHARON_LOG_INFO(g_logger) << "test_coroutine end";    
}

int main(int argc, char** argv) {
    test_coroutine();
    return 0;
}