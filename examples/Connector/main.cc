/*
 * =============================================================================
 *
 *       Filename:  main.cc
 *        Created:  01/17/16 20:18:46
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include <alpha/logger.h>
#include <alpha/tcp_connector.h>
#include <alpha/event_loop.h>
#include <alpha/net_address.h>
#include <alpha/ScopedGeneric.h>

int main(int argc, char* argv[]) {
  if (argc != 3) {
    ::fprintf(stderr, "Usage: %s ip port\n", argv[0]);
    return EXIT_FAILURE;
  }
  alpha::Logger::set_logtostderr(true);
  alpha::Logger::Init(argv[0]);
  alpha::EventLoop loop;
  alpha::TcpConnector connector(&loop);
  int port = std::stoi(argv[2]);
  alpha::NetAddress addr(argv[1], port);
  connector.SetOnConnected([addr, &loop](int fd) {
    alpha::ScopedFD scoped_fd(fd);
    LOG_INFO << "Connected to " << addr << ", fd: " << fd;
    loop.Quit();
  });
  connector.SetOnError([addr, &loop](const alpha::NetAddress& address) {
    CHECK(addr == address);
    LOG_INFO << "Connect to " << address << " failed";
    loop.Quit();
  });
  LOG_INFO << "Try connect to " << addr;
  connector.ConnectTo(addr);
  loop.Run();
}
