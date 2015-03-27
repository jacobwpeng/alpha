/*
 * =====================================================================================
 *
 *       Filename:  poller.h
 *        Created:  08/22/14 14:40:03
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =====================================================================================
 */

#ifndef  __POLLER_H__
#define  __POLLER_H__

#include <map>
#include <vector>
#include <chrono>
#include <boost/noncopyable.hpp>

#include "channel.h"

struct epoll_event;

namespace alpha {
    class Poller : boost::noncopyable {
        public:
            using TimeStamp = std::chrono::system_clock::time_point;
            Poller();
            ~Poller();

            TimeStamp Poll(int timeout, ChannelList * active_channels);

            void UpdateChannel(Channel * channel);
            void RemoveChannel(Channel * channel);

        private:
            typedef std::vector<epoll_event> EventList;
            typedef std::map<int, Channel*> ChannelMap;

            void FillActiveChannels(int nevents, ChannelList * active_channels);

        private:
            const static int kMaxFdCount = 100;
            int epoll_fd_;
            ChannelMap channels_;
            EventList events_;
    };
}

#endif   /* ----- #ifndef __POLLER_H__  ----- */