/*
 * =============================================================================
 *
 *       Filename:  File.h
 *        Created:  01/16/16 13:57:19
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#pragma once

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <alpha/Compiler.h>
#include <alpha/Slice.h>
#include <alpha/IOBuffer.h>

namespace alpha {
class File {
 public:
  File();
  explicit File(int fd, bool owns_fd = false);
  explicit File(alpha::Slice path, int flags = O_RDONLY, mode_t mode = 0666);
  // Non-Copyable
  DISABLE_COPY_ASSIGNMENT(File);
  // Moveable
  File(File&& other);
  File& operator=(File&& other);
  ~File();

  bool owns_fd() const { return owns_fd_; }
  int fd() const { return fd_; }
  int Release();
  explicit operator bool() const { return fd_ != -1; }
  bool Valid() const { return fd_ != -1; }
  bool Close();
  void swap(File& other);

  int64_t Seek(int64_t offset, int whence);
  int ReadAt(int64_t offset, void* data, int size);
  int Read(void* data, int size);

  int WriteAt(int64_t offset, const void* data, int size);
  int Write(const void* data, int size);
  bool Flush();

  int64_t GetLength() const;
  bool SetLength(int64_t length);

  void Lock();
  bool TryLock();
  void UnLock();

  void LockShared();
  bool TryLockShared();

 private:
  bool owns_fd_;
  int fd_;
};
}
