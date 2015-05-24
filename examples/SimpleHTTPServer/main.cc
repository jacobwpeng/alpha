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

#include <alpha/logger.h>
#include <alpha/net_address.h>
#include <alpha/event_loop.h>
#include <alpha/simple_http_server.h>
#include <alpha/http_message.h>

int main(int argc, char* argv[]) {
    alpha::Logger::Init(argv[0]);
    alpha::EventLoop loop;
    loop.TrapSignal(SIGINT, [&]{
            loop.Quit();
    });
    alpha::SimpleHTTPServer http_server(&loop,
            alpha::NetAddress("127.0.0.1", 8080));
    http_server.SetCallback([](alpha::TcpConnectionPtr conn,
                const alpha::HTTPMessage& message) {
        LOG_INFO << "Client ip = " << message.ClientIp()
        << ", port = " << message.ClientPort()
        << ", method = " << message.Method()
        << ", path = " << message.Path();
        message.Headers().Foreach([](const std::string& name,
                    const std::string& val) {
            LOG_INFO << name << ": " << val;
        });
        if (!message.Body().empty()) {
            LOG_INFO << "Body: " << message.Body();
        }

        for (auto & p : message.Params()) {
            LOG_INFO << p.first << ": " << p.second;
        }
    });
    if (http_server.Run()) {
        loop.Run();
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}
