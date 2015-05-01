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

namespace tokyotyrant {
    enum CodecStatus {
        kOk = 0,
        kNeedsMore = 100,
        kNotConsumed = 101,
    };

    class ProtocolCodecUnit {
        public:
            virtual ~ProtocolCodecUnit() = default;
            virtual CodecStatus Process(const uint8_t* buffer, int size, int* consumed) = 0;
    };

    template<typename NumberType>
    class NumberCodecUnit final : public ProtocolCodecUnit {
        public:
            NumberCodecUnit(NumberCodecUnit* val)
                :val_(val) {
            }

            virtual CodecStatus Process(const uint8_t* buffer, int size, 
                    int* consumed) override {
                if (size < sizeof(NumberType)) {
                    return kNeedsMore;
                }

                *val_ = *reinterpret_cast<const NumberType*>(buffer);
                *consumed = sizeof(NumberType);
                return CodecStatus::kOk;
            }
        private:
            NumberType* val_;
    };

    class LengthPrefixedCodecUnit final : public ProtocolCodecUnit {
        public:
            LengthPrefixedCodecUnit(std::string* val);
            virtual CodecStatus Process(const uint8_t* buffer, int size, 
                    int* consumed) override;
        private:
            std::string* val_;
    };


    class KeyValuePairCodecUnit final : public ProtocolCodecUnit {
        public:
            using ResultMap = std::map<std::string, std::string>;
            KeyValuePairCodecUnit(ResultMap* m, int* rnum);
            virtual CodecStatus Process(const uint8_t* buffer, int size, int* consumed) 
                override;
        private:
            std::map<std::string, std::string>* m_;
            int* rnum_;
    };

    class ProtocolCodec {
        public:
            void AddUnit(ProtocolCodecUnit* unit);
            //每次Process完成后外层传入的Buffer都应该加上一个ConsumedBytes的offset
            //同时调用ClearConsumed
            //eg:
            //      char * data_from_somewhere;
            //      int data_size;
            //      int offset = 0;
            //      ProtocolCodec codec;
            //      while (1) {
            //          auto err = codec.Process(data_from_somewhere + offset,
            //                              data_size - offset);
            //          if (err && err != kNeedsMore) return err;
            //          offset += codec.ConsumedBytes();
            //          assert (offset <= data_size);
            //          codec.ClearConsumed();
            //      }
            int Process(const uint8_t* buffer, int size);
            int ConsumedBytes() const;
            void ClearConsumed();

        private:
            bool parsed_code_ = false;
            int consumed_ = 0;
            size_t current_unit_index_ = 0;
            std::vector<ProtocolCodecUnit*> units_;
    };
}

#endif   /* ----- #ifndef __TT_PROTOCOL_CODEC_H__  ----- */
