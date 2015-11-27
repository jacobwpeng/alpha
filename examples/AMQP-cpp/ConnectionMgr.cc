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
#include "Frame.h"
#include "ConnectionEstablishFSM.h"
#include "ConnectionCloseFSM.h"
#include "Connection.h"

namespace amqp {

using namespace std::placeholders;
ConnectionMgr::ConnectionContext::ConnectionContext(
    alpha::TcpConnectionPtr& conn)
    : w(conn),
      codec_env(GetCodecEnv("")),
      fsm(alpha::make_unique<ConnectionEstablishFSM>(
          codec_env, PlainAuthorization({"guest", "guest"}),
          ConnectionParameters())) {}

bool ConnectionMgr::ConnectionContext::FlushReply() {
  while (!frame_packers_.empty()) {
    auto it = frame_packers_.begin();
    bool done = it->WriteTo(&w);
    if (!done) {
      return false;
    }
    frame_packers_.erase(it);
  }
  return true;
}

ConnectionMgr::ConnectionMgr() : tcp_client_(&loop_) {
  tcp_client_.SetOnConnected(
      std::bind(&ConnectionMgr::OnTcpConnected, this, _1));
  tcp_client_.SetOnConnectError(
      std::bind(&ConnectionMgr::OnTcpConnectError, this, _1));
  tcp_client_.SetOnClose(std::bind(&ConnectionMgr::OnTcpClosed, this, _1));
}

ConnectionMgr::~ConnectionMgr() { Run(); }
void ConnectionMgr::CloseConnection(Connection* conn) {
  DLOG_INFO << "Try to close amqp::Connection";
  auto tcp_connection = conn->tcp_connection();
  if (tcp_connection) {
    auto p = tcp_connection->GetContext<ConnectionContext*>();
    CHECK(p && *p);
    auto ctx = *p;
    MethodCloseArgs method_close_args;
    method_close_args.reply_code = 0;
    method_close_args.reply_text = "Normal Shutdown";
    method_close_args.class_id = method_close_args.method_id = 0;
    ctx->fsm = ConnectionCloseFSM::ActiveClose(
        ctx->codec_env, ctx->send_reply_func, method_close_args);
    ctx->FlushReply();
  }
}

void ConnectionMgr::OpenNewChannel(Connection* conn, ChannelID channel_id) {
  auto tcp_connection = conn->tcp_connection();
  if (tcp_connection) {
    auto p = tcp_connection->GetContext<ConnectionContext*>();
    CHECK(p && *p);
    auto ctx = *p;
    ctx->send_reply_func(channel_id, Frame::Type::kMethod,
                         alpha::make_unique<MethodChannelOpenArgsEncoder>(
                             MethodChannelOpenArgs(), ctx->codec_env));
  }
}

void ConnectionMgr::OnConnectionEstablished(ConnectionPtr& conn) {
  DLOG_INFO << "AMQP Connection established";
  if (connected_callback_) {
    connected_callback_(conn);
  } else {
    DLOG_INFO << "No connected callback found, close it";
    conn->Close();
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
  const std::string amqp_protocol_header = {'A', 'M', 'Q', 'P', 0, 0, 9, 1};
  conn->Write(amqp_protocol_header);
  conn->SetContext(p.first->second.get());
  conn->SetOnRead(std::bind(&ConnectionMgr::OnTcpMessage, this, _1, _2));
  auto ctx = p.first->second.get();
  ctx->send_reply_func = [&ctx](ChannelID channel, Frame::Type type,
                                std::unique_ptr<EncoderBase>&& encoder) {
    ctx->frame_packers_.emplace_back(channel, type, std::move(encoder));
  };
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
  if (frame == nullptr) return;
  DLOG_INFO << "New frame, type: " << static_cast<int>(frame->type())
            << ", payload_size: " << frame->payload_size();
  ctx->fsm->HandleFrame(std::move(frame), ctx->send_reply_func);
  bool flushed = ctx->FlushReply();
  if (!flushed) {
    conn->SetOnWriteDone(std::bind(&ConnectionMgr::OnTcpWriteDone, this, _1));
  }
  if (flushed && ctx->fsm->done()) {
    auto p = ctx->fsm.get();
    if (dynamic_cast<ConnectionEstablishFSM*>(p)) {
      ctx->conn = std::make_shared<Connection>(this, conn);
      OnConnectionEstablished(ctx->conn);
    } else if (dynamic_cast<ConnectionCloseFSM*>(p)) {
      DLOG_INFO << "AMQP Connection closed";
      conn->Close();
    }
  }
}

void ConnectionMgr::OnTcpWriteDone(alpha::TcpConnectionPtr conn) {
  auto p = conn->GetContext<ConnectionContext*>();
  CHECK(p && *p);
  auto ctx = *p;
  bool flushed = ctx->FlushReply();
  if (flushed) {
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
