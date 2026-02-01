#pragma once

#include "utils/NonCopyable.h"

class Socket : public NonCopyable
{
public:
    Socket();
    explicit Socket(int _fd);
    ~Socket();

    /**
     * @brief Get the socket file descriptor.
     *
     * @return int Socket file descriptor.
     */
    int getFd() const noexcept;

private:
    /**
     * @brief socket file descriptor.
     *
     */
    int m_fd{};
};