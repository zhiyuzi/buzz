#pragma once

#include "Timer.h"
#include "Callbacks.h"
#include "Timestamp.h"
#include "noncopyable.h"
#include "CurrentThread.h"
#include "BlockingQueue.h"

#include <atomic>

namespace buzz
{
    class Poller;
    class Channel;
    class TimerId;
    class TimerManger;

    class EventLoop : noncopyable
    {
    public:
        EventLoop();
        ~EventLoop();

        void Loop();
        void Exit();

        bool IsInLoopThread() { return m_thread_id == CurrentThread::threadId(); }
        void AssertInLoopThread();

        TimerId RunAt(const Timestamp& time, TaskCallback&& task);
        TimerId RunAfter(double delay, TaskCallback&& task);
        TimerId RunEvery(double interval, TaskCallback&& task);

        Poller* GetPoller() { return m_poller.get(); }
        void RunInLoop(const TaskCallback&& task);
    private:
        pid_t m_thread_id;
        bool  m_exited;
        bool  m_looping;
        
        std::unique_ptr<Poller> m_poller;
        std::vector<Channel*>   m_active_channel;
        TimerManager            m_timer_manager;

        Channel*  m_wakeup_channel;
        Timestamp m_poll_return_time;
        
        BlockingQueue<TaskCallback> m_tasks;

        int m_wakeup_pipe[2];

        void Wakeup();
        void AbortNotInLoopThread();
    };
}
