/*
 * ==============================================================================
 *
 *       Filename:  bus_manager.cc
 *        Created:  12/24/14 11:23:44
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#include "bus_manager.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/foreach.hpp>

namespace alpha {

    using boost::property_tree::ptree;
    BusManager::BusManager(const ptree& pt) {
#if 0
        <buses>
            <bus id="10001" path="/tmp/bus.log" size="1048576" />
        </buses>
#endif
        BOOST_FOREACH(const ptree::value_type& child, pt.get_child("")) {
            if (child.first != "bus") continue;

            const ptree& bus = child.second;
            BusInfo info = {
                .id = bus.get<int>("<xmlattr>.id"),
                .filepath = bus.get<std::string>("<xmlattr>.path"),
                .filesize = bus.get<size_t>("<xmlattr>.size")
            };
            buses_.emplace(info.id, info);
        }
    }

    std::unique_ptr<alpha::ProcessBus> BusManager::CreateBus(int busid) const {
        auto it = buses_.find(busid);
        if (it != buses_.end()) {
            return alpha::ProcessBus::CreateFrom(it->second.filepath, 
                    it->second.filesize);
        }
        return nullptr;
    }

    std::unique_ptr<alpha::ProcessBus> BusManager::RestoreBus(int busid) const {
        auto it = buses_.find(busid);
        if (it != buses_.end()) {
            return alpha::ProcessBus::RestoreFrom(it->second.filepath, 
                    it->second.filesize);
        }
        return nullptr;
    }

    std::unique_ptr<alpha::ProcessBus> BusManager::RestoreOrCreateBus(
            int busid, bool force)  const {
        auto it = buses_.find(busid);
        if (it != buses_.end()) {
            return alpha::ProcessBus::RestoreOrCreate(it->second.filepath, 
                    it->second.filesize, force);
        }
        return nullptr;
    }
}
