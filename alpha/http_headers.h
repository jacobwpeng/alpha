/*
 * ==============================================================================
 *
 *       Filename:  http_headers.h
 *        Created:  05/24/15 16:00:07
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#ifndef  __HTTP_HEADERS_H__
#define  __HTTP_HEADERS_H__

#include <vector>
#include <algorithm>
#include <alpha/slice.h>

namespace alpha {
    class HTTPHeaders {
        public:
            HTTPHeaders();
            void Add(alpha::Slice name, alpha::Slice value);
            void Set(alpha::Slice name, alpha::Slice value);
            bool Remove(alpha::Slice name);
            bool Exists(alpha::Slice name) const;
            std::string Get(alpha::Slice name) const;

            template<typename LAMBDA>
            void Foreach(LAMBDA lambda) const;
        private:
            static const int kInitializeHeaderSize = 16;
            std::vector<std::string> header_names_;
            std::vector<std::string> header_values_;
    };

    template<typename LAMBDA>
    void HTTPHeaders::Foreach(LAMBDA lambda) const {
        for (auto i = 0u; i < header_names_.size(); ++i) {
            if (!header_names_[i].empty()) {
                lambda(header_names_[i], header_values_[i]);
            }
        }
    }
}

#endif   /* ----- #ifndef __HTTP_HEADERS_H__  ----- */
