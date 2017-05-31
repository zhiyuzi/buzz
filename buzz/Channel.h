#pragma once

#include "Callbacks.h"
#include "noncopyable.h"

namespace buzz
{
    const int kNoneEvent = 0;
    extern const int kReadEvent;
    extern const int kWriteEvent;

    class Poller;
    class EventLoop;

    class Channel : noncopyable
    {
    public:
        Channel(EventLoop* loop, int fd, int events);
        ~Channel();

        int GetFd() const { return m_fd; }
        int GetId() const { return m_id; }

        void EnableRead(bool enable);
        void EnableWrite(bool enable);
        void EnableReadWrite(bool readable, bool writable);

        bool ReadEnable()  { return m_events & kReadEvent; }
        bool WriteEnable() { return m_events & kWriteEvent; }

        void OnWrite(const EventHandler&& handler) { m_write_event_handler = handler; }
        void OnRead(const ReadEventHandler&& handler) { m_read_event_handler = handler; }

        void HandleEvent(Timestamp timestamp);

        int  GetEvents() { return m_events; }
        void SetRevents(int revents) { m_revents = revents; }
        
        EventLoop* GetOwnerLoop() { return m_owner_loop; }
    private:
        EventLoop* m_owner_loop;
        Poller*    m_poller;

        const int  m_fd;
        const int  m_id;

        int m_events;
        int m_revents;

        EventHandler     m_write_event_handler;
        ReadEventHandler m_read_event_handler;
    };
}