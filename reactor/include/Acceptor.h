#pragma once
#include "InetAddress.h"
#include "utils/NonCopyable.h"
#include "Socket.h"

class InetAddress;

class Acceptor : public NonCopyable
{
public:
    Acceptor(std::string const& _ip, unsigned short _port);
    ~Acceptor();

    /**
     * @brief Get the listening socket fd.
     *
     * @return int Listening socket fd.
     */
    int getFd() const;

    /**
     * @brief Accept a new connection.
     *
     * @return int The new connection fd.
     */
    int accept();

    /**
     * @brief Ready the acceptor to accept connections.include setting socket options, binding and
     * listening.
     *
     */
    void ready();

private:
    /**
     * @brief set socket options: REUSEADDR
     *
     */
    void setReuseAddr();

    /**
     * @brief set socket options: REUSEPORT
     *
     */
    void setReusePort();

    /**
     * @brief Bind the socket to the specified address and port.
     *
     */
    void bind();

    /**
     * @brief Listen for incoming connections.
     *
     */
    void listen();

private:
    /**
     * @brief socket object for to bind listening address.
     *
     */
    Socket m_sock;

    /**
     * @brief address for to listen.
     *
     */
    InetAddress m_addr;
};