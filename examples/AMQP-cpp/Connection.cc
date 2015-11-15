/*
 * =============================================================================
 *
 *       Filename:  Connection.cc
 *        Created:  11/15/15 17:14:27
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "Connection.h"
#include "ConnectionMgr.h"

namespace amqp {
Connection::Connection(ConnectionMgr* owner) : owner_(owner) {}

void Connection::Close() { owner_->CloseConnection(this); }
}
