#include "Poller.h"
#include "Logger.h"
#include "Channel.h"
#include "Timestamp.h"

#include <set>
#include <vector>

#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>

namespace buzz
{
    const int kReadEvent  = EPOLLIN;
    const int kWriteEvent = EPOLLOUT;

    class PollerEpoll : public Poller
    {
    public:
        PollerEpoll();
        ~PollerEpoll();

        Timestamp Poll(int timeout, ChannelList* active_events) override;

        void AddChannel(Channel* channel)    override;
        void UpdateChannel(Channel* channel) override;
        void RemoveChannel(Channel* channel) override;

    private:
        int m_epfd;

        std::set<Channel*>              m_channels;
        std::vector<struct epoll_event> m_active_events;
    };

    PollerEpoll::PollerEpoll()
        :m_epfd(epoll_create1(EPOLL_CLOEXEC)),
        m_active_events(64)
    {
        if (m_epfd == -1) {
            LOG(FATAL) << "epoll_create1 error " << errno << " (" << ::strerror(errno) << ')';
        }

        LOG(DEBUG) << "poller epoll " << m_epfd << " created";
    }

    PollerEpoll::~PollerEpoll()
    {
        LOG(DEBUG) << "destroying PollerEpoll " << m_epfd;

        for (auto it : m_channels) { delete it; }
        ::close(m_epfd);
    }

    Timestamp PollerEpoll::Poll(int timeout, ChannelList* active_events)
    {
        assert(active_events);
        
        int nready = epoll_wait(m_epfd, m_active_events.data(), m_active_events.size(), timeout);

        if (nready == -1 && errno != EINTR) {
            LOG(FATAL) << "epoll_wait reutrn " << nready << " (" << ::strerror(errno) << ')';
        }

        Timestamp now(Timestamp::Now());

        for (int i = 0; i < nready; i++) {
            Channel* channel = static_cast<Channel*>(m_active_events[i].data.ptr);
            channel->SetRevents(m_active_events[i].events);

            active_events->push_back(channel);
        }

        if (static_cast<size_t>(nready) == m_active_events.size()) {
            m_active_events.resize(nready << 1);
        }

        return now;
    }

    void PollerEpoll::AddChannel(Channel* channel)
    {
        assert(channel);
        
        struct epoll_event ev[1];
        ev[0].data.ptr = channel;
        ev[0].events = channel->GetEvents();

        LOG(TRACE) << "adding channel " << channel->GetId()
                   << " fd " << channel->GetFd()
                   << " events " << channel->GetEvents() << " epoll " << m_epfd;

        int ret = epoll_ctl(m_epfd, EPOLL_CTL_ADD,channel->GetFd(), ev);
        if (ret == -1) {
            LOG(FATAL) << "epoll_ctl add failed " << errno << " (" << strerror(errno) << ')';
        }

        m_channels.insert(channel);
    }

    void PollerEpoll::UpdateChannel(Channel* channel)
    {
        assert(channel);

        struct epoll_event ev[1];
        ev[0].data.ptr = channel;
        ev[0].events = channel->GetEvents();

        LOG(TRACE) << "modifying channel " << channel->GetId()
                   << " fd " << channel->GetFd()
                   << " events " << channel->GetEvents() << " epoll " << m_epfd;

        int ret = epoll_ctl(m_epfd, EPOLL_CTL_MOD, channel->GetFd(), ev);
        if (ret == -1) {
            LOG(FATAL) << "epoll_ctl mod failed " << errno << " (" << strerror(errno) << ')';
        }
    }

    void PollerEpoll::RemoveChannel(Channel* channel)
    {
        assert(channel);

        struct epoll_event ev[1];
        ev[0].data.ptr = channel;
        ev[0].events = channel->GetEvents();

        LOG(TRACE) << "deleting channel " << channel->GetId()
                   << " fd " << channel->GetFd()
                   << " epoll " << m_epfd;


        epoll_ctl(m_epfd, EPOLL_CTL_DEL, channel->GetFd(), ev);

        m_channels.erase(channel);
    }

    Poller* MakePoller() { return new PollerEpoll(); }
}