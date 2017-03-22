/*
 * =============================================================================
 *
 *       Filename:  netsvrd_worker.h
 *        Created:  12/11/15 21:20:30
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#pragma once

#include <alpha/Compiler.h>
#include <alpha/ProcessBus.h>
#include <alpha/Subprocess.h>

class NetSvrdWorker final {
 public:
  NetSvrdWorker(const alpha::Subprocess& subprocess, alpha::ProcessBus&& bus);
  DISABLE_COPY_ASSIGNMENT(NetSvrdWorker);

  alpha::Subprocess* Process() { return &subprocess_; }
  alpha::ProcessBus* bus() { return &bus_; }

 private:
  alpha::Subprocess subprocess_;
  alpha::ProcessBus bus_;
};
using NetSvrdWorkerPtr = std::unique_ptr<NetSvrdWorker>;
