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
    Int32DecodeUnit::Int32DecodeUnit(int32_t* val)
        :val_(val) {
    }

    CodecStatus Int32DecodeUnit::Decode(const uint8_t* buffer, int size, int* consumed) {
        *consumed = 0;
        if (static_cast<size_t>(size) < sizeof(int32_t)) {
            return kNeedsMore;
        }

        CodedInputStream stream(buffer, size);
        bool ok = stream.ReadBigEndianInt32(val_);
        *consumed = sizeof(int32_t);
        assert (ok);
        return kOk;
    }

    Int64DecodeUnit::Int64DecodeUnit(int64_t* val)
        :val_(val) {
    }

    CodecStatus Int64DecodeUnit::Decode(const uint8_t* buffer, int size, int* consumed) {
        *consumed = 0;
        if (static_cast<size_t>(size) < sizeof(int64_t)) {
            return kNeedsMore;
        }

        CodedInputStream stream(buffer, size);
        bool ok = stream.ReadBigEndianInt64(val_);
        *consumed = sizeof(int64_t);
        assert (ok);
        return kOk;
    }

    LengthPrefixedDecodeUnit::LengthPrefixedDecodeUnit(std::string* val)
        :val_(val) {
        assert (val_->empty());
    }

    CodecStatus LengthPrefixedDecodeUnit::Decode(const uint8_t* buffer, int size
            , int* consumed) {
        CodedInputStream stream(buffer, size);
        if (vsize_ == -1 && !stream.ReadBigEndianInt32(&vsize_)) {
            return kNeedsMore;
        }
        LOG_INFO << "Expected size = " << vsize_
            << ", val_->size() = " << val_->size();
        assert (vsize_ >= static_cast<int32_t>(val_->size()));
        bool ok = stream.ReadPartialString(val_, vsize_ - val_->size());
        *consumed += stream.pos();
        return ok ? kOk : kNeedsMore;
    }

    KeyValuePairDecodeUnit::KeyValuePairDecodeUnit(ResultMap* m, int* rnum)
        :m_(m), rnum_(rnum) {
    }

    CodecStatus KeyValuePairDecodeUnit::Decode(const uint8_t* buffer, int size, 
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

    KeyValuePairEncodeUnit::KeyValuePairEncodeUnit(alpha::Slice key, alpha::Slice val)
        :key_(key), val_(val) {
    }

    CodecStatus KeyValuePairEncodeUnit::Encode(CodedOutputStream* stream) {
        return stream->WriteKeyValuePair(key_, val_) ? kOk : kFullBuffer;
    }

    LengthPrefixedEncodeUnit::LengthPrefixedEncodeUnit(alpha::Slice val)
        :val_(val) {
    }

    CodecStatus LengthPrefixedEncodeUnit::Encode(CodedOutputStream* stream) {
        return stream->WriteLengthPrefixedString(val_) ? kOk : kFullBuffer;
    }

    ProtocolCodec::ProtocolCodec(int16_t magic, WriteFunctor w, LeftSpaceFunctor l)
        :magic_(magic), w_(w), l_(l) {
    }

    void ProtocolCodec::AddDecodeUnit(ProtocolDecodeUnit* unit) {
        decode_units_.push_back(unit);
    }

    void ProtocolCodec::AddEncodeUnit(ProtocolEncodeUnit* unit) {
        encode_units_.push_back(unit);
    }

    int ProtocolCodec::Decode(const uint8_t* buffer, int size) {
        DLOG_INFO << "size = " << size;
        int pos = 0;
        if (!parsed_code_) {
            CodedInputStream stream(buffer, size);
            int8_t err;
            if (!stream.ReadInt8(&err)) {
                return CodecStatus::kNeedsMore;
            } else {
                ++consumed_;
                DLOG_INFO << "Decode err done, err = " << static_cast<int>(err);
                parsed_code_ = true;
                if(err) {
                    return err;
                } else {
                    pos += sizeof(int8_t);
                }
            }
        }

        if (!decode_units_.empty() && current_unit_index_ < decode_units_.size()) {
            for (size_t idx = current_unit_index_; idx < decode_units_.size(); ++idx) {
                int consumed = 0;
                auto unit = decode_units_[idx];
                auto status = unit->Decode(buffer + pos,
                        size - pos, &consumed);
                switch(status) {
                    case CodecStatus::kOk:
                        pos += consumed;
                        consumed_ += consumed;
                        ++current_unit_index_;
                        break;
                    case CodecStatus::kNeedsMore:
                        pos += consumed;
                        consumed_ += consumed;
                    default:
                        return status;
                }
                DLOG_INFO << "Decode unit " << idx << " done";
            }
        }
        LOG_INFO << "size = " << size << ", pos = " << pos;
        return size == pos ? kOk : kNotConsumed;
    }

    int ProtocolCodec::ConsumedBytes() const {
        return consumed_;
    }

    void ProtocolCodec::ClearConsumed() {
        consumed_ = 0;
    }

    int16_t ProtocolCodec::magic() const {
        return magic_;
    }

    bool ProtocolCodec::Encode() {
        CodedOutputStream stream(this);
        if (!encoded_magic_) {
            if (!stream.WriteBigEndianInt16(magic_)) {
                return false;
            }
            DLOG_INFO << "Write magic done";
            encoded_magic_ = true;
        }

        for (auto idx = current_unit_index_; idx < encode_units_.size(); ++idx) {
            if (kFullBuffer == encode_units_[idx]->Encode(&stream)) {
                return false;
            }
            DLOG_INFO << "Encode unit " << idx << " done";
            ++current_unit_index_;
        }
        current_unit_index_ = 0;
        return true;
    }

    size_t ProtocolCodec::MaxBytesCanWrite() const {
        return l_();
    }

    bool ProtocolCodec::Write(const uint8_t* buffer, int size) {
        return w_(buffer, size);
    }
}
