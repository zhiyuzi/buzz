#pragma once

#include "Callbacks.h"
#include "noncopyable.h"

#include <set>
#include <vector>

namespace buzz
{
    class Timer;
    class EventLoop;
    class Timestamp;
    class TimerManager;

    class TimerId
    {
    public:
        TimerId(Timer* timer) : m_timer(timer)
        { }

    private:
        friend class TimerManager;
        Timer* m_timer;
    };

    class TimerManager : noncopyable
    {
    public:
        TimerManager(EventLoop* loop);
        ~TimerManager();

        void Cannel(TimerId timer_id);
        TimerId AddTimer(const TaskCallback&& task, const Timestamp when, double interval);
       
        void Schedule();
        time_t NearEndTime();

    private:
        EventLoop* m_owner_loop;

        typedef std::pair<Timestamp, Timer*> Entry;
        std::set<Entry> m_timers;

        void Insert(Timer* timer);
        void CannelInLoop(TimerId timer_id);
        std::vector<Entry> GetExpired(Timestamp now);
        void Reset(std::vector<Entry> expired, Timestamp now);
    };
}