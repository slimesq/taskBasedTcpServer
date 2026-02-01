#pragma once
#include <sys/types.h>
#include "utils/NonCopyable.h"

class SocketIO : public NonCopyable
{
public:
    explicit SocketIO(int _fd);
    ~SocketIO();

    /**
     * @brief Read n bytes from socket.
     *
     * @param[out] _buf Buffer to store reading data.
     * @param[in] _count Number of bytes to read.
     * @return ssize_t Number of bytes read.
     */
    ssize_t readn(void* _buf, size_t _count);

    /**
     * @brief Write n bytes to socket.
     *
     * @param[in] _buf Buffer to write.
     * @param[in] _count Number of bytes to write.
     * @return ssize_t Number of bytes written.
     */
    ssize_t writen(void const* _buf, size_t _count);

    /**
     * @brief Read a line from socket.
     *
     * @param[out] _buf Buffer to store reading data.
     * @param[in] _maxCount Maximum number of bytes to read.
     * @return ssize_t Number of bytes read.
     */
    ssize_t readline(void* _buf, size_t _maxCount);

private:
    int m_fd;
};