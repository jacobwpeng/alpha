/*
 * =============================================================================
 *
 *       Filename:  ping.cc
 *        Created:  03/22/17 10:57:37
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include <alpha/EventLoop.h>
#include <alpha/Logger.h>
#include <alpha/TcpClient.h>

static const std::string kMsgPing = "PING";

static void OnRead(alpha::TcpConnectionPtr conn,
                   alpha::TcpConnectionBuffer* buffer) {
  static int n = 0;
  size_t len;
  auto data = buffer->Read(&len);
  (void)data;
  buffer->ConsumeBytes(len);
  conn->Write(kMsgPing.data(), kMsgPing.size());
  ++n;
  LOG_INFO_IF(n % 10000 == 0) << "Ping-Pong " << n << " times";
}

static void OnClose(alpha::EventLoop* loop, int fd) {
  LOG_INFO << "Server disconnected, fd: " << fd;
  loop->Quit();
}

static void OnConnected(alpha::TcpConnectionPtr conn) {
  conn->SetOnRead(OnRead);
  conn->SetOnClose(std::bind(OnClose, conn->loop(), std::placeholders::_1));
  conn->Write(kMsgPing.data(), kMsgPing.size());
}

static void OnConnectError(alpha::EventLoop* loop, const alpha::NetAddress&) {
  loop->Quit();
}

int main(int argc, char* argv[]) {
  alpha::Logger::Init(argv[0]);
  alpha::Logger::set_logtostderr(true);

  alpha::EventLoop loop;

  alpha::TcpClient client(&loop);
  client.SetOnConnected(OnConnected);
  client.ConnectTo(alpha::NetAddress("127.0.0.1", 12345));
  client.SetOnConnectError(
      std::bind(OnConnectError, &loop, std::placeholders::_1));

  auto quit = [&loop] { loop.Quit(); };
  loop.TrapSignal(SIGINT, quit);
  loop.TrapSignal(SIGTERM, quit);
  loop.Run();
}
