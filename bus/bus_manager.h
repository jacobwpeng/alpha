/*
 * ==============================================================================
 *
 *       Filename:  bus_manager.h
 *        Created:  12/24/14 11:03:06
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#ifndef  __BUS_MANAGER_H__
#define  __BUS_MANAGER_H__

#include <map>
#include <memory>
#include <boost/property_tree/ptree_fwd.hpp>
#include "process_bus.h"

namespace alpha {

    class BusManager {
        public:
            /* 外层用try...catch...来保证初始化成功 */
            BusManager(const boost::property_tree::ptree& pt);

            std::unique_ptr<alpha::ProcessBus> CreateBus(int busid) const;
            std::unique_ptr<alpha::ProcessBus> RestoreBus(int busid) const;
            std::unique_ptr<alpha::ProcessBus> RestoreOrCreateBus(
                    int busid, bool force = false) const;

        private:
            BusManager(const BusManager&) = delete;
            BusManager& operator=(const BusManager&) = delete;

            struct BusInfo {
                int id;
                std::string filepath;
                size_t filesize;
            };

            /* busid -> BusInfo */
            std::map<int, BusInfo> buses_;
    };
}

#endif   /* ----- #ifndef __BUS_MANAGER_H__  ----- */
