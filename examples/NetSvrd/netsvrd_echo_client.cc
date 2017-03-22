/*
 * =============================================================================
 *
 *       Filename:  netsvrd_echo_client.cc
 *        Created:  03/22/17 16:03:23
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include <alpha/EventLoop.h>
#include <alpha/Logger.h>
#include <alpha/TcpClient.h>
#include "netsvrd_frame.h"

static const std::string msg = "HELLO, WORLD!";

static void SendFrame(alpha::TcpConnectionPtr conn) {
  auto frame = NetSvrdFrame::CreateUnique(msg.size());
  memcpy(frame->payload, msg.data(), msg.size());
  conn->Write(frame->data(), frame->size());
}

static void OnRead(alpha::TcpConnectionPtr conn,
                   alpha::TcpConnectionBuffer* buffer) {
  alpha::NetAddress peer;
  conn->PeerAddr(&peer);
  size_t len;
  char* data = buffer->Read(&len);
  auto frame_header = NetSvrdFrame::CastHeaderOnly(data, len);
  CHECK(frame_header != nullptr) << "Failed to cast header";
  CHECK(frame_header->payload_size == msg.size()) << "Payload size not match";
  if (frame_header->payload_size + NetSvrdFrame::kHeaderSize <= len) {
    auto frame = (NetSvrdFrame*)data;
    CHECK(memcmp(frame->payload, msg.data(), msg.size()) == 0)
        << "Payload not match";
    buffer->ConsumeBytes(frame->size());
    SendFrame(conn);
  }
}

static int Usage(const char* argv0) {
  std::printf("Usage: %s IP PORT\n", argv0);
  return EXIT_FAILURE;
}

int main(int argc, char* argv[]) {
  if (argc != 3) {
    return Usage(argv[0]);
  }
  alpha::Logger::Init(argv[0]);
  alpha::Logger::set_logtostderr(true);

  alpha::EventLoop loop;

  alpha::TcpClient client(&loop);
  client.SetOnConnectError([&loop](const alpha::NetAddress& addr) {
    LOG_ERROR << "Connect to " << addr << " failed, exit...";
    loop.Quit();
  });
  client.SetOnClose([&loop](alpha::TcpConnectionPtr conn) {
    LOG_INFO << "Connection closed, exit...";
    loop.Quit();
  });
  client.SetOnConnected([&loop](alpha::TcpConnectionPtr conn) {
    conn->SetOnRead(OnRead);
    SendFrame(conn);
  });
  client.ConnectTo(alpha::NetAddress(argv[1], std::stoi(argv[2])));
  loop.Run();
}
