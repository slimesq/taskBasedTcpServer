#include <InetAddress.h>
#include <cstring>

InetAddress::InetAddress(std::string const& _ip, unsigned short _port)
{
    std::memset(&(this->m_addr),0,sizeof(this->m_addr));
    this->m_addr.sin_family = AF_INET;
    this->m_addr.sin_port = htons(_port);
    this->m_addr.sin_addr.s_addr = inet_addr(_ip.c_str());
}

InetAddress::InetAddress(const struct sockaddr_in& _addr) : m_addr(_addr)
{
}

InetAddress::~InetAddress()
{
}

std::string InetAddress::getIp() const
{
    return std::string(inet_ntoa(this->m_addr.sin_addr));
}

unsigned short InetAddress::getPort() const
{
    return ntohs(this->m_addr.sin_port);
}

struct sockaddr_in* InetAddress::getInetAddressPtr()
{
    return &(this->m_addr);
}