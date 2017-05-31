#pragma once

#include <signal.h>
#include <functional>

namespace buzz
{
    struct Signal
    {
        static void Register(int sig_no, const std::function<void()>& handler);
    };
}