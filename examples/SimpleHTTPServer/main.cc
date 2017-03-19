/*
 * ==============================================================================
 *
 *       Filename:  main.cc
 *        Created:  05/03/15 21:10:02
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * ==============================================================================
 */

#include <alpha/Logger.h>
#include <alpha/NetAddress.h>
#include <alpha/EventLoop.h>
#include <alpha/Format.h>
#include <alpha/SimpleHTTPServer.h>
#include <alpha/HTTPMessage.h>
#include <alpha/HTTPResponseBuilder.h>

int Usage(const char* argv0) {
  printf("Usage: %s PORT\n", argv0);
  return EXIT_FAILURE;
}

int main(int argc, char* argv[]) {
  alpha::Logger::Init(argv[0]);
  alpha::Logger::set_logtostderr(true);
  if (argc != 2) {
    return Usage(argv[0]);
  }
  alpha::EventLoop loop;
  loop.TrapSignal(SIGINT, [&] { loop.Quit(); });

  alpha::SimpleHTTPServer http_server(&loop);
  http_server.SetCallback(
      [](alpha::TcpConnectionPtr conn, const alpha::HTTPMessage& message) {
        LOG_INFO << "Client ip = " << message.ClientIp()
                 << ", port = " << message.ClientPort()
                 << ", method = " << message.Method()
                 << ", path = " << message.Path()
                 << ", query string = " << message.QueryString();
        message.Headers().Foreach(
            [](const std::string& name, const std::string& val) {
              LOG_INFO << name << ": " << val;
            });
        if (!message.Body().empty()) {
          LOG_INFO << "Body size: " << message.Body().size();
        }

        for (auto& p : message.Params()) {
          LOG_INFO << p.first << ": " << p.second;
        }
        const auto& payloads = message.payloads();
        for (auto payload : payloads) {
          LOG_INFO << "Payload size: " << payload.size();
        }
        alpha::HTTPResponseBuilder builder(conn);
        builder.status(404, "Not Found").SendWithEOM();
      });

  int port = std::stoul(argv[1]);
  if (http_server.Run(alpha::NetAddress("0.0.0.0", port))) {
    loop.Run();
    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}
