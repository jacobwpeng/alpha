/*
 * ==============================================================================
 *
 *       Filename:  ping_pong_server.cc
 *        Created:  04/06/15 18:58:00
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#include <cassert>
#include <map>
#include <memory>

#include "slice.h"
#include "coroutine.h"
#include "logger.h"
#include "event_loop.h"
#include "tcp_server.h"
#include "tcp_connection.h"
#include "tcp_connection_buffer.h"

using namespace alpha;

class PingPongServerWorker : public alpha::Coroutine {
    public:
        virtual void Routine() {
            while (!Closed()) {
                auto data = Read();
                Write(data);
            }
            LOG_INFO << "Routine done";
        }

        void SetClosed() {
            closed_ = true;
        }

        bool Closed() const {
            return closed_;
        }

        void SetSlice(const Slice& data) {
            slice_ = data;
        }

        Slice GetSlice() const {
            return slice_;
        }

        Slice Read() {
            Yield();
            return slice_;
        }

        void Write(const Slice& data) {
            slice_ = data;
            Yield();
            slice_ = Slice();
        }

    private:
        bool closed_ = false;
        Slice slice_;
};

using PingPongServerWorkerPtr = std::shared_ptr<PingPongServerWorker>;

class PingPongServer {
    public:
        PingPongServer(EventLoop* loop, const NetAddress& addr)
            :loop_(loop), server_(loop_, addr) {
            assert (loop);
            using namespace std::placeholders;
            server_.SetOnNewConnection(std::bind(&PingPongServer::OnNewConnection, this, _1));
            server_.SetOnRead(std::bind(&PingPongServer::OnRead, this, _1, _2));
            server_.SetOnClose(std::bind(&PingPongServer::OnClose, this, _1));
        }

        bool Run() {
            return server_.Run();
        }

    private:
        void OnNewConnection(TcpConnectionPtr conn) {
            PingPongServerWorkerPtr worker = std::make_shared<PingPongServerWorker>();
            workers_.emplace(conn->fd(), worker);
            worker->Resume();
        }

        void OnRead(TcpConnectionPtr conn, TcpConnectionBuffer* buffer) {
            auto it = workers_.find(conn->fd());
            assert (it != workers_.end());
            auto data = buffer->Read();
            it->second->SetSlice(data);
            it->second->Resume();
            buffer->ConsumeBytes(data.size());
            data = it->second->GetSlice();
            if (!data.empty()) {
                conn->Write(data);
            }
            it->second->Resume();
        }

        void OnClose(TcpConnectionPtr conn) {
            auto it = workers_.find(conn->fd());
            it->second->SetClosed();
            it->second->Resume();
            workers_.erase(it);
        }

    private:
        using WorkerMap = std::map<int, PingPongServerWorkerPtr>;
        EventLoop* loop_;
        TcpServer server_;
        WorkerMap workers_;
};

int main(int argc, char* argv[]) {
    alpha::Logger::Init(argv[0]);
    EventLoop loop;
    PingPongServer s(&loop, NetAddress("127.0.0.1", 7890));
    s.Run();
    loop.Run();
    return 0;
}
