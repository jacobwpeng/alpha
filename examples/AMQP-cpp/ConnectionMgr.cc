/*
 * =============================================================================
 *
 *       Filename:  ConnectionMgr.cc
 *        Created:  11/15/15 17:18:31
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "ConnectionMgr.h"

#include <alpha/logger.h>
#include "ConnectionEstablishFSM.h"
#include "Connection.h"

namespace amqp {

ConnectionParameters::ConnectionParameters()
    : host("127.0.0.1"),
      port(5672),
      vhost("/"),
      channel_max(0),          // Unlimited channel num (up to 65535)
      frame_max(1 << 17),      // 128K same as default RabbitMQ configure
      heartbeat_delay(600),    // 600s same as default RabbitMQ configure
      connection_attempts(3),  // Try to connect 3 times at most
      socket_timeout(3000)     // 3000ms before timeout
{}

ConnectionMgr::ConnectionContext::ConnectionContext(
    alpha::TcpConnectionPtr& conn)
    : w(conn), codec_env(nullptr), fsm(nullptr) {}

ConnectionMgr::ConnectionContext::~ConnectionContext() {
  if (fsm) {
    delete fsm;
  }
}

ConnectionMgr::ConnectionMgr() : tcp_client_(&loop_) {}
void ConnectionMgr::CloseConnection(Connection* conn) { (void)conn; }

void ConnectionMgr::ConnectTo(ConnectionParameters params) {
  tcp_client_.ConnectTo(alpha::NetAddress(params.host, params.port));
}

void ConnectionMgr::OnTcpConnected(alpha::TcpConnectionPtr conn) {
  DLOG_INFO << "Connected to remote, addr = " << conn->PeerAddr();
  auto ctx = new ConnectionContext(conn);
  conn->SetContext(ctx);
}
void ConnectionMgr::OnTcpClosed(alpha::TcpConnectionPtr conn) {
  DLOG_INFO << "Connection to " << conn->PeerAddr() << " closed";
  auto p = conn->GetContext<ConnectionContext*>();
  CHECK(p && *p) << "Null connection context";
  conn->SetContext(nullptr);
  delete *p;
}
void ConnectionMgr::OnTcpConnectError(const alpha::NetAddress& addr) {
  DLOG_INFO << "Cannot connect to " << addr;
}
}
