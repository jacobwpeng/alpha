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
  public:
    FieldValue Get(const std::string& key) const;
    FieldValue* GetPtr(const std::string& key);
    const FieldValue* GetPtr(const std::string& key) const;
    std::pair<FieldValue*, bool> Insert(const std::string& key,
        const FieldValue& val);

  private:
    std::map<std::string, FieldValue> m_;
};
}

#endif   /* ----- #ifndef __FIELDTABLE_H__  ----- */
