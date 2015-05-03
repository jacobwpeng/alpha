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

int main(int argc, char* argv[]) {
    alpha::Logger::Init(argv[0], alpha::Logger::LogToStderr);
    alpha::EventLoop loop;
    loop.TrapSignal(SIGINT, [&]{
            loop.Quit();
    });
    alpha::SimpleHTTPServer http_server(&loop, alpha::NetAddress("127.0.0.1", 8080));
    http_server.SetOnGet([](alpha::TcpConnectionPtr conn, alpha::Slice path
                , const alpha::SimpleHTTPServer::HTTPHeader& header, alpha::Slice data){
        LOG_INFO << "Get request from " << conn->PeerAddr()
            << ", path = " << path.ToString();
    });
    if (http_server.Run()) {
        loop.Run();
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}
