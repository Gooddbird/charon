#include "util.h"
#include "coroutine.h"
#include <execinfo.h>
#include <unistd.h>
#include <sys/syscall.h>

namespace charon {

pid_t GetThreadId() {
	return syscall(SYS_gettid);
}

uint32_t GetFiberId() {
	return charon::Coroutine::GetCoroutineId();
}


}



