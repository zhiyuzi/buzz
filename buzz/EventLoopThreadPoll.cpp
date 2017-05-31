#include "EventLoop.h"
#include "EventLoopThreadPoll.h"

#include <assert.h>

using namespace buzz;

class buzz::EventLoopThread
{
public:
    EventLoopThread() : m_loop(nullptr), m_thread(nullptr)
    { }

    EventLoop* GetLoop()
    {
        m_thread.reset(new std::thread([this] {
            EventLoop loop;
            
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_loop = &loop;

                m_cond.notify_one();
            }

            loop.Loop();
        }));

        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cond.wait(lock, [this] { return m_loop; });
        }

        return m_loop;
    }
private:
    EventLoop* m_loop;
    std::mutex m_mutex;

    std::condition_variable      m_cond;
    std::unique_ptr<std::thread> m_thread;
};

EventLoopThreadPoll::~EventLoopThreadPoll()
{
    m_loop_start = false;

    for (size_t i = 0; i < m_event_poll_size; i++) {
        delete m_threads[i];
    }
}
void EventLoopThreadPoll::Start(size_t poll_size)
{
    assert(m_loop_start == false);
    m_loop_start = true;

    m_event_poll_size = poll_size;

    for (size_t i = 0; i < m_event_poll_size; i++) {
        auto t = new EventLoopThread();

        m_threads.push_back(t);
        m_event_loops.push_back(t->GetLoop());
    }
}

EventLoop* EventLoopThreadPoll::Get()
{
    if (m_event_poll_size) {
        return m_event_loops[m_id++ % m_event_poll_size];
    }
    
    return m_base_loop;
}