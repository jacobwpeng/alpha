/*
 * =============================================================================
 *
 *       Filename:  main.cc
 *        Created:  12/01/15 11:55:11
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include <cstdio>
#include <alpha/logger.h>
#include <alpha/IOBuffer.h>
#include <alpha/event_loop.h>
#include <alpha/AsyncTcpClient.h>
#include <alpha/AsyncTcpConnectionException.h>

int Usage(const char* argv0) {
  std::printf("Usage: %s IP PORT\n", argv0);
  return EXIT_FAILURE;
}

int main(int argc, char* argv[]) {
  if (argc != 3) {
    return Usage(argv[0]);
  }
  alpha::Logger::Init(argv[0]);
  std::string ip = argv[1];
  int port = std::stoi(argv[2]);
  alpha::EventLoop loop;
  alpha::AsyncTcpClient client(&loop);
  client.RunInCoroutine([&loop, &ip, &port](
      alpha::AsyncTcpClient* client, alpha::AsyncTcpConnectionCoroutine* co) {
    auto conn = client->ConnectTo(alpha::NetAddress(ip, port), co);
    if (conn) {
      DLOG_INFO << "Connected";
      conn->Write("GET / HTTP/1.0\r\n\r\n");
      size_t n = 0;
      alpha::IOBufferWithSize buffer(1 << 16);
      try {
        n += conn->ReadFull(&buffer, buffer.size());
        DLOG_INFO << "n = " << n;
      } catch (alpha::AsyncTcpConnectionException& e) {
        DLOG_WARNING << e.what();
      }
      std::string reply(buffer.data(), n);
      DLOG_INFO << "Read " << reply.size() << " Bytes";
      DLOG_INFO << '\n' << reply;
    }
    loop.Quit();
  });
  loop.Run();
  return EXIT_SUCCESS;
}
