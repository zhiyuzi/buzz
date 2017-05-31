#include "Logger.h"
#include "Channel.h"
#include "TcpServer.h"
#include "EventLoop.h"
#include "TcpConnection.h"

#include <errno.h>
#include <string.h>
#include <sys/socket.h>

using namespace buzz;

TcpServer::TcpServer(EventLoop* loop, const std::string& server_name, InetAddress& local_addr, 
                     bool reuse_addr)
    : m_sock(CreateNonBlockSocket()),
    m_owner_loop(loop),
    m_local_addr(local_addr),
    m_server_name(server_name),
    m_ip_port(local_addr.ToString()),
    m_id(0),
    m_channel(new Channel(loop, m_sock.GetFd(), kReadEvent)),
    m_event_loop_poll(m_owner_loop)
{
    m_sock.ReuseAddr(reuse_addr);

    m_sock.Bind(m_local_addr);
    m_sock.Listen();

    m_channel->OnRead(std::bind(&TcpServer::NewConnection, this));
}

TcpServer::~TcpServer() 
{
    m_owner_loop->AssertInLoopThread();

    LOG(TRACE) << "TcpServer::~TcpServer [" << m_server_name << "] destructing";

    for (auto it : m_connections) {
        TcpConnectionPtr conn = it.second;

        it.second.reset();
        conn->OwnerLoop()->RunInLoop(std::bind(&TcpConnection::ConnectDestroyed, conn));
        
        conn.reset();
    }
}

void TcpServer::NewConnection()
{
    InetAddress peer_addr;

    int clnt_fd = m_sock.Accept(peer_addr);
    if (clnt_fd < 0 && errno != EAGAIN && errno != EINTR) {
        LOG(WARN) << "bad accept " << errno << " (" << ::strerror(errno) << ')';
        return;
    }

    char buf[128];
    snprintf(buf, sizeof(buf), " %s@%" PRIu64, m_ip_port.c_str(), m_id++);

    std::ostringstream oss;
    oss << m_server_name << buf;
    
    const std::string& conn_name = oss.str();
    LOG(INFO) << "new connection [" << conn_name << "] from " << peer_addr.ToString();
    
    EventLoop* io_loop = m_event_loop_poll.Get();

    TcpConnectionPtr conn(new TcpConnection(io_loop, conn_name, clnt_fd, m_local_addr, peer_addr));
    m_connections[conn_name] = conn;
    
    conn->OnStateChange(std::move(m_state_change_event_handler));
    conn->OnError(std::move(m_error_event_handler));
    conn->OnMessage(std::move(m_message_event_handler));
    conn->OnWriteComplete(std::move(m_write_complete_event_handler));
    conn->OnClose(std::bind(&TcpServer::RemoveConnection, this, std::placeholders::_1));
    
    io_loop->RunInLoop(std::bind(&TcpConnection::ConnectEstablished, conn));
}

void TcpServer::RemoveConnection(const TcpConnectionPtr& conn)
{
    LOG(INFO) << "TcpServer::RemoveConnection [" << m_server_name << "] - connection "
              << conn->Name();
    
    size_t n = m_connections.erase(conn->Name());
    assert(n == 1); (void) n;
}