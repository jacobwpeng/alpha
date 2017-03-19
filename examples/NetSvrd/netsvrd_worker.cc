/*
 * =============================================================================
 *
 *       Filename:  netsvrd_worker.cc
 *        Created:  12/11/15 21:27:45
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "netsvrd_worker.h"

NetSvrdWorker::NetSvrdWorker(SubprocessPtr&& subprocess,
                             ProcessBusPtr&& bus_in,
                             ProcessBusPtr&& bus_out)
    : subprocess_(std::move(subprocess)),
      bus_in_(std::move(bus_in)),
      bus_out_(std::move(bus_out)) {}
