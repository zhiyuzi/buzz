#pragma once

#include "noncopyable.h"

#include <vector>

namespace buzz
{
    class Channel;
    class Timestamp;

    class Poller : noncopyable
    {
    public:
        typedef std::vector<Channel*> ChannelList;

        Poller() { }

        virtual ~Poller() { }

        virtual Timestamp Poll(int timeout, ChannelList* active_events) = 0;

        virtual void AddChannel(Channel* channel) = 0;
        virtual void UpdateChannel(Channel* channel) = 0;
        virtual void RemoveChannel(Channel* channel) = 0;
    };

    Poller* MakePoller();
}
