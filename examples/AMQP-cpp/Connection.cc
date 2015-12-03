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
#include "CodecEnv.h"
#include "Channel.h"
#include "ConnectionMgr.h"
#include "MethodPayloadCodec.h"

namespace amqp {
Connection::Connection(const CodecEnv* codec_env,
                       alpha::AsyncTcpConnection* conn)
    : next_channel_id_(1),
      codec_env_(codec_env),
      conn_(conn),
      w_(conn),
      r_(conn) {}

std::shared_ptr<Channel> Connection::NewChannel() {
  MethodChannelOpenArgs channel_open_args;
  MethodChannelOpenArgsEncoder channel_open_encoder(channel_open_args,
                                                    codec_env_);
  w_.WriteMethod(next_channel_id_, &channel_open_encoder);
  auto frame = HandleIncomingFrames(next_channel_id_, false);
  MethodChannelOpenOkArgsDecoder channel_open_ok_decoder(codec_env_);
  channel_open_ok_decoder.Decode(std::move(frame));
  auto channel = CreateChannel(next_channel_id_);
  ++next_channel_id_;
  DLOG_INFO << "Channel created";
  return channel;
}

void CloseChannel(const std::shared_ptr<Channel>& conn) {
  // MethodCloseArgs close_args;
  // close_args.reply_code = 0;
}

FramePtr Connection::HandleIncomingFrames(ChannelID stop_channel_id,
                                          bool cached_only) {
  bool done = cached_only && !conn_->HasCachedData();
  while (!done) {
    auto frame = r_.Read();
    CHECK(frame);
    if (frame->channel_id() == stop_channel_id) {
      return std::move(frame);
    } else {
      auto channel_id = frame->channel_id();
      if (channel_id == 0) {
        HandleConnectionFrame(std::move(frame));
      } else {
        auto channel = FindChannel(channel_id);
        // TODO: throw a ConnectionException
        CHECK(channel) << "Invalid frame with unknown channel id: "
                       << channel_id;
        channel->AddCachedFrame(std::move(frame));
      }
    }
  }

  CHECK(cached_only);
  return nullptr;
}

void Connection::HandleConnectionFrame(FramePtr&& frame) {
  GenericMethodArgsDecoder decoder(codec_env_);
  decoder.Decode(std::move(frame));
  DLOG_INFO << "Connection Frame, class id: "
            << decoder.accurate_decoder()->class_id()
            << ", method id: " << decoder.accurate_decoder()->method_id();
}

std::shared_ptr<Channel> Connection::CreateChannel(ChannelID channel_id) {
  auto p = channels_.insert(std::make_pair(
      channel_id, std::make_shared<Channel>(channel_id, shared_from_this())));
  CHECK(p.second) << "Channel exists, id: " << channel_id;
  return p.first->second;
}

std::shared_ptr<Channel> Connection::FindChannel(ChannelID channel_id) const {
  auto it = channels_.find(channel_id);
  return it == std::end(channels_) ? nullptr : it->second;
}

#if 0
Connection::~Connection() = default;

void Connection::Close() { owner_->CloseConnection(this); }

bool Connection::HandleFrame(FramePtr&& frame) { return false; }

void Connection::NewChannel() {}

void Connection::CloseChannel(ChannelID channel_id) {}

alpha::TcpConnectionPtr Connection::tcp_connection() const {
  return conn_.lock();
}

void Connection::OnChannelOpened(ChannelID channel_id) {
  auto p = channels_.emplace(
      channel_id, alpha::make_unique<Channel>(channel_id, shared_from_this()));
  CHECK(p.second) << "Channel opened twice, channel_id: " << channel_id;
}
#endif
}
