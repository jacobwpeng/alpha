/*
 * ==============================================================================
 *
 *       Filename:  http_headers.cc
 *        Created:  05/24/15 16:19:02
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * ==============================================================================
 */

#include "http_headers.h"

namespace alpha {
HTTPHeaders::HTTPHeaders() {
  header_names_.reserve(kInitializeHeaderSize);
  header_names_.reserve(kInitializeHeaderSize);
}

void HTTPHeaders::Add(alpha::Slice name, alpha::Slice value) {
  header_names_.emplace_back(name.ToString());
  header_values_.emplace_back(value.ToString());
}

void HTTPHeaders::Set(alpha::Slice name, alpha::Slice value) {
  Remove(name);
  Add(name, value);
}

bool HTTPHeaders::Remove(alpha::Slice target) {
  bool removed = false;
  std::for_each(header_names_.begin(), header_names_.end(),
                [&removed, &target](std::string& name) {
    if (name == target.ToString()) {
      name.clear();
      removed = true;
    }
  });
  return removed;
}

bool HTTPHeaders::Exists(alpha::Slice name) const {
  return std::find(header_names_.begin(), header_names_.end(),
                   name.ToString()) != header_names_.end();
}

std::string HTTPHeaders::Get(alpha::Slice name) const {
  auto it =
      std::find(header_names_.begin(), header_names_.end(), name.ToString());
  if (it == header_names_.end()) {
    return "";
  } else {
    return header_values_[std::distance(header_names_.begin(), it)];
  }
}
}
