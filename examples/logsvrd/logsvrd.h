/*
 * ==============================================================================
 *
 *       Filename:  logsvrd.h
 *        Created:  12/24/14 14:49:09
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * ==============================================================================
 */

#pragma once

#include <memory>
#include <atomic>
#include <vector>
#include <alpha/Compiler.h>
#include <alpha/Slice.h>

namespace alpha {
class BusManager;

class LogServer {
 public:
  LogServer();
  bool InitFromFile(const alpha::Slice& filepath);
  int Run();

 private:
  int CreateLogDir();
  void RegisterSignalHandler();
  void CreateAndWaitWorkers();

 private:
  std::unique_ptr<alpha::BusManager> bus_mgr_;
  std::string log_dir_;
  std::vector<int> bus_ids_;
  DISABLE_COPY_ASSIGNMENT(LogServer);
};
}

