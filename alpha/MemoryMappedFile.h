/*
 * =============================================================================
 *
 *       Filename:  MemoryMappedFile.h
 *        Created:  07/26/16 09:48:52
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#pragma once

#include <alpha/slice.h>
#include <alpha/File.h>
#include <alpha/compiler.h>

namespace alpha {

struct MemoryMappedRegion {
  uint64_t offset;
  uint64_t size;
};

enum MemoryMappedFlags {
  kDefault = 0,
  kTruncate = 1,
  kCreateIfNotExists = 1 << 1,
  kZeroClear = 1 << 2,
};

class MemoryMappedFile {
 public:
  MemoryMappedFile() = default;

  ~MemoryMappedFile();

  MemoryMappedFile(MemoryMappedFile&& other);

  MemoryMappedFile& operator=(MemoryMappedFile&& other);

  DISABLE_COPY_ASSIGNMENT(MemoryMappedFile);

  bool Init(alpha::Slice filepath,
            int64_t size,
            unsigned flags = MemoryMappedFlags::kDefault);

  void swap(MemoryMappedFile& other);

  void* mapped_start();

  int64_t size() const;

  operator bool() const;

  std::string filepath() const;

  bool newly_created() const { return newly_created_; }

 private:
  bool newly_created_{false};
  void* mapped_start_{nullptr};
  alpha::File file_;
  std::string filepath_;
};
}
