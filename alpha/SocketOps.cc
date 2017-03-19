/*
 * =============================================================================
 *
 *       Filename:  SocketOps.cc
 *        Created:  04/05/15 14:16:41
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include <alpha/SocketOps.h>

#include <fcntl.h>
#include <sys/socket.h>
#include <alpha/Compiler.h>
#include <alpha/Logger.h>

namespace alpha {
namespace SocketOps {
void SetNonBlocking(int fd) {
  int opts = ::fcntl(fd, F_GETFL);
  if (unlikely(opts == -1)) {
    PLOG_WARNING << "fcntl F_GETFL fd = " << fd;
  }

  opts |= O_NONBLOCK;
  if (unlikely(::fcntl(fd, F_SETFL, opts) == -1)) {
    PLOG_WARNING << "fcntl F_SETFL fd = " << fd;
  }
}

void SetReuseAddress(int fd) {
  int enable_reuse_addr = 1;
  if (unlikely(::setsockopt(fd,
                            SOL_SOCKET,
                            SO_REUSEADDR,
                            &enable_reuse_addr,
                            sizeof(enable_reuse_addr)) == -1)) {
    PLOG_WARNING << "setsockopt failed";
  }
}

void SetReceiveTimeout(int fd, int microseconds) {
  static const int kMicroSecondsPerSecond = 1000000;
  struct timeval tv;
  tv.tv_sec = microseconds / kMicroSecondsPerSecond;
  tv.tv_usec = microseconds % kMicroSecondsPerSecond;
  if (unlikely(::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) ==
               -1)) {
    PLOG_WARNING << "setsockopt failed";
  }
}

int GetAndClearError(int fd) {
  int err;
  socklen_t len = sizeof(err);
  if (unlikely(::getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) == -1)) {
    PLOG_WARNING << "getsockopt fd = " << fd;
    return errno;
  } else {
    return err;
  }
}

void DisableReading(int fd) {
  if (unlikely(::shutdown(fd, SHUT_RD) == -1)) {
    PLOG_WARNING << "shutdown SHUT_RD failed, fd = " << fd;
  }
}

void DisableWriting(int fd) {
  if (unlikely(::shutdown(fd, SHUT_WR) == -1)) {
    PLOG_WARNING << "shutdown SHUT_WR failed, fd = " << fd;
  }
}
void DisableReadingAndWriting(int fd) {
  if (unlikely(::shutdown(fd, SHUT_RDWR) == -1)) {
    PLOG_WARNING << "shutdown SHUT_RDWR failed, fd = " << fd;
  }
}
}
}
