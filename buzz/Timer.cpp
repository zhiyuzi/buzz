#include "Timer.h"
#include "Logger.h"
#include "Timestamp.h"
#include "EventLoop.h"

#include <atomic>

#include <assert.h>

using namespace buzz;

class buzz::Timer : noncopyable
{
public:
    Timer(const TaskCallback&& task, const Timestamp when, double interval)
        : m_seq(m_timer_seq++),
        m_repeat(interval > 0.0),
        m_interval(interval),
        m_expiration(when),
        m_timer_task_callback(task)
    { }

    bool Repeat() const { return m_repeat; }
    Timestamp Expiration() { return m_expiration; }

    void Run() { if (m_timer_task_callback) m_timer_task_callback(); }

    void Restart(Timestamp now)
    {
        if (m_repeat) {
            m_expiration = AddTime(now, m_interval);
        } else {
            m_expiration = Timestamp::Invalid();
        }
    }

    uint64_t Sequence() { return m_seq; }
    
private:
    uint64_t m_seq;

    bool   m_repeat;
    double m_interval;

    Timestamp    m_expiration;
    TaskCallback m_timer_task_callback;
    

    static std::atomic<uint64_t> m_timer_seq;
};

std::atomic<uint64_t> buzz::Timer::m_timer_seq(0);

TimerManager::TimerManager(EventLoop* loop)
    : m_owner_loop(loop)
{ }

TimerManager::~TimerManager()
{
    for (auto it : m_timers) {
        delete it.second;
    }
}

void TimerManager::Cannel(TimerId timer_id)
{
    m_owner_loop->RunInLoop(std::bind(&TimerManager::CannelInLoop, this, timer_id));
}

TimerId TimerManager::AddTimer(const TaskCallback&& task, const Timestamp when,
                               double interval)
{
    Timer* timer = new Timer(std::move(task), when, interval);

    m_owner_loop->RunInLoop(std::bind(&TimerManager::Insert, this, timer));

    return TimerId(timer);
}

void TimerManager::Schedule()
{
    Timestamp now(Timestamp::Now());
    auto expired = GetExpired(now);

    for (auto it : expired) {
        it.second->Run();
    }

    Reset(expired, now);
}

time_t TimerManager::NearEndTime()
{
    Timestamp now(Timestamp::Now());

    auto it = m_timers.begin();
    if (it == m_timers.end()) return -1;

    auto t = it->first.MicroSecondsSinceEpoch() - now.MicroSecondsSinceEpoch();

    return static_cast<time_t>(t < 0 ? 0 : t / 1000);
}

void TimerManager::Insert(Timer* timer)
{
    Timestamp when(timer->Expiration());
    
    auto result = m_timers.insert(std::make_pair(when, timer));
    assert(result.second); (void) result;

    LOG(TRACE) << "add timer " << when.ToString() << " id " << timer->Sequence();
}

void TimerManager::CannelInLoop(TimerId timer_id)
{
    Timer* timer = timer_id.m_timer;
    if (timer == NULL) return;

    auto it = m_timers.find(std::make_pair(timer->Expiration(), timer));
    if (it != m_timers.end()) {
        auto n = m_timers.erase(Entry(it->first, it->second));
        assert(n == 1); (void) n;

        delete it->second;
        LOG(TRACE) << "cannel timer " << timer->Expiration().ToString()
                   << " id " << timer->Sequence();
    }
}

std::vector<TimerManager::Entry> TimerManager::GetExpired(Timestamp now)
{
    Entry entry = std::make_pair(now, reinterpret_cast<Timer*>(UINTPTR_MAX));

    auto it = m_timers.lower_bound(entry);
    assert(it == m_timers.end() || now < it->first);
    
    std::vector<Entry> expired;
    std::copy(m_timers.begin(), it, std::back_inserter(expired));

    m_timers.erase(m_timers.begin(), it);

    return expired;
}

void TimerManager::Reset(std::vector<Entry> expired, Timestamp now)
{
    for (auto it : expired) {
        if (it.second->Repeat()) {
            it.second->Restart(now);
            Insert(it.second);
        } else {
            delete it.second;
        }
    }
}