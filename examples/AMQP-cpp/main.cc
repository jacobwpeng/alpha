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
void OnNewFrame(alpha::TcpConnectionPtr& conn, amqp::FramePtr&& frame);

void OnMessage(alpha::TcpConnectionPtr conn,
    alpha::TcpConnectionBuffer* buffer) {
  auto data = buffer->Read();
  DLOG_INFO << "Receive " << data.size() << " bytes from server";
  DLOG_INFO << '\n' << alpha::HexDump(data);
  auto consumed = data.size();
  auto frame = codec.Process(data);
  consumed -= data.size();
  DLOG_INFO << "FrameCodec consumed " << consumed << " bytes";
  if (frame) {
    OnNewFrame(conn, std::move(frame));
  }
  buffer->ConsumeBytes(consumed);
}

void OnConnected(alpha::TcpConnectionPtr conn) {
  using namespace std::placeholders;
  DLOG_INFO << "Connected to " << conn->PeerAddr().FullAddress();
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
  DLOG_INFO << "Disconnected from " << conn->PeerAddr().FullAddress();
  connection.reset();
}

void OnNewFrame(alpha::TcpConnectionPtr& conn, amqp::FramePtr&& frame) {
  DLOG_INFO << "Frame type: " << static_cast<int>(frame->type())
    << ", Frame channel: " << frame->channel_id()
    << ", Frame expeced payload size: " << frame->expected_payload_size()
    << ", Frame paylaod size: " << frame->payload_size();
  amqp::MethodStartArgsDecoder d;
  int rc = d.Decode(frame->payload());
  if (rc == 0) {
    auto args = d.Get();
    DLOG_INFO << "Decode done"
      << ", version_major: " << static_cast<uint16_t>(args.version_major)
      << ", version_minor: " << static_cast<uint16_t>(args.version_minor)
      << ", mechanisms: " << args.mechanisms
      << ", locales: " << args.locales;
    for (const auto& p : args.server_properties.underlying_map()) {
      DLOG_INFO << p.first << ": " << p.second;
    }

    //amqp::MethodStartOkArgs ok;
    //ok.mechanism = "PLAIN";
    //ok.locale = "en_US";
    //std::string user = "guest";
    //std::string passwd = "guest";
    //ok.response = '\0' + user + '\0' + passwd;
    //amqp::MethodStartArgsEncoder encoder(ok);
    //encoder.Process([&conn](const char* data, size_t sz){
    //  conn->Write(alpha::Slice(data, sz));
    //});
  } else {
    DLOG_INFO << "Decode returns: " << rc;
  }
}

int main(int argc, char* argv[]) {
  alpha::Logger::Init(argv[0]);
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
  client.ConnectTo(addr);
  loop.Run();
  return 0;
}
