/*
 * =====================================================================================
 *
 *       Filename:  poller.cc
 *        Created:  08/22/14 14:44:11
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =====================================================================================
 */

#include "poller.h"

#include <cassert>
#include <sys/epoll.h>
#include "logger.h"

namespace alpha {
    Poller::Poller() {
        epoll_fd_ = ::epoll_create(kMaxFdCount);
        //PLOG_IF(ERROR, epoll_fd_ < 0) << "epoll_create failed";
        LOG_ERROR_IF(epoll_fd_ < 0) << "epoll_create failed";
        events_.resize(20);
    }

    Poller::~Poller() {
        if (epoll_fd_ >= 0) ::close(epoll_fd_);
    }

    Poller::TimeStamp Poller::Poll(int timeout_in_ms, 
            ChannelList * active_channels) {
        assert (active_channels);

        int nevents = ::epoll_wait(epoll_fd_, &events_[0], events_.size(), timeout_in_ms);
        Poller::TimeStamp now  = std::chrono::system_clock::now();
        if (nevents > 0) {
            FillActiveChannels(nevents, active_channels);
            if (static_cast<size_t>(nevents) == events_.size()) {
                events_.resize(events_.size() * 2);
            }
        }
        else if (nevents == 0) {
            //Maybe timeout
        }
        else {
            //Not interupted by a signal or something
            //PLOG_IF(ERROR, errno != EINTR) << "epoll_wait return error";
            LOG_ERROR_IF(errno != EINTR) << "epoll_wait return error";
        }
        return now;
    }

    void Poller::UpdateChannel(Channel * channel) {
        ChannelMap::iterator iter = channels_.find(channel->fd());

        if (iter == channels_.end()) {
            //Insert new channel
            epoll_event ev;
            ev.data.fd = channel->fd();
            ev.events = channel->events();
            ::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, channel->fd(), &ev);

            channels_[channel->fd()] = channel;
        } else {
            //Update channel
            epoll_event ev;
            ev.data.fd = channel->fd();
            ev.events = channel->events();

            ::epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, channel->fd(), &ev);
        }
    }

    void Poller::RemoveChannel(Channel * channel) {
        ChannelMap::iterator iter = channels_.find(channel->fd());
        assert (iter != channels_.end());

        channels_.erase(iter);
        epoll_event ev;
        ev.data.fd = channel->fd();
        ::epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, channel->fd(), &ev);
    }

    void Poller::FillActiveChannels(int nevents, ChannelList * active_channels) {
        assert (static_cast<size_t>(nevents) <= events_.size());
        for (int idx = 0; idx < nevents; ++idx) {
            int fd = events_[idx].data.fd;
            ChannelMap::iterator iter = channels_.find(fd);
            assert (iter != channels_.end());

            Channel * channel = iter->second;
            channel->set_revents(events_[idx].events);
            active_channels->push_back(channel);
        }
    }
}
