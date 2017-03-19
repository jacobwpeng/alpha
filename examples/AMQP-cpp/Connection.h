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

#include <map>
#include <alpha/AsyncTcpConnection.h>
#include "Frame.h"
#include "FrameCodec.h"
#include "MethodArgTypes.h"
#include "MethodPayloadCodec.h"

namespace amqp {
class CodecEnv;
class Channel;
class ConnectionMgr;
class Connection final : public std::enable_shared_from_this<Connection> {
 public:
  Connection(const CodecEnv* codec_env, alpha::AsyncTcpConnection* conn);
  //~Connection();
  void Close();

  // Channel operation
  std::shared_ptr<Channel> NewChannel();

 private:
  FramePtr HandleCachedFrames();
  FramePtr HandleIncomingFramesUntil(ChannelID channel_id);
  // Handle all frames until frame.channel_id() == stop_channel_id
  // if such frame exists, return that frame
  // else return nullptr
  FramePtr HandleIncomingFrames(ChannelID stop_channel_id, bool cached_only);
  void HandleConnectionFrame(FramePtr&& frame);
  std::shared_ptr<Channel> CreateChannel(ChannelID channel_id);
  std::shared_ptr<Channel> FindChannel(ChannelID channel_id) const;
  void DestroyChannel(ChannelID channel_id);

  // Used by Class Channel
  void CloseChannel(ChannelID channel_id);
  template <typename Args>
  void WriteMethod(ChannelID channel_id, Args&& args);

  uint32_t next_channel_id_;
  const CodecEnv* codec_env_;
  alpha::AsyncTcpConnection* conn_;
  FrameWriter w_;
  FrameReader r_;
  std::map<ChannelID, std::shared_ptr<Channel>> channels_;
  friend class Channel;
};

using ConnectionPtr = std::shared_ptr<Connection>;
using ConnectionWeakPtr = std::weak_ptr<Connection>;

template <typename Args>
void Connection::WriteMethod(ChannelID channel_id, Args&& args) {
  using RawArgType = typename std::remove_pointer<typename std::remove_cv<
      typename std::remove_reference<Args>::type>::type>::type;
  using RequestEncoderType =
      typename ArgsToCodecHelper<RawArgType>::EncoderType;
  RequestEncoderType encoder(std::forward<Args>(args), codec_env_);
  w_.WriteMethod(channel_id, &encoder);
  auto frame = HandleIncomingFramesUntil(channel_id);
  // using ResponseDecoderType = typename ArgsToCodecHelper<
  //    typename
  //    RequestToResponseHelper<RawArgType>::ResponseType>::DecoderType;
  GenericMethodArgsDecoder generic_decoder(codec_env_);
  generic_decoder.Decode(std::move(frame));
  // auto response_class_id = generic_decoder.accurate_decoder()->class_id();
  // auto response_method_id = generic_decoder.accurate_decoder()->method_id();
  // if (response_class_id == kClassChannelID && response_method_id ==
  // MethodChannelCloseArgs::kMethodID) {
  //  //
  //}
  // ResponseDecoderType decoder(codec_env_);
  // decoder.Decode(std::move(frame));
}
}

#endif /* ----- #ifndef __CONNECTION_H__  ----- */
