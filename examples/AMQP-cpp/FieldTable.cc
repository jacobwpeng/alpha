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

FieldValue FieldTable::Get(const std::string& key) const {
  auto it = m_.find(key);
  return it == m_.end() ? FieldValue() : it->second;
}

FieldValue* FieldTable::GetPtr(const std::string& key) {
  auto it = m_.find(key);
  return it == m_.end() ? nullptr : &it->second;
}

const FieldValue* FieldTable::GetPtr(const std::string& key) const {
  auto it = m_.find(key);
  return it == m_.end() ? nullptr : &it->second;
}

std::pair<FieldValue*, bool> FieldTable::Insert(const std::string& key,
    const FieldValue& val) {
  auto p = m_.insert(std::make_pair(key, val));
  return std::make_pair(&p.first->second, p.second);
}

}
