#include "../charon/log.h"

static charon::Logger::ptr g_logger = CHARON_LOG_ROOT();

int main(int argc, char** argv) {
    CHARON_LOG_INFO(g_logger) << "test log!";    
    return 0;
}