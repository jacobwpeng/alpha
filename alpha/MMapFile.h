/*
 * ==============================================================================
 *
 *       Filename:  MMapFile.h
 *        Created:  12/21/14 13:42:52
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * ==============================================================================
 */

#ifndef __ALPHA_MMAP_FILE_H__
#define __ALPHA_MMAP_FILE_H__

#include <string>
#include <memory>
#include "compiler.h"
#include "slice.h"

namespace alpha {
class MMapFile {
 public:
  enum Flags {
    kDefault = 0,
    kTruncate = 1 << 0,
    kCreateIfNotExists = 1 << 1,
    kZeroClear = 1 << 2,
  };

 public:
  static std::unique_ptr<MMapFile> Open(Slice path, size_t size,
                                        int flags = kDefault);
  DISABLE_COPY_ASSIGNMENT(MMapFile);
  ~MMapFile();

  bool newly_created() const { return newly_created_; }

  void* start() const;
  void* end() const;
  size_t size() const;

 private:
  MMapFile();
  std::string path_;
  size_t size_;
  int fd_;
  void* start_;
  bool newly_created_;
};
}

#endif /* ----- #ifndef __ALPHA_MMAP_FILE_H__  ----- */
