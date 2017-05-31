#include "Buffer.h"

#include <errno.h>
#include <sys/uio.h>

using namespace buzz;

ssize_t Buffer::ReadFd(int fd, int* err_code)
{
    char extrabuf[65536];
    struct iovec vec[2];

    const size_t writable = WritableBytes();
    
    vec[0].iov_base = Begin() + m_writer_index;
    vec[0].iov_len  = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len  = sizeof(extrabuf);

    const int iov_cnt = (writable < sizeof(extrabuf)) ? 2 : 1;

    const ssize_t n = ::readv(fd, vec, iov_cnt);
    
    if (n == -1) {
        *err_code = errno;
    } else if (static_cast<size_t>(n) <= writable) {
        m_writer_index += n;
    } else {
        m_writer_index = m_buffer.size();
        Append(extrabuf, n - writable);
    }

    return n;
}