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
#include <alpha/AsyncTcpConnectionException.h>
#include "MethodPayloadCodec.h"
#if 0
#include "Frame.h"
#include "ConnectionEstablishFSM.h"
#include "ConnectionCloseFSM.h"
#include "Connection.h"
#include "CodedInputStream.h"
#endif

namespace amqp {
ConnectionMgr::ConnectionMgr(alpha::EventLoop* loop)
    : loop_(loop), async_tcp_client_(loop) {}

void ConnectionMgr::ConnectTo(const ConnectionParameters& params,
                              const PlainAuthorization& auth) {
  async_tcp_client_.RunInCoroutine([this, params, auth](
      alpha::AsyncTcpClient* client, alpha::AsyncTcpConnectionCoroutine* co) {
    auto conn =
        client->ConnectTo(alpha::NetAddress(params.host, params.port), co);
    if (!conn) {
      DLOG_INFO << "Connect to " << params.host << ":" << params.port
                << " failed";
      return;
    }
    const std::string amqp_protocol_header = {'A', 'M', 'Q', 'P', 0, 0, 9, 1};
    try {
      // Send protocol header
      conn->Write(amqp_protocol_header);
      FrameReader frame_reader(conn.get());
      FrameWriter frame_writer(conn.get());

      // Receive Start
      auto frame = frame_reader.Read();
      MethodStartArgsDecoder start_ok_decoder(GetCodecEnv(""));
      start_ok_decoder.Decode(std::move(frame));
      const CodecEnv* codec_env = GetCodecEnv("");

      AsyncTcpConnectionWriter w(conn.get());
      // Send Start-OK
      MethodStartOkArgs start_ok_args;
      start_ok_args.mechanism = "PLAIN";
      start_ok_args.locale = "en_US";
      start_ok_args.response.push_back('\0');
      start_ok_args.response.append(auth.user);
      start_ok_args.response.push_back('\0');
      start_ok_args.response.append(auth.passwd);
      start_ok_args.client_properties.Insert(
          "Product", FieldValue(FieldValue::Type::kShortString, "AMQP-cpp"));
      start_ok_args.client_properties.Insert(
          "Version", FieldValue(FieldValue::Type::kShortString, "0.01"));
      MethodStartOkArgsEncoder start_ok_encoder(start_ok_args, codec_env);
      frame_writer.WriteMethod(0, &start_ok_encoder);

      // Receive Tune
      MethodTuneArgsDecoder tune_decoder(codec_env);
      tune_decoder.Decode(frame_reader.Read());

      // Send Tune-OK
      MethodTuneOkArgs tune_ok_args;
      tune_ok_args.channel_max = params.channel_max;
      tune_ok_args.frame_max = params.frame_max;
      tune_ok_args.heartbeat_delay = params.heartbeat_delay;
      MethodTuneOkArgsEncoder tune_ok_encoder(tune_ok_args, codec_env);
      frame_writer.WriteMethod(0, &tune_ok_encoder);

      // Send Open
      MethodOpenArgs open_args;
      open_args.vhost_path = params.vhost;
      open_args.capabilities = false;
      open_args.insist = true;
      MethodOpenArgsEncoder open_encoder(open_args, codec_env);
      frame_writer.WriteMethod(0, &open_encoder);

      // Receive Open-OK
      MethodOpenOkArgsDecoder open_ok_decoder(codec_env);
      open_ok_decoder.Decode(frame_reader.Read());

      DLOG_INFO << "Connection established";
      auto amqp_conn = std::make_shared<Connection>(codec_env, conn.get());
      auto amqp_channel = amqp_conn->NewChannel();

    } catch (alpha::AsyncTcpConnectionException& e) {
      DLOG_WARNING << e.what();
    }
  });
}
#if 0

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

ConnectionMgr::ConnectionMgr()
    : tcp_client_(&loop_), async_tcp_client_(&loop_) {
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
  async_tcp_client_.RunInCoroutine([this, params](
      alpha::AsyncTcpClient* client, alpha::AsyncTcpConnectionCoroutine* co) {
    auto conn =
        client->ConnectTo(alpha::NetAddress(params.host, params.port), co);
    if (!conn) {
      DLOG_INFO << "Connect to " << params.host << ":" << params.port
                << " failed";
      return;
    }
    const std::string amqp_protocol_header = {'A', 'M', 'Q', 'P', 0, 0, 9, 1};
    try {
      conn->Write(amqp_protocol_header);
      auto frame = FrameReader(conn).Read();
      auto frame_header = conn->Read(7);
      CodedInputStream stream(frame_header);
      uint8_t frame_type;
      ChannelID channel_id;
      uint32_t payload_size;
      stream.ReadUInt8(&frame_type);
      stream.ReadBigEndianUInt16(&channel_id);
      stream.ReadBigEndianUInt32(&payload_size);

      auto payload = conn->Read(payload_size);
      auto frame_end = conn->Read(1);

      DLOG_INFO << "Frame type: " << static_cast<int>(frame_type)
                << ", Channel ID: " << channel_id
                << ", Payload size: " << payload_size
                << ", payload.size(): " << payload.size();
    } catch (alpha::AsyncTcpConnectionException& e) {
      DLOG_WARNING << e.what();
    }
  });
  // tcp_client_.ConnectTo(alpha::NetAddress(params.host, params.port));
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
#endif
}
