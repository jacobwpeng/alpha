/*
 * =============================================================================
 *
 *       Filename:  tt_protocol_codec.cc
 *        Created:  04/30/15 16:49:12
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#include "tt_protocol_codec.h"
#include <cassert>
#include <alpha/logger.h>

#include "tt_coded_stream.h"

namespace tokyotyrant {
    LengthPrefixedCodecUnit::LengthPrefixedCodecUnit(std::string* val)
        :val_(val) {
    }

    CodecStatus LengthPrefixedCodecUnit::Process(const uint8_t* buffer, int size
            , int* consumed) {
        CodedInputStream stream(buffer, size);
        int32_t vsize;
        if (!stream.ReadBigEndianInt32(&vsize)) {
            return kNeedsMore;
        }

        DLOG_INFO << "Expected value size = " << vsize;

        if (!stream.ReadString(val_, vsize)) {
            return kNeedsMore;
        }
        *consumed = sizeof(int32_t) + vsize;
        return kOk;
    }

    KeyValuePairCodecUnit::KeyValuePairCodecUnit(ResultMap* m, int* rnum)
        :m_(m), rnum_(rnum) {
    }

    CodecStatus KeyValuePairCodecUnit::Process(const uint8_t* buffer, int size, 
            int* consumed) {
        *consumed = 0;
        assert (*rnum_ >= 0);
        while (*rnum_) {
            CodedInputStream stream(buffer, size);
            int32_t ksize, vsize;
            if (!stream.ReadBigEndianInt32(&ksize) || !stream.ReadBigEndianInt32(&vsize)) {
                return CodecStatus::kNeedsMore;
            } 
            std::string key, value;
            if (!stream.ReadString(&key, ksize) || !stream.ReadString(&key, vsize)) {
                return CodecStatus::kNeedsMore;
            }
            m_->emplace(key, value);
            --*rnum_;
            auto bytes = 2 * sizeof(int32_t) + ksize + vsize;
            *consumed += bytes;
        }
        return CodecStatus::kOk;
    }

    void ProtocolCodec::AddUnit(ProtocolCodecUnit* unit) {
        units_.push_back(unit);
    }

    int ProtocolCodec::Process(const uint8_t* buffer, int size) {
        if (current_pos_ == 0) {
            CodedInputStream stream(buffer, size);
            int8_t err;
            if (!stream.ReadBigEndianInt8(&err)) {
                return CodecStatus::kNeedsMore;
            } else if (err != kOk) {
                ++consumed_;
                return err;
            } else {
                ++consumed_;
                current_pos_ += sizeof(int8_t);
            }
        }

        if (!units_.empty() && current_unit_index_ < units_.size()) {
            for (size_t idx = current_unit_index_; idx < units_.size(); ++idx) {
                int consumed = 0;
                auto unit = units_[idx];
                auto status = unit->Process(buffer + current_pos_,
                        size - current_pos_, &consumed);
                switch(status) {
                    case CodecStatus::kOk:
                        current_pos_ += consumed;
                        consumed_ += consumed;
                        ++current_unit_index_;
                        break;
                    default:
                        return status;
                }
            }
        }
        LOG_INFO << "size = " << size << ", current_pos_ = " << current_pos_;
        return size == current_pos_ ? kOk : kNotConsumed;
    }

    int ProtocolCodec::ConsumedBytes() const {
        return consumed_;
    }

    void ProtocolCodec::ClearConsumed() {
        current_pos_ = 0;
        consumed_ = 0;
    }
}
