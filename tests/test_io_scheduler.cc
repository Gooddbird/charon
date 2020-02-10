#include "../charon/log.h"
#include "../charon/scheduler.h"
#include "../charon/thread.h"
#include "../charon/coroutine.h"
#include "../charon/io_scheduler.h"
#include <unistd.h>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>
#include <sys/epoll.h>
#include <string.h>
#include <errno.h>

static charon::Logger::ptr g_logger = CHARON_LOG_ROOT();
int sock = 0;

void test_fiber() {
    
    CHARON_LOG_INFO(g_logger) << "test_fiber sock=" << sock;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "14.215.177.39", &addr.sin_addr.s_addr);
    CHARON_LOG_INFO(g_logger) << "add event errno=" << errno << " " << strerror(errno);
    charon::Coroutine::ptr read_cor(new charon::Coroutine([](){
        CHARON_LOG_INFO(g_logger) << "WRITE callback";
    }));
    charon::IO_Scheduler::GetIO_Scheduler()->addEvent(sock, charon::Event::WRITE, read_cor); 
    connect(sock, (const sockaddr*)&addr, sizeof(addr));
    // if(!connect(sock, (const sockaddr*)&addr, sizeof(addr))) {
    // } else if(errno == EINPROGRESS) {
        // CHARON_LOG_INFO(g_logger) << "add event errno=" << errno << " " << strerror(errno);
        // charon::IO_Scheduler::GetIO_Scheduler()->addEvent(sock, charon::Event::READ, [](){
        //     CHARON_LOG_INFO(g_logger) << "read callback";
        // });
        // charon::IO_Scheduler::GetIO_Scheduler()->addEvent(sock, charon::Event::WRITE, [](){
        //     CHARON_LOG_INFO(g_logger) << "write callback";
            
        // });
    // } else {

    // }

}
void test() {
    charon::IO_Scheduler io(1, "test_io");
    charon::Coroutine::ptr cor(new charon::Coroutine(&test_fiber));
    io.schedule(cor);
}

int main(int argc, char** argv) {
    test();
    return 0;
}