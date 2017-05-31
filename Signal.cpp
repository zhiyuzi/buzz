#include "Signal.h"

#include <map>
#include <signal.h>

namespace buzz
{
    static std::map<int, std::function<void()>> g_handlers;
    void SignalHandler(int sig_no)
    {
        g_handlers[sig_no]();
    }
}

using namespace buzz;

void Signal::Register(int sig_no, const std::function<void()>& handler)
{
    g_handlers[sig_no] = handler;
    ::signal(sig_no, SignalHandler);
}