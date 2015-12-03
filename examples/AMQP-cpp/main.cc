/*
 * =============================================================================
 *
 *       Filename:  main.cc
 *        Created:  10/19/15 14:21:16
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include <alpha/logger.h>
#include "Connection.h"
#include "ConnectionMgr.h"
#if 0

Connection::NewChannel() {
  conn_->WriteMethod(next_channel_id_, channel_open);
  auto frame = HandleCachedFrames(next_channel_id_);
  MethodChannelOpenOkArgsDecoder decoder(codec_env);
  decoder.Decode(std::move(frame));
  auto channel = CreateChannel(next_channel_id_);
  ++next_channel_id_;
  return channel;
}

Connection::HandleChannelFrames(ChannelID newly_created_channel_id) {
  while (conn_->HasCachedData()) {
    auto frame = frame_reader.Read();
    if (frame.channel_id() == newly_created_channel_id) {
      return std::move(frame);
    } else {
      auto channel = FindChannel(frame.channel_id());
      if (channel == nullptr) {
        AbortConnection();
      }
      channel->AddCachedFrame(std::move(frame));
    }
  }
  //auto channel = FindChannel(frame.channel_id());
  //if (channel == nullptr) {
  //  ThrowConnectionException();
  //}
}

void Process(amqp::ConnectionPtr& conn) {
  auto channel = conn->NewChannel();
  auto channel2 = conn->NewChannel();
  Exchange* exchange = channel->DeclareExchange();
  Queue* queue = channel->DeclareQueue();
  channel->BindQueue(exchange, queue);
}
#endif

int main(int argc, char* argv[]) {
  alpha::Logger::Init(argv[0]);
  alpha::EventLoop loop;
  amqp::ConnectionMgr mgr(&loop);
  // mgr.set_connected_callback([](amqp::ConnectionPtr& conn) {
  //  DLOG_INFO << "amqp::Connection created";
  //  conn->Close();
  //});
  auto auth = amqp::PlainAuthorization();
  auth.user = "guest";
  auth.passwd = "guest";
  mgr.ConnectTo(amqp::ConnectionParameters(), auth);
  loop.Run();
#if 0
  // Blocking Connection
  auto auth = amqp::PlainAuthorization(user, passwd);
  auto conn = amqp::BlockingConnection(host, port, auth);
  auto channel = conn->NewChannel();
  auto exchange = channel->DeclareExchange(exchange_name, exchange_type,
                                           exchange_properties);
  auto queue = channel->DeclareQueue(queue_name, queue_properties);
  channel->BindQueue(exchange, queue);

  channel->SetQueueMessageCallback(queue, callback);
  while (channel->Wait(timeout))
    ;
#endif
  return 0;
}
