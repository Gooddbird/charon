#include "../charon/log.h"
#include "../charon/scheduler.h"
#include "../charon/thread.h"
#include "../charon/coroutine.h"
#include <unistd.h>
#include <vector>

static charon::Logger::ptr g_logger = CHARON_LOG_ROOT();


void fun() {
    CHARON_LOG_INFO(g_logger) << "FUN BEGIN";
    CHARON_LOG_INFO(g_logger) << "FUN END";

}

void test() {

    CHARON_LOG_INFO(g_logger) << "test begin";
    charon::Scheduler sc(1, "SCHE");
    sc.start();
    // sleep(2);
    // std::vector<charon::Coroutine::ptr> cors;
    // for(int i = 0; i < 1; i++) {
    //     charon::Coroutine::ptr cor(new charon::Coroutine(&fun));
    //     cors.push_back(cor);
    // }
    // sc.schedule(cors);
    sleep(3);
    sc.stop();
    CHARON_LOG_INFO(g_logger) << "test end";
}


int main(int argc, char** argv) {
    test();
    return 0;
}