/*
 * =============================================================================
 *
 *       Filename:  tt_protocol_codec.h
 *        Created:  04/30/15 16:36:38
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#ifndef  __TT_PROTOCOL_CODEC_H__
#define  __TT_PROTOCOL_CODEC_H__

#include <cstdint>
#include <map>
#include <vector>
#include <string>
#include <type_traits>
#include <functional>
#include "tt_coded_stream.h"

namespace tokyotyrant {
    enum CodecStatus {
        kOk = 0,
        kNeedsMore = 100,
        kNotConsumed = 101,
        kFullBuffer = 102,
    };

    class ProtocolDecodeUnit {
        public:
            virtual ~ProtocolDecodeUnit() = default;
            virtual CodecStatus Decode(const uint8_t* buffer, int size, int* consumed) = 0;
    };

    class ProtocolEncodeUnit {
        public:
            virtual ~ProtocolEncodeUnit() = default;
            virtual CodecStatus Encode(CodedOutputStream* stream) = 0;
    };

    class Int32DecodeUnit final : public ProtocolDecodeUnit {
        public:
            Int32DecodeUnit(int32_t* val);
            virtual CodecStatus Decode(const uint8_t* buffer, int size, 
                    int* consumed) override;
        private:
            int32_t* val_;
    };

    class Int64DecodeUnit final : public ProtocolDecodeUnit {
        public:
            Int64DecodeUnit(int64_t* val);
            virtual CodecStatus Decode(const uint8_t* buffer, int size, 
                    int* consumed) override;
        private:
            int64_t* val_;
    };

    class LengthPrefixedDecodeUnit final : public ProtocolDecodeUnit {
        public:
            LengthPrefixedDecodeUnit(std::string* val);
            virtual CodecStatus Decode(const uint8_t* buffer, int size, 
                    int* consumed) override;
        private:
            int32_t vsize_ = -1;
            std::string* val_;
    };

    class KeyValuePairDecodeUnit final : public ProtocolDecodeUnit {
        public:
            using ResultMap = std::map<std::string, std::string>;
            KeyValuePairDecodeUnit(ResultMap* m, int* rnum);
            virtual CodecStatus Decode(const uint8_t* buffer, int size, int* consumed) 
                override;
        private:
            std::map<std::string, std::string>* m_;
            int* rnum_;
    };

    template<typename OutputIterator>
    class RepeatedLengthPrefixedDecodeUnit final : public ProtocolDecodeUnit {
        public:
            RepeatedLengthPrefixedDecodeUnit(OutputIterator it)
                :it_(it) {
            }

            virtual CodecStatus Decode(const uint8_t* buffer, int size, int* consumed) 
                override {
            }

        private:
            OutputIterator it_;
    };

    template<typename IntegerType>
    class IntegerEncodeUnit final : public ProtocolEncodeUnit {
        public:
            IntegerEncodeUnit(typename std::enable_if<
                    std::is_same<IntegerType, int32_t>::value
                    || std::is_same<IntegerType, int64_t>::value, IntegerType>::type val)
                :val_(val) {
            }

            virtual CodecStatus Encode(CodedOutputStream* stream) {
                if (std::is_same<IntegerType, int32_t>::value) {
                    stream->WriteBigEndianInt32(val_);
                } else {
                    stream->WriteBigEndianInt64(val_);
                }
            }

        private:
            IntegerType val_;
    };

    //所有传入*EncodeUnit的参数必须保证在对应Encode调用前保持有效
    class KeyValuePairEncodeUnit final : public ProtocolEncodeUnit {
        public:
            KeyValuePairEncodeUnit(alpha::Slice key, alpha::Slice val);
            virtual CodecStatus Encode(CodedOutputStream* stream);

        private:
            alpha::Slice key_;
            alpha::Slice val_;
    };

    class LengthPrefixedEncodeUnit final : public ProtocolEncodeUnit {
        public:
            LengthPrefixedEncodeUnit(alpha::Slice val);
            virtual CodecStatus Encode(CodedOutputStream* stream);

        private:
            alpha::Slice val_;
    };

    template<typename InputIterator, typename SizeType>
    class RepeatedLengthPrefixedEncodeUnit final : public ProtocolEncodeUnit  {
        public:
            RepeatedLengthPrefixedEncodeUnit(InputIterator begin, InputIterator end
                    , SizeType size)
                :encode_size_done_(false), it_(begin), end_(end), size_(size) {
#ifndef NDEBUG
                assert (std::distance(begin, end) == size);
#endif
            }

            virtual CodecStatus Encode(CodedOutputStream* stream) {
                if (!encode_size_done_ && !stream->WriteBigEndianInt32(size_)) {
                    return kFullBuffer;
                }
                while (it_ != end_) {
                    if (!stream->WriteLengthPrefixedString(*it_)) {
                        return kFullBuffer;
                    }
                    ++it_;
                }
                return kOk;
            }

        private:
            bool encode_size_done_;
            InputIterator it_;
            InputIterator end_;
            SizeType size_;
    };

    class ProtocolCodec {
        public:
            using WriteFunctor = std::function<bool (const uint8_t*, int size)>;
            using LeftSpaceFunctor = std::function<size_t ()>;
            ProtocolCodec(int16_t magic, WriteFunctor, LeftSpaceFunctor);
            void AddDecodeUnit(ProtocolDecodeUnit* unit);
            void AddEncodeUnit(ProtocolEncodeUnit* unit);
            //每次Decode完成后外层传入的Buffer都应该加上一个ConsumedBytes的offset
            //同时调用ClearConsumed
            //eg:
            //      char * data_from_somewhere;
            //      int data_size;
            //      int offset = 0;
            //      ProtocolCodec codec;
            //      while (1) {
            //          auto err = codec.Decode(data_from_somewhere + offset,
            //                              data_size - offset);
            //          if (err && err != kNeedsMore) return err;
            //          offset += codec.ConsumedBytes();
            //          assert (offset <= data_size);
            //          codec.ClearConsumed();
            //      }
            int Decode(const uint8_t* buffer, int size);
            int ConsumedBytes() const;
            void ClearConsumed();
            int16_t magic() const;

            bool Encode();
            size_t MaxBytesCanWrite() const;
            bool Write(const uint8_t* buffer, int size);

        private:
            int16_t magic_;
            bool encoded_magic_ = false;
            bool parsed_code_ = false;
            int consumed_ = 0;
            size_t current_unit_index_ = 0;
            WriteFunctor w_;
            LeftSpaceFunctor l_;
            std::vector<ProtocolDecodeUnit*> decode_units_;
            std::vector<ProtocolEncodeUnit*> encode_units_;
    };
}

#endif   /* ----- #ifndef __TT_PROTOCOL_CODEC_H__  ----- */
