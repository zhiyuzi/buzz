#pragma once

#include <thread>
#include <vector>
#include <atomic>
#include <memory>

namespace buzz
{
    class EventLoop;
    class EventLoopThread;

    class EventLoopThreadPoll
    {
    public:
        EventLoopThreadPoll(EventLoop* base_loop)
            : m_base_loop(base_loop),
            m_loop_start(false),
            m_event_poll_size(0), 
            m_id(0),
            m_event_loops(0),
            m_threads(0)
        { }

        ~EventLoopThreadPoll();

        void Start(size_t poll_size = 0);
        
        EventLoop* Get();
    private:
        EventLoop*   m_base_loop;

        bool   m_loop_start;
        size_t m_event_poll_size;

        std::atomic_ullong       m_id;
        std::vector<EventLoop* > m_event_loops;

        std::vector<EventLoopThread*> m_threads;
    };
}