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
void hexdump(const void *ptr, int buflen) {
  const unsigned char *buf = (unsigned char*)ptr;
  int i, j;
  for (i=0; i<buflen; i+=16) {
    printf("%06x: ", i);
    for (j=0; j<16; j++) 
      if (i+j < buflen)
        printf("%02x ", buf[i+j]);
      else
        printf("   ");
    printf(" ");
    for (j=0; j<16; j++) 
      if (i+j < buflen)
        printf("%c", isprint(buf[i+j]) ? buf[i+j] : '.');
    printf("\n");
  }
}

namespace tokyotyrant {
    Client::Client(alpha::EventLoop* loop)
        :loop_(loop), co_(nullptr), state_(ConnectionState::kDisconnected) {
        tcp_client_.reset(new alpha::TcpClient(loop_));
        using namespace std::placeholders;
        tcp_client_->SetOnConnected(std::bind(&Client::OnConnected, this, _1));
        tcp_client_->SetOnConnectError(std::bind(&Client::OnConnectError, this, _1));
        tcp_client_->SetOnClose(std::bind(&Client::OnDisconnected, this, _1));
    }

    Client::~Client() = default;

    void Client::SetCoroutine(alpha::Coroutine* co) {
        co_ = co;
    }

    int Client::Connnect(const alpha::NetAddress& addr) {
        if (addr_ != nullptr) {
            return false;
        } else {
            addr_.reset (new alpha::NetAddress(addr));
            tcp_client_->ConnectTo(addr);
            state_ = ConnectionState::kConnecting;
            co_->Yield();
            LOG_INFO_IF(!Connected()) << "Connect failed";
            return Connected() ? kOk : kRefused;
        }
    }

    bool Client::Connected() const {
        return state_ == ConnectionState::kConnected;
    }

    int Client::Put(alpha::Slice key, alpha::Slice value) {
        if (state_ == ConnectionState::kDisconnected) {
            return kInvalidOperation;
        }
        const int16_t kMagic = 0xC810;
        auto codec = CreateCodec(kMagic);
        KeyValuePairEncodeUnit unit(key, value);
        codec->AddEncodeUnit(&unit);

        return Request(codec.get());
    }

    int Client::PutKeep(alpha::Slice key, alpha::Slice value) {
        if (state_ == ConnectionState::kDisconnected) {
            return kInvalidOperation;
        }
        const int16_t kMagic = 0xC811;
        auto codec = CreateCodec(kMagic);
        KeyValuePairEncodeUnit unit(key, value);
        codec->AddEncodeUnit(&unit);

        int err = Request(codec.get());
        if (err == 1) {
            return kExisting;
        } else {
            return err;
        }
    }

    int Client::PutCat(alpha::Slice key, alpha::Slice value) {
        if (state_ == ConnectionState::kDisconnected) {
            return kInvalidOperation;
        }
        const int16_t kMagic = 0xC812;
        auto codec = CreateCodec(kMagic);
        KeyValuePairEncodeUnit unit(key, value);
        codec->AddEncodeUnit(&unit);

        return Request(codec.get());
    }

    int Client::PutNR(alpha::Slice key, alpha::Slice value) {
        if (state_ == ConnectionState::kDisconnected) {
            return kInvalidOperation;
        }
        const int16_t kMagic = 0xC818;
        auto codec = CreateCodec(kMagic);
        KeyValuePairEncodeUnit unit(key, value);
        codec->AddEncodeUnit(&unit);

        return Request(codec.get());
    }

    int Client::Out(alpha::Slice key) {
        if (state_ == ConnectionState::kDisconnected) {
            return kInvalidOperation;
        }
        const int16_t kMagic = 0xC820;
        auto codec = CreateCodec(kMagic);
        LengthPrefixedEncodeUnit unit(key);
        codec->AddEncodeUnit(&unit);

        return Request(codec.get());
    }

    int Client::Get(alpha::Slice key, std::string* val) {
        assert (val);
        if (state_ == ConnectionState::kDisconnected) {
            return kInvalidOperation;
        }
        const int16_t kMagic = 0xC830;
        auto codec = CreateCodec(kMagic);
        LengthPrefixedEncodeUnit unit(key);
        LengthPrefixedDecodeUnit res(val);
        codec->AddEncodeUnit(&unit);
        codec->AddDecodeUnit(&res);

        int err = Request(codec.get());
        if (err == 1) {
            return kNoRecord;
        } else {
            return err;
        }
    }

    int Client::Stat(std::string* stat) {
        if (state_ == ConnectionState::kDisconnected) {
            return kInvalidOperation;
        }
        const int16_t kMagic = 0xC888;
        auto codec = CreateCodec(kMagic);
        LengthPrefixedDecodeUnit unit(stat);
        codec->AddDecodeUnit(&unit);
        return Request(codec.get());
    }

    int Client::ValueSize(alpha::Slice key, int32_t* size) {
        assert (size);
        if (state_ == ConnectionState::kDisconnected) {
            return kInvalidOperation;
        }
        const int16_t kMagic = 0xC838;
        auto codec = CreateCodec(kMagic);
        LengthPrefixedEncodeUnit unit(key);
        codec->AddEncodeUnit(&unit);
        Int32DecodeUnit res(size);
        codec->AddDecodeUnit(&res);

        int err = Request(codec.get());
        if (err == 1) {
            return kNoRecord;
        } else {
            return err;
        }
    }

    int Client::RecordNumber(int64_t* rnum) {
        assert (rnum);
        if (state_ == ConnectionState::kDisconnected) {
            return kInvalidOperation;
        }
        const int16_t kMagic = 0xC880;
        auto codec = CreateCodec(kMagic);
        Int64DecodeUnit unit(rnum);
        codec->AddDecodeUnit(&unit);

        return Request(codec.get());
    }

    std::unique_ptr<Iterator> Client::NewIterator() {
        if (state_ == ConnectionState::kDisconnected) {
            return nullptr;
        }
        const int16_t kMagic = 0xC850;
        auto codec = CreateCodec(kMagic);

        int err = Request(codec.get());
        if (err) {
            return nullptr;
        } else {
            auto it = std::unique_ptr<Iterator>(new Iterator(this));
            it->Next();
            return std::move(it);
        }
    }

    //int Client::GetForwardMatchKeys(alpha::Slice key, int max_size, MatchKeysCallback cb) {
    //}

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
        DLOG_INFO << "New data, size = " << buffer->Read().size();
        //auto data = buffer->Read();
        //hexdump(data.data(), data.size());
        co_->Resume();
    }

    void Client::OnTimeout() {
    }

    void Client::Next(Iterator* it) {
        assert (it);
        const int16_t kMagic = 0xC851;
        auto codec = CreateCodec(kMagic);
        it->key_.clear();
        LengthPrefixedDecodeUnit unit(&it->key_);
        codec->AddDecodeUnit(&unit);

        int err = Request(codec.get());
        if (err && err != 1) {
            it->status_ = err;
        } else if (err == 1) {
            it->status_ = kNoRecord;
        } else {
            it->status_ = kOk;
        }
    }

    std::unique_ptr<ProtocolCodec> Client::CreateCodec(int magic) {
        using namespace std::placeholders;
        return std::unique_ptr<ProtocolCodec>(new ProtocolCodec(magic
                    , std::bind(&Client::Write, this, _1, _2)
                    , std::bind(&Client::MaxBytesCanWrite, this)));
    }

    int Client::Request(ProtocolCodec* codec) {
        while (!codec->Encode()) {
            if (unlikely(conn_ == nullptr || conn_->closed())) {
                return kSendError;
            } else {
                co_->Yield();
            }
        }

        DLOG_INFO << "Encode done";

        const int16_t kPutNRMagic = 0xC818;
        if (codec->magic() == kPutNRMagic) {
            return 0;
        }

        auto status = kNeedsMore;
        while (status == kNeedsMore) {
            alpha::Slice data = conn_->ReadBuffer()->Read();
            auto buffer = reinterpret_cast<const uint8_t*>(data.data());
            status = static_cast<CodecStatus>(codec->Decode(buffer, data.size()));
            LOG_INFO << "status = " << status;
            if (status == 0 || status == kNeedsMore) {
                auto nbytes = codec->ConsumedBytes();
                LOG_INFO << "ConsumedBytes = " << nbytes;
                codec->ClearConsumed();
                conn_->ReadBuffer()->ConsumeBytes(nbytes);
            } else {
                conn_->ReadBuffer()->ConsumeBytes(data.size());
            }
            if (status == kNeedsMore) {
                co_->Yield();
                if (unlikely(conn_ == nullptr || conn_->closed())) {
                    return kRecvError;
                }
            }
        }
        if (status == 1 || status == 0) {
            return status;
        } else {
            LOG_INFO << "kMiscellaneous, status = " << status;
            return kMiscellaneous;
        }
    }

    size_t Client::MaxBytesCanWrite() {
        return 1 << 31;
    }

    bool Client::Write(const uint8_t* buffer, int size) {
        return conn_->Write(alpha::Slice(reinterpret_cast<const char*>(buffer), size));
    }

    Iterator::Iterator(Client* client)
        :client_(client) {
    }

    std::string Iterator::key() const {
        return key_;
    }

    int Iterator::status() const {
        return status_;
    }

    void Iterator::Next() {
        client_->Next(this);
    }
}
