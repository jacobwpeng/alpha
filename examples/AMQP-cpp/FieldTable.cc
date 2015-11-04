/*
 * =============================================================================
 *
 *       Filename:  FieldTable.cc
 *        Created:  10/23/15 17:08:10
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#include "FieldTable.h"

namespace amqp {

FieldValue FieldTable::Get(alpha::Slice key) const {
  auto it = m_.find(key.ToString());
  return it == m_.end() ? FieldValue() : it->second;
}

FieldValue* FieldTable::MutablePtr(alpha::Slice key) {
  auto it = m_.find(key.ToString());
  return it == m_.end() ? nullptr : &it->second;
}

const FieldValue* FieldTable::GetPtr(alpha::Slice key) const {
  auto it = m_.find(key.ToString());
  return it == m_.end() ? nullptr : &it->second;
}

std::pair<FieldValue*, bool> FieldTable::Insert(alpha::Slice key,
    const FieldValue& val) {
  auto p = m_.emplace(key.ToString(), val);
  return std::make_pair(&p.first->second, p.second);
}

bool FieldTable::empty() const {
  return m_.empty();
}

const FieldTable::UnderlyingMap& FieldTable::underlying_map() const {
  return m_;
}

}
