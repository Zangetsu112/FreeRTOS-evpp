// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)
//

// Modified : zieckey (zieckey at gmail dot com)

#include "inner_pre.h"
#include "evpp_buffer.h"
#include "FreeRTOS_Sockets.h"

const char Buffer::kCRLF[] = "\r\n";

const size_t Buffer::kCheapPrependSize = 8;
const size_t Buffer::kInitialSize  = 1024;

ssize_t Buffer::ReadFromFD(Socket_t fd, int* savedErrno) {
    // saved an ioctl()/FIONREAD call to tell how much to read
    // char extrabuf[65536];
    // struct iovec vec[2];
    // const size_t writable = WritableBytes();
    // vec[0].iov_base = begin() + write_index_;
    // vec[0].iov_len = writable;
    // vec[1].iov_base = extrabuf;
    // vec[1].iov_len = sizeof extrabuf;
    // // when there is enough space in this buffer, don't read into extrabuf.
    // // when extrabuf is used, we read 64k bytes at most.
    // const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
    // const ssize_t n = ::readv(fd, vec, iovcnt);

    // if (n < 0) {
    //     *savedErrno = errno;
    // } else if (static_cast<size_t>(n) <= writable) {
    //     write_index_ += n;
    // } else {
    //     write_index_ = capacity_;
    //     Append(extrabuf, n - writable);
    // }

    // return n;

    char extrabuf[65536];
    const size_t writable = WritableBytes();
    int32_t n = 0;

    // First, try to read directly into the buffer
    if (writable > 0) {
        n = FreeRTOS_recv(fd, begin() + write_index_, writable, 0);
        if (n > 0) {
            write_index_ += n;
        }
    }

    // If we filled the buffer or couldn't read, try the extrabuf
    if (n == 0 || (n > 0 && static_cast<size_t>(n) == writable)) {
        int32_t extra = FreeRTOS_recv(fd, extrabuf, sizeof(extrabuf), 0);
        if (extra > 0) {
            Append(extrabuf, extra);
            n += extra;
        }
    }

    if (n < 0) {
        *savedErrno = n;
    }

    return n;
}