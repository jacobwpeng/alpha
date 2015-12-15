/*
 * ==============================================================================
 *
 *       Filename:  MMapFile.cc
 *        Created:  12/21/14 13:46:15
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * ==============================================================================
 */

#include "MMapFile.h"

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <memory.h>
#include <cstdio>
#include <cassert>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

namespace alpha {
std::unique_ptr<MMapFile> MMapFile::Open(Slice path, size_t size, int flags) {
  int open_flags = O_RDWR;
  const int kDefaultMode = 0644;
  int fd = ::open(path.data(), open_flags, kDefaultMode);
  if (fd == -1) {
    if (errno != ENOENT) {
      ::perror("open");
      return nullptr;
    } else if (!(flags & kCreateIfNotExists)) {
      return nullptr;
    }
  }

  bool newly_created = false;
  if (fd == -1) {
    assert(flags & kCreateIfNotExists);
    open_flags |= O_CREAT;
    open_flags |= O_EXCL;
    fd = ::open(path.data(), open_flags, kDefaultMode);
    if (fd == -1) {
      ::perror("open");
      return nullptr;
    }
    newly_created = true;
  }

  off_t real_size;
  if ((flags & kTruncate) || newly_created) {
    ::ftruncate(fd, size);
    real_size = size;
  } else {
    struct stat sb;
    if (fstat(fd, &sb) < 0) {
      close(fd);
      perror("fstat");
      return nullptr;
    }
    real_size = sb.st_size;
  }

  void* mem =
      ::mmap(NULL, real_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (mem == MAP_FAILED) {
    ::close(fd);
    perror("mmap");
    return nullptr;
  }

  if (flags & kZeroClear) {
    ::memset(mem, 0x0, real_size);
  }

  std::unique_ptr<MMapFile> file(new MMapFile);
  file->path_ = path.ToString();
  file->size_ = real_size;
  file->fd_ = fd;
  file->start_ = mem;
  file->newly_created_ = newly_created;
  return std::move(file);
}

MMapFile::~MMapFile() {
  if (fd_ != -1) {
    ::munmap(start_, size_);
    close(fd_);
  }
}

MMapFile::MMapFile() : fd_(-1) {}

void* MMapFile::start() const { return start_; }

void* MMapFile::end() const { return reinterpret_cast<char*>(start_) + size_; }

size_t MMapFile::size() const { return size_; }
}
