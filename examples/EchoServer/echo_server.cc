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
            :loop_(loop), server_(loop, addr) {
        }

        bool Run() {
            using namespace std::placeholders;
            server_.SetOnNewConnection(std::bind(&EchoServer::OnNewConnection, this, _1));
            server_.SetOnRead(std::bind(&EchoServer::OnRead, this, _1, _2));
            return server_.Run();
        }

    private:
        void OnNewConnection(alpha::TcpConnectionPtr conn) {
            LOG_INFO << "New connection from " << conn->PeerAddr();
            UpdateTimer(conn);
        }

        void OnRead(alpha::TcpConnectionPtr conn, alpha::TcpConnectionBuffer* buffer) {
            alpha::Slice data = buffer->Read();
            DLOG_INFO << "Receive from " << conn->PeerAddr()
                << ", data.size() = " << data.size();
            std::vector<char> input(data.size());
            size_t n = buffer->ReadAndClear(input.data(), input.size());
            assert (n == input.size()); (void)n;
            conn->Write(alpha::Slice(input.data(), input.size()));
            UpdateTimer(conn);
        }

        void UpdateTimer(alpha::TcpConnectionPtr& conn) {
            auto ctx = conn->GetContext();
            if (!ctx.empty()) {
                auto timer_id = boost::any_cast<alpha::TimerManager::TimerId>(ctx);
                loop_->RemoveTimer(timer_id);
            }
            auto timer_id = loop_->RunAfter(kDefaultTimeout, std::bind(&EchoServer::KickOff, 
                                this, alpha::TcpConnectionWeakPtr(conn)));
            conn->SetContext(timer_id);
        }

        void KickOff(alpha::TcpConnectionWeakPtr weak_conn) {
            if (alpha::TcpConnectionPtr conn = weak_conn.lock()) {
                LOG_INFO << "Kickoff " << conn->PeerAddr();
                auto ctx = conn->GetContext();
                auto timer_id = boost::any_cast<alpha::TimerManager::TimerId>(ctx);
                loop_->RemoveTimer(timer_id);
                conn->Write("Timeout\n");
                conn->Close();
            }
        }

        const uint32_t kDefaultTimeout = 3000;
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
