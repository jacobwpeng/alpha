/*
 * =============================================================================
 *
 *       Filename:  MemoryMappedFile.cc
 *        Created:  07/26/16 09:55:13
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include <alpha/MemoryMappedFile.h>

#include <sys/mman.h>
#include <alpha/Logger.h>

namespace alpha {
MemoryMappedFile::~MemoryMappedFile() {
  if (mapped_start_) {
    CHECK(file_.Valid());
    ::munmap(mapped_start_, file_.GetLength());
  }
}

MemoryMappedFile::MemoryMappedFile(MemoryMappedFile&& other) { swap(other); }

MemoryMappedFile& MemoryMappedFile::operator=(MemoryMappedFile&& other) {
  swap(other);
  return *this;
}

bool MemoryMappedFile::Init(alpha::Slice filepath,
                            int64_t size,
                            unsigned flags) {
  CHECK(size >= 0);
  bool newly_created = false;
  alpha::File file(filepath, O_RDWR);
  if (!file) {
    if (errno != ENOENT) {
      PLOG_WARNING << "Open file failed, filepath: [" << filepath.ToString()
                   << "]";
      return false;
    } else if (flags && kCreateIfNotExists == 0) {
      return false;
    } else {
      // 创建文件
    }
  }

  if (!file) {
    alpha::File new_file(filepath, O_RDWR | O_CREAT | O_EXCL);
    if (!new_file) {
      PLOG_WARNING << "Create file failed, filepath: [" << filepath.ToString()
                   << "]";
      return false;
    }
    file = std::move(new_file);
    newly_created = true;
  }
  CHECK(file.Valid());

  if ((flags && kTruncate) || newly_created) {
    file.SetLength(size);
  }

  void* mem = ::mmap(
      NULL, file.GetLength(), PROT_READ | PROT_WRITE, MAP_SHARED, file.fd(), 0);
  if (mem == MAP_FAILED) {
    PLOG_WARNING << "mmap failed";
    return false;
  }

  newly_created_ = newly_created;
  mapped_start_ = mem;
  filepath_ = filepath.ToString();
  file_ = std::move(file);
  return true;
}

void MemoryMappedFile::swap(MemoryMappedFile& other) {
  std::swap(newly_created_, other.newly_created_);
  std::swap(mapped_start_, other.mapped_start_);
  std::swap(file_, other.file_);
  std::swap(filepath_, other.filepath_);
}

MemoryMappedFile::operator bool() const { return mapped_start_ != nullptr; }

void* MemoryMappedFile::mapped_start() { return mapped_start_; }

int64_t MemoryMappedFile::size() const { return file_.GetLength(); }

std::string MemoryMappedFile::filepath() const { return filepath_; }
}
