#include "Socket.h"
#include "Logger.h"
#include "Channel.h"
#include "EventLoop.h"
#include "TcpConnection.h"

#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>

using namespace buzz;

TcpConnection::TcpConnection(EventLoop* loop, const std::string& name, int clnt_fd, 
                             const InetAddress& local_addr,
                             const InetAddress& peer_addr)
    : m_owner_loop(loop),
    m_name(name),
    m_state(kConnecting),
    m_sock(new Socket(clnt_fd)),
    m_channel(new Channel(loop, clnt_fd, kNoneEvent)),
    m_local_addr(local_addr),
    m_peer_addr(peer_addr)
{
    m_sock->KeepAlive(true);
    
    m_channel->OnWrite(std::bind(&TcpConnection::HandleWrite, this));
    m_channel->OnRead(std::bind(&TcpConnection::HandleRead, this, std::placeholders::_1));

    LOG(DEBUG) << "TcpConnection::TcpConnection [" << name << "] at fd " << clnt_fd;
}

TcpConnection::~TcpConnection()
{
    LOG(DEBUG) << " TcpConnection::~TcpConnection [" << m_name << "] at " << this
               << " fd " << m_sock->GetFd();
    assert(m_state == kDisconnected);
}

void TcpConnection::Close(double seconds)
{
    StateE expected0 = kConnected;
    StateE expected1 = kDisconnecting;

    if (m_state.compare_exchange_strong(expected0, kDisconnecting) ||
        m_state.compare_exchange_strong(expected1, kDisconnecting)) {

        if (seconds == 0.0) {
            m_owner_loop->RunInLoop(std::bind(&TcpConnection::HandlerClose, shared_from_this()));
        } else {
            m_owner_loop->RunAfter(seconds, std::bind(&TcpConnection::HandlerClose, shared_from_this()));
        }
    }
}

void TcpConnection::Shutdown()
{
    StateE expected = kConnected;
    if (m_state.compare_exchange_strong(expected, kDisconnecting)) {
        m_owner_loop->RunInLoop(std::bind(&TcpConnection::HandlerShutdown, this));
    }
}

void TcpConnection::ConnectDestroyed()
{
    StateE expected = kConnected;

    if (m_state.compare_exchange_strong(expected, kDisconnected)) {
        m_channel->EnableReadWrite(false, false);
        if (m_state_change_event_handler) {
            m_state_change_event_handler(shared_from_this());
        }
    }
}

void TcpConnection::ConnectEstablished()
{
    m_owner_loop->RunInLoop([this] {
        assert(m_state == kConnecting);
        StateE expected = kConnecting;

        if (m_state.compare_exchange_strong(expected, kConnected)) {
            assert(m_state == kConnected);

            m_channel->EnableRead(true);

            if (m_state_change_event_handler) {
                m_state_change_event_handler(shared_from_this());
            }
        }
    });
}

void TcpConnection::SendMessage(Buffer* message)
{
    SendMessage(message->Peek(), message->ReadableBytes());
    message->RetrieveAll();
}

void TcpConnection::SendMessage(const std::string& message)
{
    SendMessage(static_cast<const void*>(message.data()), message.size());
}

void TcpConnection::SendMessage(const void* message, size_t msg_len)
{
    if (m_state == kConnected) {
        m_owner_loop->RunInLoop(std::bind(&TcpConnection::SendBase, this, message, msg_len));
    }
}

void TcpConnection::SendBase(const void* messgae, size_t msg_len)
{
    ssize_t nwtote = 0;
    ssize_t remaining = msg_len;
    bool fault_error = false;

    if (m_state == kDisconnected) {
        LOG(WARN) << "connection [" << m_name << "] disconnected, give up writing" ;
        return;
    }

    if (m_channel->WriteEnable() == false && m_output_buffer.ReadableBytes() == 0) {
        nwtote = ::write(m_sock->GetFd(), messgae, msg_len);
        if (nwtote >= 0) {
            remaining = msg_len - nwtote;
            if (remaining == 0) {
                if (m_write_complete_event_handler) {
                    m_write_complete_event_handler(shared_from_this());
                }
            }
        } else {
            nwtote = 0;
            if (errno != EWOULDBLOCK && (errno == EINTR || errno == ECONNRESET)) {
                fault_error = true;
            }
        }
    }

    assert(remaining < 0 || static_cast<size_t>(remaining) <= msg_len);
    if (fault_error == false && remaining > 0) {
        m_output_buffer.Append(static_cast<const char*>(messgae) + nwtote, remaining);
        if (m_channel->WriteEnable() == false) {
            m_channel->EnableWrite(true);
        }
    }
}

void TcpConnection::HandlerClose()
{
    assert(m_state == kConnected || m_state == kDisconnecting);
    m_state = kDisconnected;

    m_channel->EnableReadWrite(false, false);
    TcpConnectionPtr guard_this(shared_from_this());

    if (m_state_change_event_handler) {
        m_state_change_event_handler(guard_this);
    }

    m_owner_loop->RunInLoop(std::bind(m_close_event_handler, guard_this));
}

void TcpConnection::HandlerShutdown()
{
    if (m_channel->ReadEnable() == false) {
        m_sock->ShutdownWrite();
    }
}

void TcpConnection::HandleRead(Timestamp receiveTime)
{       
    int err_code = 0;
    ssize_t n = m_input_buffer.ReadFd(m_sock->GetFd() , &err_code);
    if (n > 0) {
        if (m_message_event_handler) {
            m_message_event_handler(shared_from_this(), &m_input_buffer, receiveTime);
        }
    } else if(n == 0) {
        HandlerClose();
    } else if (err_code != EAGAIN && err_code != EINTR) {
        socklen_t len = sizeof(err_code);

        ::getsockopt(m_sock->GetFd(), SOL_SOCKET, SO_ERROR, &err_code, &len);
        LOG(WARN) << "TcpConnection [" << m_name << "] SO_ERROR = " << err_code 
                  << " (" << ::strerror(err_code) << ')';
        
        if (m_error_event_handler) {
            m_error_event_handler(shared_from_this(), err_code);
        }
    }
}

void TcpConnection::HandleWrite()
{
    if (m_channel->WriteEnable()) {
        ssize_t nwtote = ::write(m_sock->GetFd(), m_output_buffer.Peek(), 
                                 m_output_buffer.ReadableBytes());

        if (nwtote > 0) {
            m_output_buffer.Retrieve(nwtote);
            if (m_output_buffer.ReadableBytes() == 0) {
                m_channel->EnableWrite(false);

                if (m_write_complete_event_handler) {
                    m_write_complete_event_handler(shared_from_this());
                }

                if (m_state == kDisconnecting) Shutdown();
            }
        } else {
            LOG(ERROR) << "TcpConnection::HandleWrite error " << errno << " ("
                       << ::strerror(errno) << ')';
        }
    } else {
        LOG(TRACE) << "connection fd " << m_sock->GetFd() << " is down, no more writing";
    }
}