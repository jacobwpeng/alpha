/*
 * =============================================================================
 *
 *       Filename:  File.cc
 *        Created:  01/16/16 14:03:40
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include <alpha/File.h>
#include <sys/file.h>
#include <alpha/ScopedGeneric.h>

namespace alpha {
namespace detail {
template <class F, class... Args>
ssize_t WrapNoInt(F f, Args... args) {
  ssize_t r;
  do {
    r = f(args...);
  } while (r == -1 && errno == EINTR);
  return r;
}
}
File::File() : owns_fd_(false), fd_(-1) {}

File::File(int fd, bool owns_fd) : owns_fd_(owns_fd), fd_(fd) {}

File::File(alpha::Slice path, int flags, mode_t mode) : File() {
  alpha::ScopedFD scoped_fd(::open(path.data(), flags, mode));
  if (scoped_fd.is_valid()) {
    owns_fd_ = true;
    fd_ = scoped_fd.Release();
  }
}

File::File(File&& other) { swap(other); }

File& File::operator=(File&& other) {
  swap(other);
  return *this;
}

File::~File() {
  if (owns_fd_) {
    Close();
  }
}

int File::Release() {
  int fd = fd_;
  fd_ = -1;
  owns_fd_ = false;
  return fd;
}

bool File::Close() {
  int err = owns_fd_ ? ::close(fd_) : 0;
  Release();
  return err == 0;
}

void File::swap(File& other) {
  std::swap(fd_, other.fd_);
  std::swap(owns_fd_, other.owns_fd_);
}

int64_t File::Seek(int64_t offset, int whence) {
  static_assert(sizeof(int64_t) == sizeof(off_t), "off_t must be 64 bits");
  return ::lseek(fd_, static_cast<off_t>(offset), whence);
}

int File::ReadAt(int64_t offset, void* data, int size) {
  assert(Valid());
  int nread = 0;
  int rc = 0;
  auto p = reinterpret_cast<uintptr_t>(data);
  do {
    rc = pread(
        fd_, reinterpret_cast<void*>(p + nread), size - nread, offset + nread);
    if (rc <= 0) {
      // I/O operations on disk devices are not interrupted by signals.
      // so no EINTR here.
      break;
    }
    nread += rc;
  } while (nread < size);
  return nread ? nread : rc;
}

int File::Read(void* data, int size) {
  assert(Valid());
  int nread = 0;
  int rc = 0;
  auto p = reinterpret_cast<uintptr_t>(data);
  do {
    rc = read(fd_, reinterpret_cast<void*>(p + nread), size - nread);
    if (rc <= 0) {
      // I/O operations on disk devices are not interrupted by signals.
      // so no EINTR here.
      break;
    }
    nread += rc;
  } while (nread < size);
  return nread ? nread : rc;
}

int File::WriteAt(int64_t offset, const void* data, int size) {
  assert(Valid());
  int nwritten = 0;
  int rc = 0;
  auto p = reinterpret_cast<uintptr_t>(data);
  do {
    rc = pwrite(fd_,
                reinterpret_cast<const void*>(p + nwritten),
                size - nwritten,
                offset + nwritten);
    if (rc <= 0) {
      // I/O operations on disk devices are not interrupted by signals.
      // so no EINTR here.
      break;
    }
    nwritten += rc;
  } while (nwritten < size);
  return nwritten ? nwritten : rc;
}

int File::Write(const void* data, int size) {
  assert(Valid());
  int nwritten = 0;
  int rc = 0;
  auto p = reinterpret_cast<uintptr_t>(data);
  do {
    rc = write(
        fd_, reinterpret_cast<const void*>(p + nwritten), size - nwritten);
    if (rc <= 0) {
      // I/O operations on disk devices are not interrupted by signals.
      // so no EINTR here.
      break;
    }
    nwritten += rc;
  } while (nwritten < size);
  return nwritten ? nwritten : rc;
}

bool File::Flush() {
  assert(Valid());
  int rc = ::fdatasync(fd_);
  return rc != -1;
}

int64_t File::GetLength() const {
  assert(Valid());
  struct stat buf;
  fstat(fd_, &buf);
  return buf.st_size;
}

bool File::SetLength(int64_t length) {
  assert(Valid());
  return ftruncate(fd_, length) == 0;
}

void File::Lock() {
  assert(Valid());
  detail::WrapNoInt(flock, fd_, LOCK_EX);
}

bool File::TryLock() {
  assert(Valid());
  int rc = detail::WrapNoInt(flock, fd_, LOCK_EX | LOCK_NB);
  if (rc == -1 && errno == EWOULDBLOCK) return false;
  assert(rc == 0);
  return true;
}

void File::UnLock() {
  assert(Valid());
  detail::WrapNoInt(flock, fd_, LOCK_UN);
}

void File::LockShared() {
  assert(Valid());
  detail::WrapNoInt(flock, fd_, LOCK_SH);
}

bool File::TryLockShared() {
  assert(Valid());
  int rc = detail::WrapNoInt(flock, fd_, LOCK_SH | LOCK_NB);
  if (rc == -1 && errno == EWOULDBLOCK) return false;
  assert(rc == 0);
  return true;
}
}
