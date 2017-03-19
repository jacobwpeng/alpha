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

#include <memory>
#include <alpha/Compiler.h>
#include <alpha/ProcessBus.h>
#include <alpha/Subprocess.h>

using ProcessBusPtr = std::unique_ptr<alpha::ProcessBus>;
using SubprocessPtr = std::unique_ptr<alpha::Subprocess>;
class NetSvrdWorker final {
 public:
  NetSvrdWorker(SubprocessPtr&& subprocess,
                ProcessBusPtr&& bus_in,
                ProcessBusPtr&& bus_out);
  DISABLE_COPY_ASSIGNMENT(NetSvrdWorker);

  alpha::Subprocess* Process() { return subprocess_.get(); }
  alpha::ProcessBus* BusIn() { return bus_in_.get(); }
  alpha::ProcessBus* BusOut() { return bus_out_.get(); }
  ProcessBusPtr&& ReleaseBusIn() { return std::move(bus_in_); }
  ProcessBusPtr&& ReleaseBusOut() { return std::move(bus_out_); }

 private:
  SubprocessPtr subprocess_;
  ProcessBusPtr bus_in_;
  ProcessBusPtr bus_out_;
};
using NetSvrdWorkerPtr = std::unique_ptr<NetSvrdWorker>;

