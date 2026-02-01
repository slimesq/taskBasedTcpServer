#pragma once
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string>
#include "utils/NonCopyable.h"

class InetAddress : public NonCopyable
{
public:
    InetAddress(std::string const& _ip, unsigned short _port);
    InetAddress(const struct sockaddr_in& _addr);
    ~InetAddress();

    /**
     * @brief Get IP address in string format.
     *
     * @return std::string Ip address.
     */
    std::string getIp() const;

    /**
     * @brief Get port number.
     *
     * @return unsigned short Port number.
     */
    unsigned short getPort() const;

    /**
     * @brief Get pointer to sockaddr_in structure.
     *
     * @return struct sockaddr_in* Sockaddr_in pointer.
     */
    struct sockaddr_in* getInetAddressPtr();

private:
    /**
     * @brief Socket address structure for IPv4.
     *
     */
    struct sockaddr_in m_addr;
};