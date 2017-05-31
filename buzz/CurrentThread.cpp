#include "CurrentThread.h"

#if defined(HAVE_PTHREAD_GETTHREADID_NP)
    #include <pthread_np.h>
#elif defined(__FreeBSD__)
    #include <sys/thr.h>
#elif defined(__linux__)
    #include <sys/syscall.h>
#endif

__thread pid_t t_cacheTid;

void cacheTid()
{
#if defined(HAVE_PTHREAD_GETTHREADID_NP)
    ret = pthread_getthreadid_np();
#elif defined(__linux__)
    t_cacheTid = syscall(__NR_gettid);
#elif defined(__FreeBSD__)
    long lwpid;
    thr_self(&lwpid);
    t_cacheTid = static_cast<pid_t>(lwpid);
#elif defined(__APPLE__)
    t_cacheTid = mach_thread_self();
    mach_port_deallocate(mach_task_self(), ret);
#else
    static_assert(false, "unknown Operating System");
#endif
}

pid_t buzz::CurrentThread::threadId()
{
    if (__builtin_expect(t_cacheTid == 0, 0)) {
        cacheTid();
    }

    return t_cacheTid;
}
