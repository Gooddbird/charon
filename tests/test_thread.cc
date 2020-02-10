#include "../charon/log.h"
#include "../charon/thread.h"
#include <vector>
#include <string>


int count = 0;
static charon::Logger::ptr g_logger = CHARON_LOG_ROOT();
charon::RWMutex s_mutex;
void test() {
    CHARON_LOG_INFO(g_logger) << "name: " << charon::Thread::GetName()
                             << "this.name: " << charon::Thread::GetThis()->getName()
                             << "id:" << charon::GetThreadId() 
                             << "this.id: " << charon::Thread::GetThis()->getId();
    for(int i = 0; i < 1000000; i++) {
        // charon::RWMutex::WriteLock lock(s_mutex);
        ++count;
    }
}

void test_thread() {
    CHARON_LOG_INFO(g_logger) << "test_thread begin";
    std::vector<charon::Thread::ptr> m_thrs;
    std::vector<int> m_ids;
    for(int i = 0; i < 3; ++i) {
        charon::Thread::ptr thr(new charon::Thread(&test, "name" + std::to_string(i)));
        m_thrs.push_back(thr);
        m_ids.push_back(m_thrs[i]->getId());
    }
    for(int i = 0; i < 3; ++i) {
        m_thrs[i]->join();
    }
    CHARON_LOG_INFO(g_logger) << "count = " << count;
    CHARON_LOG_INFO(g_logger) << "test_thread end";
}

int main(int argc, char** argv) {

    test_thread();
    return 0;
}