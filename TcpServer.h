#pragma once

#include "Socket.h"
#include "Callbacks.h"
#include "InetAddress.h"
#include "noncopyable.h"
#include "EventLoopThreadPoll.h"

#include <map>
#include <atomic>
#include <string>
#include <memory>

namespace buzz
{
    class Channel;
    class EventLoop;

    class TcpServer : noncopyable
    {
    public:
        TcpServer(EventLoop* loop, const std::string& server_name, InetAddress& local_addr,
                  bool reuse_addr = true);

        ~TcpServer();

        void Start(size_t poll_size) { m_event_loop_poll.Start(poll_size); }

        void OnError(const ErrorEventHandler&& handler)
        {
            m_error_event_handler = handler;
        }

        void OnStateChange(const StateChangeEventHandler&& handler) 
        { 
            m_state_change_event_handler = handler;
        }

        void OnMessage(const MessageEventHandler&& handler)
        {
            m_message_event_handler = handler;
        }

        void OnWriteComplete(const WriteCompleteEventHandler&& handler)
        {
            m_write_complete_event_handler = handler;
        }
    private:
        Socket m_sock;
        
        EventLoop*  m_owner_loop;
        InetAddress m_local_addr;
        std::string m_server_name;
        std::string m_ip_port;

        std::atomic<uint64_t> m_id;
        std::unique_ptr<Channel> m_channel;

        EventLoopThreadPoll m_event_loop_poll;

        std::map<std::string, TcpConnectionPtr> m_connections;
        
        ErrorEventHandler         m_error_event_handler;
        MessageEventHandler       m_message_event_handler;
        StateChangeEventHandler   m_state_change_event_handler;
        WriteCompleteEventHandler m_write_complete_event_handler;

        void NewConnection();
        void RemoveConnection(const TcpConnectionPtr& conn);
    };
}