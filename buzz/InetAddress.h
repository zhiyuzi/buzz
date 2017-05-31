#pragma once

#include <string>
#include <netinet/in.h>

namespace buzz
{
    class InetAddress
    {
    public:
        InetAddress()
        { }

        InetAddress(unsigned short port, bool loopback_only = false)
            : InetAddress(loopback_only ? "localhost" : "", port)
        { }

        InetAddress(const std::string& ip, unsigned short port);

        InetAddress(struct sockaddr_in& addr) : m_addr(addr)
        { }

        std::string ToString() const;

        struct ::sockaddr_in& GetSockAddrInet() { return m_addr; }
        void SetSockAddrInet(struct ::sockaddr_in& addr) { m_addr = addr; }
    private:
        struct ::sockaddr_in m_addr;
    };
}