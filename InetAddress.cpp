#include "Logger.h"
#include "InetAddress.h"

#include <string.h>

#include <arpa/inet.h>
#include <sys/socket.h>

using namespace buzz;

InetAddress::InetAddress(const std::string& ip, unsigned short port)
{
    bzero(&m_addr, sizeof(struct sockaddr_in));

    m_addr.sin_family = AF_INET;
    m_addr.sin_port = htons(port);

    if (ip.empty()) {
        m_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else if (ip == "localhost") {
        m_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    }
    else if (inet_pton(AF_INET, ip.c_str(), &m_addr.sin_addr) <= 0) {
        LOG(FATAL) << "invalid address " << ip;
    }
}

std::string InetAddress::ToString() const
{
    char str[32], portstr[8];

    ::inet_ntop(AF_INET, &m_addr.sin_addr, str, sizeof(str));

    if (m_addr.sin_port) {
        snprintf(portstr, sizeof(portstr), ":%u", ntohs(m_addr.sin_port));
        strcat(str, portstr);
    }

    return str;
}