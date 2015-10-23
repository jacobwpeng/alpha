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

#include <alpha/event_loop.h>
#include <alpha/logger.h>
#include <alpha/tcp_client.h>
#include <alpha/format.h>
#include "Frame.h"
#include "FrameCodec.h"
#include "MethodPayloadCodec.h"
#include "FieldValue.h"

static alpha::TcpConnectionPtr connection;
static const uint8_t kMajorVersion = 9;
static const uint8_t kMinorVersion = 1;
amqp::FrameCodec codec;

void OnMessage(alpha::TcpConnectionPtr conn,
    alpha::TcpConnectionBuffer* buffer) {
  auto data = buffer->Read();
  LOG_INFO << "Receive " << data.size() << " bytes from server";
  LOG_INFO << '\n' << alpha::HexDump(data);
  auto sz = codec.Process(data);
  buffer->ConsumeBytes(sz);
}

void OnConnected(alpha::TcpConnectionPtr conn) {
  using namespace std::placeholders;
  LOG_INFO << "Connected to " << conn->PeerAddr().FullAddress();
  connection = conn;
  conn->SetOnRead(std::bind(OnMessage, _1, _2));
  std::string handshake = "AMQP";
  handshake += char(0);
  handshake += char(0);
  handshake += char(kMajorVersion);
  handshake += char(kMinorVersion);
  conn->Write(handshake);
}

void OnConnectError(const alpha::NetAddress& addr) {
  LOG_WARNING << "Connect to " << addr.FullAddress() << " failed";
}

void OnDisconnected(alpha::TcpConnectionPtr conn) {
  LOG_INFO << "Disconnected from " << conn->PeerAddr().FullAddress();
  connection.reset();
}

void OnNewFrame(amqp::Frame* frame) {
  LOG_INFO << "Frame type: " << static_cast<int>(frame->type())
    << ", Frame channel: " << frame->channel_id()
    << ", Frame expeced payload size: " << frame->expected_payload_size()
    << ", Frame paylaod size: " << frame->payload_size();
  amqp::MethodStartArgsDecoder d;
  int rc = d.Decode(frame->payload());
  if (rc == 0) {
    auto args = d.Get();
    LOG_INFO << "Decode done, "
      << "version_major: " << static_cast<uint8_t>(args.version_major)
      << "version_minor: " << static_cast<uint8_t>(args.version_minor)
      << "mechanisms: " << args.mechanisms
      << "locales: " << args.locales;
    //for (const auto& p : args.server_properties) {
    //  LOG_INFO << p.first << ": " << p.second;
    //}
  } else {
    LOG_INFO << "Decode returns: " << rc;
  }
}

int main(int argc, char* argv[]) {
  alpha::Logger::Init(argv[0]);
  amqp::FieldValue v;
  {
    amqp::FieldValue val(amqp::FieldValue::Type::kLongString, "123");
    v = val;
  }
  auto p = v.AsPtr<std::string>();
  LOG_INFO << p->data();
#if 0
  if (argc != 3) {
    LOG_INFO << "Usage: " << argv[0] << " [ip] [port]";
    return -1;
  }
  alpha::EventLoop loop;
  alpha::TcpClient client(&loop);
  client.SetOnConnected(OnConnected);
  client.SetOnConnectError(OnConnectError);
  client.SetOnClose(OnDisconnected);
  auto addr = alpha::NetAddress(argv[1], std::atoi(argv[2]));
  codec.SetNewFrameCallback(OnNewFrame);
  client.ConnectTo(addr);
  loop.Run();
  return 0;
#endif
}
