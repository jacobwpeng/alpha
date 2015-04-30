/*
 * =============================================================================
 *
 *       Filename:  tt_coded_stream.h
 *        Created:  04/30/15 16:45:42
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#ifndef  __TT_CODED_STREAM_H__
#define  __TT_CODED_STREAM_H__

#include <cstdint>
#include <string>

namespace tokyotyrant {
    class CodedInputStream {
        public:
            CodedInputStream(const uint8_t* buf, int size);

            bool Skip(int count);
            bool ReadString(std::string* buffer, int size);
            bool ReadBigEndianInt8(int8_t* val);
            bool ReadBigEndianInt32(int32_t* val);

        private:
            bool Overflow(int amount);
            void Advance(int amount);
            const uint8_t* const original_buf_;
            const uint8_t* buf_;
            const int size_;
            int current_pos_;
    };
}

#endif   /* ----- #ifndef __TT_CODED_STREAM_H__  ----- */
