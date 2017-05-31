#pragma once

#include "any.h"
#include "Buffer.h"
#include "Callbacks.h"
#include "InetAddress.h"
#include "noncopyable.h"

#include <atomic>
#include <memory>

namespace buzz
{
    class Socket;
    class Channel;
    class EventLoop;

    class TcpConnection 
        : noncopyable , public std::enable_shared_from_this<TcpConnection>
    {
    public:
        enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };

        TcpConnection(EventLoop* loop, const std::string& name, int clnt_fd, 
                      const InetAddress& local_addr,
                      const InetAddress& peer_addr);

        ~TcpConnection();

        StateE GetState() { return m_state; }
        EventLoop* OwnerLoop() { return m_owner_loop; }

        const std::string Name() const { return m_name; }

        void OnError(const ErrorEventHandler&& handler)
        {
            m_error_event_handler = handler;
        }

        void OnClose(const CloseEventHandler&& handler)
        {
            m_close_event_handler = handler;
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
        
        void Close(double seconds = 0.0);
        void Shutdown();
        
        void ConnectDestroyed();
        void ConnectEstablished();

        void SendMessage(Buffer* message);
        void SendMessage(const std::string& message);
        void SendMessage(const void *message, size_t msg_len);
        
        void SetContex(any& contex) { m_contex = contex; }
        const any* GetContex() const { return &m_contex; }
    private:
        EventLoop*  m_owner_loop;
        
        const std::string        m_name;
        std::atomic<StateE>      m_state;
        std::unique_ptr<Socket>  m_sock;
        std::unique_ptr<Channel> m_channel;
        
        const InetAddress m_local_addr;
        const InetAddress m_peer_addr;

        ErrorEventHandler         m_error_event_handler;
        CloseEventHandler         m_close_event_handler;
        MessageEventHandler       m_message_event_handler;
        StateChangeEventHandler   m_state_change_event_handler;
        WriteCompleteEventHandler m_write_complete_event_handler;

        Buffer m_input_buffer;
        Buffer m_output_buffer;

        any m_contex;

        void HandlerClose();
        void HandlerShutdown();
        void HandleRead(Timestamp receiveTime);
        void HandleWrite();

        void SendBase(const void* messgae, size_t msg_len);        
    };
}