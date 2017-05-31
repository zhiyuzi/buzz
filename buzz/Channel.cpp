#include "Logger.h"
#include "Poller.h"
#include "Channel.h"
#include "Timestamp.h"
#include "EventLoop.h"

#include <atomic>

using namespace buzz;

std::atomic<uint64_t> g_channel_id(0);

Channel::Channel(EventLoop* loop, int fd, int events)
    :m_owner_loop(loop),
    m_poller(loop->GetPoller()),
    m_fd(fd),
    m_id(g_channel_id++),
    m_events(events)
{
    assert(m_owner_loop);

    m_poller->AddChannel(this);
}

Channel::~Channel()
{
    m_poller->RemoveChannel(this);
}

void Channel::EnableRead(bool enable)
{
    if (enable) {
        m_events |= kReadEvent;
    } else {
        m_events &= (~kReadEvent);
    }
    
    m_poller->UpdateChannel(this);
}

void Channel::EnableWrite(bool enable)
{
    if (enable) {
        m_events |= kWriteEvent;
    } else {
        m_events &= (~kWriteEvent);
    }

    m_poller->UpdateChannel(this);
}

void Channel::EnableReadWrite(bool readable, bool writable)
{
    if (readable) {
        m_events |= kReadEvent;
    } else {
        m_events &= (~kReadEvent);
    }

    if (writable) {
        m_events |= kWriteEvent;
    } else {
        m_events &= (~kWriteEvent);
    }

    m_poller->UpdateChannel(this);
}

void Channel::HandleEvent(Timestamp timestamp)
{
    if (m_revents & kReadEvent) {
        LOG(TRACE) << "channel " << m_id << " fd " << m_fd << " handle read";
        if (m_read_event_handler)  m_read_event_handler(timestamp);
    } else if (m_revents & kWriteEvent) {
        LOG(TRACE) << "channel " << m_id << " fd " << m_fd << " handle wrte";
        if (m_write_event_handler) m_write_event_handler();
    } else {
        LOG(FATAL) << "Channel::HandleEvent unexpected poller events";
    }
}