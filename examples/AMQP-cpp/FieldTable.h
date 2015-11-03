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

#ifndef  __FIELDTABLE_H__
#define  __FIELDTABLE_H__

#include <map>
#include "FieldValue.h"
namespace amqp {

class FieldTable {
  private:
    using UnderlyingMap = std::map<std::string, FieldValue>;
  public:
    FieldValue Get(alpha::Slice key) const;
    FieldValue* GetPtr(alpha::Slice key);
    const FieldValue* GetPtr(alpha::Slice key) const;
    std::pair<FieldValue*, bool> Insert(alpha::Slice key,
        const FieldValue& val);
    const UnderlyingMap& underlying_map() const;

  private:
    UnderlyingMap m_;
};
}

#endif   /* ----- #ifndef __FIELDTABLE_H__  ----- */
