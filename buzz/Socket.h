#include "noncopyable.h"

namespace buzz
{
    class InetAddress;

    class Socket : noncopyable
    {
    public:
        Socket(int sock_fd) : m_sock_fd(sock_fd)
        { }

        ~Socket();

        int GetFd() const { return m_sock_fd; }

        void Listen();
        void Bind(InetAddress& local_addr);
        int  Accept(InetAddress& peer_addr);

        void ShutdownWrite();

        void KeepAlive(bool on);
        void ReuseAddr(bool on);
        void TcpNoDelay(bool on);
    private:
        int m_sock_fd;
    };

    int CreateNonBlockSocket();
}