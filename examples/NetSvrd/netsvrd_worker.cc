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

NetSvrdWorker::NetSvrdWorker(const alpha::Subprocess& subprocess,
                             alpha::ProcessBus&& bus)
    : subprocess_(subprocess), bus_(std::move(bus)) {}
