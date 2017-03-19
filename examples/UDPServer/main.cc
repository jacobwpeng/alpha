/*
 * =============================================================================
 *
 *       Filename:  main.cc
 *        Created:  03/13/17 16:29:58
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include <alpha/Logger.h>
#include <alpha/EventLoop.h>
#include <alpha/UDPSocket.h>
#include <alpha/UDPServer.h>
#include <alpha/IOBuffer.h>

static int num = 0;
static void EchoHandler(alpha::UDPSocket* sock,
                        alpha::IOBuffer* buffer,
                        size_t buf_len,
                        const alpha::NetAddress& raddr) {
  int n = sock->SendTo(buffer, buf_len, raddr);
  CHECK(n == int(buf_len));
  ++num;
  LOG_INFO_IF(num % 10000 == 0) << "Receive " << num << " messages";
}

int main(int argc, char* argv[]) {
  alpha::Logger::Init(argv[0]);
  alpha::Logger::set_logtostderr(true);
  LOG_INFO << "Listening at 0.0.0.0:40000";
  alpha::EventLoop loop;
  alpha::UDPServer server(&loop);
  server.SetMessageCallback(EchoHandler);
  server.Run(alpha::NetAddress("0.0.0.0", 40000));
  loop.Run();
}
