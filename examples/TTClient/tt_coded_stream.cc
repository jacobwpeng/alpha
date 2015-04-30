/*
 * =============================================================================
 *
 *       Filename:  tt_coded_stream.cc
 *        Created:  04/30/15 16:46:42
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#include "tt_coded_stream.h"
#include <arpa/inet.h>
#include <cassert>

namespace tokyotyrant {
    CodedInputStream::CodedInputStream(const uint8_t* buffer, int size)
        :original_buf_(buffer), buf_(buffer), size_(size) {
    }

    bool CodedInputStream::Skip(int count) {
        if (Overflow(count)) {
            return false;
        }
        Advance(count);
        return true;
    }

    bool CodedInputStream::ReadString(std::string* buffer, int size) {
        if (Overflow(size)) {
            return false;
        }
        buffer->assign(reinterpret_cast<const char*>(buf_), size);
        Advance(size);
        return true;
    }

    bool CodedInputStream::ReadBigEndianInt8(int8_t* val) {
        if (Overflow(sizeof(int8_t))) {
            return false;
        }
        *val = *reinterpret_cast<const int8_t*>(buf_);
        Advance(sizeof(int8_t));
        *val = ::ntohs(*val);
        return true;
    }

    bool CodedInputStream::ReadBigEndianInt32(int32_t* val) {
        if (Overflow(sizeof(int32_t))) {
            return false;
        }

        *val = *reinterpret_cast<const int32_t*>(buf_);
        Advance(sizeof(int32_t));
        *val = ::ntohl(*val);
        return true;
    }

    bool CodedInputStream::Overflow(int amount) {
        if (original_buf_ + size_ < buf_ + amount) {
            return true;
        }
        return false;
    }

    void CodedInputStream::Advance(int amount) {
        assert (!Overflow(amount));
        buf_ += amount;
    }
}
