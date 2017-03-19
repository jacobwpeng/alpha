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
#include <alpha/Channel.h>
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

void Connection::Close() {
  if (conn_->closed()) {
    return;
  }

  MethodCloseArgs close_args;
  close_args.reply_code = 0;
  close_args.reply_text = "Normal Shutdown";
  close_args.class_id = close_args.method_id = 0;
  MethodCloseArgsEncoder close_encoder(close_args, codec_env_);
  w_.WriteMethod(0, &close_encoder);
  while (1) {
    auto frame = HandleIncomingFramesUntil(0);
    if (frame->type() != Frame::Type::kMethod) {
      continue;  // Skip all non-method frames
    }
    GenericMethodArgsDecoder decoder(codec_env_);
    decoder.Decode(std::move(frame));
    auto accurate_decoder = decoder.accurate_decoder();
    if (accurate_decoder->class_id() != kClassConnectionID) {
      continue;  // Skip all non-connection methods
    }
    if (accurate_decoder->method_id() == MethodCloseOkArgs::kMethodID) {
      // Close OK, we are done
      break;
    } else if (accurate_decoder->method_id() == MethodCloseArgs::kMethodID) {
      // Simultaneous Close
      MethodCloseOkArgs close_ok_args;
      MethodCloseOkArgsEncoder close_ok_encoder(close_ok_args, codec_env_);
      w_.WriteMethod(0, &close_ok_encoder);
      // We are done here
      break;
    } else {
      // Skip all other connection methods
    }
  }
}

std::shared_ptr<Channel> Connection::NewChannel() {
  MethodChannelOpenArgs channel_open_args;
  MethodChannelOpenArgsEncoder channel_open_encoder(channel_open_args,
                                                    codec_env_);
  w_.WriteMethod(next_channel_id_, &channel_open_encoder);
  auto frame = HandleIncomingFramesUntil(next_channel_id_);
  MethodChannelOpenOkArgsDecoder channel_open_ok_decoder(codec_env_);
  channel_open_ok_decoder.Decode(std::move(frame));
  auto channel = CreateChannel(next_channel_id_);
  ++next_channel_id_;
  DLOG_INFO << "Channel created";
  return channel;
}

FramePtr Connection::HandleCachedFrames() {
  return HandleIncomingFrames(next_channel_id_, true);
}
FramePtr Connection::HandleIncomingFramesUntil(ChannelID channel_id) {
  return HandleIncomingFrames(channel_id, false);
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

void Connection::CloseChannel(ChannelID channel_id) {
  auto channel = FindChannel(channel_id);
  CHECK(channel) << "No channel found in this connection, id: " << channel_id;
  MethodChannelCloseArgs close_args;
  close_args.reply_code = 0;
  close_args.reply_text = "Normal Shutdown";
  close_args.class_id = close_args.method_id = 0;
  MethodChannelCloseArgsEncoder close_encoder(close_args, codec_env_);
  w_.WriteMethod(channel_id, &close_encoder);
  auto frame = HandleIncomingFramesUntil(channel_id);
  MethodChannelCloseOkArgsDecoder(codec_env_).Decode(std::move(frame));
  DestroyChannel(channel_id);
  DLOG_INFO << "Channel closed";
}

void Connection::DestroyChannel(ChannelID channel_id) {
  channels_.erase(channel_id);
}

#if 0
Connection::~Connection() = default;


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
