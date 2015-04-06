/*
 * =============================================================================
 *
 *       Filename:  ping_pong_client.cc
 *        Created:  04/06/15 16:17:38
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#include "logger.h"
#include "event_loop.h"
#include "tcp_client.h"

class PingPongClient {
    public:
        PingPongClient(alpha::EventLoop* loop)
            :loop_(loop), client_(loop), connections(0) {
            using namespace std::placeholders;
            client_.SetOnConnected(std::bind(&PingPongClient::OnConnected, this, _1));
            client_.SetOnRead(std::bind(&PingPongClient::OnRead, this, _1, _2));
        }

        void Run() {
            alpha::NetAddress server_addr("127.0.0.1", 7890);
            for (int i = 0; i < 1; ++i) {
                LOG_INFO << "i = " << i;
                client_.ConnectTo(server_addr);
            }
        }

    private:
        void OnConnected(alpha::TcpConnectionPtr conn) {
            LOG_INFO << "Connected to " << conn->PeerAddr();
            ++connections;
            conn->Write("Hello, Server!");
        }

        void OnRead(alpha::TcpConnectionPtr conn, alpha::TcpConnectionBuffer* buffer) {
            auto data = buffer->Read();
            LOG_INFO << "Read " << data.size() << " bytes from " << conn->PeerAddr();
            buffer->ConsumeBytes(data.size());
            conn->Write(data);
        }

        void OnClose(alpha::TcpConnectionPtr conn) {
            --connections;
            if (connections == 0) {
                loop_->Quit();
            }
        }

        alpha::EventLoop* loop_;
        alpha::TcpClient client_;
        int connections;
};

int main(int argc, char* argv[]) {
    alpha::Logger::Init(argv[0], alpha::Logger::LogToStderr);
    alpha::EventLoop loop;
    PingPongClient c(&loop);
    loop.QueueInLoop(std::bind(&PingPongClient::Run, &c));
    loop.Run();
    return EXIT_SUCCESS;
}
