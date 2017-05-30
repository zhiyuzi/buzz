#include "Socket.h"
#include "Logger.h"
#include "InetAddress.h"

#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/tcp.h>

using namespace buzz;

int buzz::CreateNonBlockSocket()
{
    int sock_fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                           IPPROTO_TCP);

    if (sock_fd == -1) {
        LOG(FATAL) << "create socket failed" << errno << " ("
                   << ::strerror(errno) << ')';
    }

    return sock_fd;
}

Socket::~Socket() { ::close(m_sock_fd); }

void Socket::Bind(InetAddress& local_addr)
{
    struct ::sockaddr_in& addr = local_addr.GetSockAddrInet();
    int ret = ::bind(m_sock_fd, (struct sockaddr*)(&addr), sizeof(addr));
    if (ret == -1) {
        LOG(FATAL) << "socket " << m_sock_fd << " bind address "
                   << local_addr.ToString() << " failed "
                   << errno << " (" << ::strerror(errno) << ')';
    }
}

void Socket::Listen()
{
    int ret = ::listen(m_sock_fd, SOMAXCONN);
    if (ret == -1) {
        LOG(FATAL) << "listen socket " << m_sock_fd << " failed "
                   << errno << " (" << ::strerror(errno) << ')';
    }
}

int Socket::Accept(InetAddress& peer_addr)
{
    struct ::sockaddr_in addr;
    bzero(&addr, sizeof(addr));

    socklen_t len = sizeof(addr);

    int clnt_fd = ::accept4(m_sock_fd, (struct sockaddr*)(&addr), &len,
                            SOCK_NONBLOCK | SOCK_CLOEXEC);
    peer_addr.SetSockAddrInet(addr);

    return clnt_fd;
}

void Socket::ShutdownWrite()
{
    int ret = ::shutdown(m_sock_fd, SHUT_WR);
    if (ret < 0) {
        LOG(ERROR) << "Socket::ShutdownWrite " << m_sock_fd << " failed";
    }
}

void Socket::ReuseAddr(bool on)
{
    int opt = on ? 1 : 0;
    int ret = ::setsockopt(m_sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (ret < 0) {
        LOG(FATAL) << "SO_REUSEADDR failed " << errno << " ("
                   << ::strerror(errno) << ')';
    }
}

void Socket::TcpNoDelay(bool on)
{
    int opt = on ? 1 : 0;
    ::setsockopt(m_sock_fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
}

void Socket::KeepAlive(bool on)
{
    int opt = on ? 1 : 0;
    ::setsockopt(m_sock_fd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));
}