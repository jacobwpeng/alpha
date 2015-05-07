/*
 * =====================================================================================
 *
 *       Filename:  channel.cc
 *        Created:  08/22/14 15:13:44
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =====================================================================================
 */

#include "channel.h"

#include <sys/epoll.h>
#include <cassert>
#include <sstream>

#include "logger.h"
#include "event_loop.h"

namespace alpha {

    const int Channel::kReadEvents = EPOLLIN;
    const int Channel::kWriteEvents = EPOLLOUT;
    const int Channel::kNoneEvents = 0;

    Channel::Channel(EventLoop * loop, int fd)
        :loop_(loop), fd_(fd), events_(0), revents_(0), handling_events_(false) {
    }

    Channel::~Channel() {
        LOG_ERROR_IF(handling_events_) << "Destroy Channel when handling events"
            << ", fd = " << fd_;
        assert (!handling_events_);
        DLOG_INFO << "Channel destroyed, fd = " << fd_;
    }

    void Channel::Remove() {
        loop_->RemoveChannel(this);
    }

    void Channel::Update() {
        loop_->UpdateChannel(this);
    }

    void Channel::HandleEvents() {
        handling_events_ = true;
        if (revents_ & (EPOLLERR | EPOLLHUP)) {
            if (ecb_) ecb_();
            else {
                LOG_WARNING << "Got error for fd = " << fd_;
            }
        }

        if (revents_ & Channel::kWriteEvents) {
            if (wcb_) wcb_();
        }

        if (revents_ & Channel::kReadEvents) {
            if (rcb_) rcb_();
        }
        handling_events_ = false;
    }

    std::string Channel::ReadableEvents() const {
        std::ostringstream oss;
        oss << "revents_ = ";
        if (revents_ & EPOLLIN) oss << "EPOLLIN ";
        if (revents_ & EPOLLOUT) oss << "EPOLLOUT ";
        if (revents_ & EPOLLPRI) oss << "EPOLLPRI ";
        if (revents_ & EPOLLERR) oss << "EPOLLERR ";
        if (revents_ & EPOLLHUP) oss << "EPOLLHUP ";
        if (revents_ & EPOLLET) oss << "EPOLLET ";

        return oss.str();
    }
}
