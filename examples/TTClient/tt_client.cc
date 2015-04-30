/*
 * =============================================================================
 *
 *       Filename:  tt_client.cc
 *        Created:  04/30/15 17:23:35
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#include "tt_client.h"
#include <arpa/inet.h>
#include <alpha/logger.h>
#include <alpha/event_loop.h>
#include <alpha/tcp_client.h>
#include <alpha/coroutine.h>
#include <alpha/net_address.h>
#include "tt_protocol_codec.h"

namespace tokyotyrant {
    Client::Client(alpha::EventLoop* loop)
        :loop_(loop), co_(nullptr), state_(ConnectionState::kDisconnected) {
        tcp_client_.reset(new alpha::TcpClient(loop_));
        using namespace std::placeholders;
        tcp_client_->SetOnConnected(std::bind(&Client::OnConnected, this, _1));
        tcp_client_->SetOnConnectError(std::bind(&Client::OnConnectError, this, _1));
        tcp_client_->SetOnClose(std::bind(&Client::OnDisconnected, this, _1));
    }

    void Client::SetCoroutine(alpha::Coroutine* co) {
        co_ = co;
    }

    bool Client::Connnect(const alpha::NetAddress& addr) {
        if (addr_ != nullptr) {
            return false;
        } else {
            addr_.reset (new alpha::NetAddress(addr));
            tcp_client_->ConnectTo(addr);
            state_ = ConnectionState::kConnecting;
            co_->Yield();
            LOG_INFO_IF(!Connected()) << "Connect failed";
            return Connected();
        }
    }

    bool Client::Connected() const {
        return state_ == ConnectionState::kConnected;
    }

    int Client::Stat(std::string* stat) {
        if (state_ == ConnectionState::kDisconnected) {
            return kConn;
        }
        const int16_t kMagic = ::htons(0xC888);
        conn_->Write(alpha::Slice(&kMagic));
        ProtocolCodec codec;
        LengthPrefixedCodecUnit unit(stat);
        codec.AddUnit(&unit);
        CodecStatus status;
        do {
            co_->Yield();
            alpha::Slice data = conn_->ReadBuffer()->Read();
            auto buffer = reinterpret_cast<const uint8_t*>(data.data());
            status = static_cast<CodecStatus>(codec.Process(buffer, data.size()));
            LOG_INFO << "status = " << status;
            if (status == 0 || status == kNeedsMore) {
                auto nbytes = codec.ConsumedBytes();
                codec.ClearConsumed();
                conn_->ReadBuffer()->ConsumeBytes(nbytes);
            } else {
                conn_->ReadBuffer()->ConsumeBytes(data.size());
            }
        } while (status == kNeedsMore);

        if (status) {
            return status;
        } else {
            return 0;
        }
    }
#if 0
    int Client::Put(alpha::Slice key, alpha::Slice value) {
        LOG_INFO << "Key = " << key.data() << ", value = " << value.data();
        co_->Yield();
        return -1;
    }
#endif
    void Client::OnConnectError(const alpha::NetAddress& addr) {
        assert (addr_ && *addr_ == addr);
        state_ = ConnectionState::kDisconnected;
        LOG_WARNING << "Connect to " << addr << " failed";
        co_->Resume();
    }

    void Client::OnConnected(alpha::TcpConnectionPtr conn) {
        LOG_INFO << "Connected to " << conn->PeerAddr();
        using namespace std::placeholders;
        conn_ = conn;
        conn_->SetOnRead(std::bind(&Client::OnMessage, this, _1, _2));
        state_ = ConnectionState::kConnected;
        co_->Resume();
    }

    void Client::OnDisconnected(alpha::TcpConnectionPtr conn) {
        assert (conn_ == conn);
        state_ = ConnectionState::kDisconnected;
        conn_.reset();
    }

    void Client::OnMessage(alpha::TcpConnectionPtr conn, 
            alpha::TcpConnectionBuffer* buffer) {
        assert (conn == conn_);
        co_->Resume();
    }
}
