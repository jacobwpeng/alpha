/*
 * =============================================================================
 *
 *       Filename:  FieldTable.h
 *        Created:  10/23/15 15:16:28
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#pragma once

#include <map>
#include "FieldValue.h"
namespace amqp {

class FieldTable {
 public:
  using UnderlyingMap = std::map<std::string, FieldValue>;
  FieldValue Get(alpha::Slice key) const;
  const FieldValue* GetPtr(alpha::Slice key) const;
  FieldValue* MutablePtr(alpha::Slice key);

  std::pair<FieldValue*, bool> Insert(alpha::Slice key, const FieldValue& val);

  bool empty() const;
  const UnderlyingMap& underlying_map() const;

 private:
  UnderlyingMap m_;
};
}

