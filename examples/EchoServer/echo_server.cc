/*
 * =============================================================================
 *
 *       Filename:  echo_server.cc
 *        Created:  04/05/15 16:00:06
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#include <vector>
#include <cassert>
#include <cstdio>
#include <functional>
#include "slice.h"
#include "logger.h"
#include "event_loop.h"
#include "tcp_connection_buffer.h"
#include "tcp_connection.h"
#include "tcp_server.h"

class EchoServer {
    public:
        EchoServer(alpha::EventLoop* loop, const alpha::NetAddress& addr)
            :server_(loop, addr) {
        }

        bool Run() {
            using namespace std::placeholders;
            server_.SetOnRead(std::bind(&EchoServer::OnRead, this, _1, _2));
            return server_.Run();
        }

    private:
        void OnRead(alpha::TcpConnectionPtr conn, alpha::TcpConnectionBuffer* buffer) {
            alpha::Slice data = buffer->Read();
            DLOG_INFO << "Receive from " << conn->PeerAddr()
                << ", data.size() = " << data.size();
            std::vector<char> input(data.size());
            size_t n = buffer->ReadAndClear(input.data(), input.size());
            assert (n == input.size());
            conn->Write(alpha::Slice(input.data(), input.size()));
        }

        alpha::EventLoop* loop_;
        alpha::TcpServer server_;
};

int main(int argc, char* argv[]) {
    alpha::Logger::Init(argv[0], alpha::Logger::LogToStderr);
    alpha::EventLoop loop;
    alpha::NetAddress addr("127.0.0.1", 7890);
    EchoServer echo_server(&loop, addr);
    if (!echo_server.Run()) {
        return EXIT_FAILURE;
    }
    loop.Run();
    return EXIT_SUCCESS;
}
