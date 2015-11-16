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
#include <alpha/format.h>
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
    : w(conn),
      codec_env(GetCodecEnv("")),
      fsm(alpha::make_unique<ConnectionEstablishFSM>(&w, codec_env)) {}

ConnectionMgr::ConnectionMgr() : tcp_client_(&loop_) {
  using namespace std::placeholders;
  tcp_client_.SetOnConnected(
      std::bind(&ConnectionMgr::OnTcpConnected, this, _1));
  tcp_client_.SetOnConnectError(
      std::bind(&ConnectionMgr::OnTcpConnectError, this, _1));
  tcp_client_.SetOnClose(std::bind(&ConnectionMgr::OnTcpClosed, this, _1));
}

ConnectionMgr::~ConnectionMgr() { Run(); }
void ConnectionMgr::CloseConnection(Connection* conn) {}

void ConnectionMgr::OnConnectionEstablished(Connection* conn) {
  if (connected_callback_) {
    connected_callback_(conn);
  }
}

void ConnectionMgr::ConnectTo(ConnectionParameters params) {
  tcp_client_.ConnectTo(alpha::NetAddress(params.host, params.port));
}

void ConnectionMgr::OnTcpConnected(alpha::TcpConnectionPtr conn) {
  DLOG_INFO << "Connected to remote, addr = " << conn->PeerAddr();
  auto p = connection_context_map_.insert(
      std::make_pair(conn.get(), alpha::make_unique<ConnectionContext>(conn)));
  CHECK(p.second) << "ConnectionContext already exists when TcpConnected";
  conn->SetContext(p.first->second.get());
  using namespace std::placeholders;
  conn->SetOnRead(std::bind(&ConnectionMgr::OnTcpMessage, this, _1, _2));
}

void ConnectionMgr::OnTcpMessage(alpha::TcpConnectionPtr conn,
                                 alpha::TcpConnectionBuffer* buffer) {
  auto p = conn->GetContext<ConnectionContext*>();
  CHECK(p && *p);
  auto ctx = *p;
  alpha::Slice data = buffer->Read();
  auto sz = data.size();
  auto frame = ctx->frame_reader.Read(data);
  buffer->ConsumeBytes(sz - data.size());
  if (frame) {
    auto status = ctx->fsm->HandleFrame(std::move(frame));
    switch (status) {
      case FSM::Status::kConnectionEstablished:
        ctx->conn = alpha::make_unique<Connection>(this);
        OnConnectionEstablished(ctx->conn.get());
        break;
      case FSM::Status::kWaitForWrite:
        using namespace std::placeholders;
        conn->SetOnWriteDone(
            std::bind(&ConnectionMgr::OnTcpWriteDone, this, _1));
        break;
      case FSM::Status::kWaitMoreFrame:
        break;
    }
  }
}

void ConnectionMgr::OnTcpWriteDone(alpha::TcpConnectionPtr conn) {
  DLOG_INFO << "Flush reply";
  auto p = conn->GetContext<ConnectionContext*>();
  CHECK(p && *p);
  auto ctx = *p;
  bool done = ctx->fsm->FlushReply();
  if (done) {
    conn->SetOnWriteDone(nullptr);
  }
}

void ConnectionMgr::OnTcpClosed(alpha::TcpConnectionPtr conn) {
  DLOG_INFO << "Connection to " << conn->PeerAddr() << " closed";
  auto num = connection_context_map_.erase(conn.get());
  CHECK(num == 1) << "Missing Context for this Connection";
}

void ConnectionMgr::OnTcpConnectError(const alpha::NetAddress& addr) {
  DLOG_INFO << "Cannot connect to " << addr;
}
}
