#include "Poller.h"
#include "Logger.h"
#include "Channel.h"
#include "EventLoop.h"

#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

using namespace buzz;

struct IgnSigPipe
{
    IgnSigPipe()
    {
        ::signal(SIGPIPE, SIG_IGN);
    }
};

IgnSigPipe g_ign_sig_pipe;

__thread EventLoop* t_loop_in_this_thread = NULL;

EventLoop::EventLoop()
    : m_thread_id(CurrentThread::threadId()),
    m_exited(false), 
    m_looping(false),
    m_poller(MakePoller()),
    m_timer_manager(this),
    m_wakeup_channel(NULL),
    m_tasks(0)
{
    LOG(DEBUG) << "EventLoop created " << this << " in thread " << m_thread_id;

    if (t_loop_in_this_thread) {
        LOG(FATAL) << "Another EventLoop " << t_loop_in_this_thread
                   << " exists in this thread " << m_thread_id;
    }

    t_loop_in_this_thread = this;

    if (pipe2(m_wakeup_pipe, O_CLOEXEC) == -1) {
        LOG(FATAL) << "pipe2 create m_wakeup_pipe error " 
                   << errno << " ("<< ::strerror(errno) << ')';
    }

    m_wakeup_channel = new Channel(this, m_wakeup_pipe[0], kReadEvent);
    m_wakeup_channel->OnRead([=](Timestamp time) {
        char buf[1024];
        ssize_t ret = ::read(m_wakeup_channel->GetFd(), buf, sizeof(buf));
        if (ret > 0) {
            TaskCallback task;

            while (m_tasks.Poll(task, 0)) { 
                task();
            }
        } else if (ret == 0){
            delete m_wakeup_channel;
            m_wakeup_channel = NULL;
        } else {
            LOG(FATAL) << "wakeup channel read error " << errno << " (" << strerror(errno) << ')';
        }
    });
}

EventLoop::~EventLoop()
{
    LOG(DEBUG) << "EventLoop " << this << " of thread " << m_thread_id
               << " destructs in thread " << CurrentThread::threadId();
    
    if (m_wakeup_channel) delete m_wakeup_channel;

    ::close(m_wakeup_pipe[0]);
    ::close(m_wakeup_pipe[1]);

    t_loop_in_this_thread = NULL;
}

void EventLoop::Loop()
{
    AssertInLoopThread();

    assert(m_looping == false);
    m_looping = true;
    LOG(TRACE) << "EventLoop " << this << " start looping";

    while (m_exited == false) {
        m_active_channel.clear();

        int timeout = m_timer_manager.NearEndTime();
        
        m_poll_return_time = m_poller->Poll(timeout < 0 ? (5 * 1000) : timeout,
                                            &m_active_channel);
        
        int nready = m_active_channel.size();
        for (int i = 0; i < nready; i++) {
            m_active_channel[i]->HandleEvent(m_poll_return_time);
        }

        m_timer_manager.Schedule();
    }

    LOG(TRACE) << "EventLoop " << this << " stop looping";
    m_looping = false;
}

void EventLoop::Exit()
{
    m_exited = true;
    if (IsInLoopThread()) Wakeup();
}

void EventLoop::AssertInLoopThread()
{
    if (IsInLoopThread() == false) {
        AbortNotInLoopThread();
    }
}

TimerId EventLoop::RunAt(const Timestamp& time, TaskCallback&& task)
{
    return m_timer_manager.AddTimer(std::move(task), time, 0.0);
}

TimerId EventLoop::RunAfter(double delay, TaskCallback&& task)
{
    Timestamp time(AddTime(Timestamp::Now(), delay));
    return RunAt(time, std::move(task));
}

TimerId EventLoop::RunEvery(double interval, TaskCallback&& task)
{
    Timestamp time(AddTime(Timestamp::Now(), interval));
    return m_timer_manager.AddTimer(std::move(task), time, interval);
}

void EventLoop::RunInLoop(const TaskCallback&& task)
{
    if (IsInLoopThread()) {
        task();
    } else {
        m_tasks.Put(task);
        Wakeup();
    }
}

void EventLoop::Wakeup()
{
    char one = 1;
    ssize_t n = ::write(m_wakeup_pipe[1], &one, sizeof(one));

    if (n != sizeof(one)) {
        LOG(ERROR) << "EventLoop::Wakeup writes " << n << " bytes instead of " 
                   << sizeof(one);
    }
}

void EventLoop::AbortNotInLoopThread()
{
    LOG(FATAL) << "EventLoop::AbortNotInLoopThread - EventLoop " << this
               << " was created in m_thread_id = " << m_thread_id
               << ", current thread id = " << CurrentThread::threadId();
}

#include <iostream>

#include "Buffer.h"
#include "TcpServer.h"
#include "TcpConnection.h"

EventLoop* g_loop;

void func()
{
    EventLoop loop;
    g_loop = &loop;

    InetAddress local_address(9001);
    TcpServer tcp_server(&loop, "test", local_address);

    tcp_server.OnStateChange([](const TcpConnectionPtr& conn) {
        if (conn->GetState() == TcpConnection::kConnected) {
            LOG(INFO) << "kConnected";
        } else if (conn->GetState() == TcpConnection::kDisconnected) {
            LOG(INFO) << "kDisconnected";
            // g_loop->Exit();
        }
    });

    tcp_server.OnMessage([](const TcpConnectionPtr& conn, Buffer* buffer, Timestamp time) {
        std::string s = buffer->RetrieveAsString();
        std::cout << s << std::endl;
        conn->SendMessage(s);
    });


    tcp_server.OnWriteComplete([](const TcpConnectionPtr& conn) {
        conn->SendMessage("OnWriteComplete -> @@@123456");
    });

    tcp_server.Start(2);

    loop.Loop();
}

int main()
{
    FLAG_STDERR = true;
    
    func();

    return 0;
}
