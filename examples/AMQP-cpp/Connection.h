/*
 * =============================================================================
 *
 *       Filename:  Connection.h
 *        Created:  11/15/15 17:12:36
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include <alpha/tcp_connection.h>

namespace amqp {
class ConnectionMgr;
class Connection {
 public:
  Connection(ConnectionMgr* owner, alpha::TcpConnectionPtr& conn);
  void Close();

 private:
  ConnectionMgr* owner_;
  alpha::TcpConnectionWeakPtr conn_;
};
}

#endif /* ----- #ifndef __CONNECTION_H__  ----- */
