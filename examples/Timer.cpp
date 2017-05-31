#include <buzz/EventLoop.h>
#include <buzz/Timestamp.h>

#include <iostream>

using namespace buzz;

int main()
{
    EventLoop loop;
    
    auto Print = [&](const char* msg) {
        static int cnt = 0;
        std::cout << Timestamp::Now().ToString() << "-> " << msg << std::endl;
        if (++cnt == 20) {
            loop.Exit();
        }
    };

    loop.RunAfter(  1, std::bind(Print, "once 1s"));
    loop.RunAfter(1.5, std::bind(Print, "once 1.5s"));
    loop.RunAfter(  2, std::bind(Print, "once 2s"));

    loop.RunEvery(2, std::bind(Print, "every 2s"));
    loop.RunEvery(3, std::bind(Print, "every 3s"));

    loop.Loop();

    return 0;
}