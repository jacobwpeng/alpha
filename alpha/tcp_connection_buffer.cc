/*
 * =============================================================================
 *
 *       Filename:  tcp_connection_buffer.cc
 *        Created:  04/05/15 12:28:24
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#include "tcp_connection_buffer.h"
#include <cassert>
#include <cstring>
#include "compiler.h"

namespace alpha {
    TcpConnectionBuffer::TcpConnectionBuffer()
        :internal_buffer_(kDefaultBufferSize), read_index_(0), write_index_(0) {
    }

    size_t TcpConnectionBuffer::GetContiguousSpace() const {
        CheckIndex();
        return internal_buffer_.size() - write_index_;
    }

    char* TcpConnectionBuffer::WriteBegin() {
        CheckIndex();
        return &internal_buffer_[write_index_];
    }

    void TcpConnectionBuffer::AddBytes(size_t len) {
        write_index_ += len;
        CheckIndex();
    }

    bool TcpConnectionBuffer::Append(const alpha::Slice& data) {
        if (unlikely(!EnsureSpace(data.size()))) {
            return false;
        }
        assert (GetContiguousSpace() >= data.size());
        ::memcpy(WriteBegin(), data.data(), data.size());
        AddBytes(data.size());
        return true;
    }

    alpha::Slice TcpConnectionBuffer::Read() const {
        CheckIndex();
        return alpha::Slice(&internal_buffer_[read_index_], write_index_ - read_index_);
    }

    size_t TcpConnectionBuffer::ReadAndClear(char* buf, size_t len) {
        alpha::Slice data = Read();
        size_t n = std::min(len, data.size());
        ::memcpy(buf, data.data(), n);
        ConsumeBytes(n);
        return n;
    }

    void TcpConnectionBuffer::ConsumeBytes(size_t n) {
        read_index_ += n;
        CheckIndex();
        if (read_index_ == write_index_) {
            read_index_ = write_index_ = 0; //重置位置
        }
    }

    bool TcpConnectionBuffer::EnsureSpace(size_t n) {
        if (unlikely(n > kMaxBufferSize)) {
            return false;
        }

        size_t space_left = GetContiguousSpace();
        if (space_left < n) {
            size_t space_more = (n - space_left) * 2;
            if (unlikely(internal_buffer_.size() + space_more > kMaxBufferSize)) {
                return false;
            }
            internal_buffer_.resize (internal_buffer_.size() + space_more);
        }
        return true;
    }

    void TcpConnectionBuffer::CheckIndex() const {
        assert (read_index_ <= write_index_);
        assert (write_index_ < internal_buffer_.size());
    }
}
