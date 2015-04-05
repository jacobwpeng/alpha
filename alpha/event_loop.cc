/*
 * =====================================================================================
 *
 *       Filename:  event_loop.cc
 *        Created:  08/22/14 15:05:44
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =====================================================================================
 */

#include "event_loop.h"

#include "logger.h"
#include "channel.h"
#include "poller.h"

namespace alpha
{
    EventLoop::EventLoop()
        :quit_(false), iteration_(0), wait_time_(20), idle_time_(100) {
        poller_.reset(new Poller);
    }

    EventLoop::~EventLoop() {
    }

    void EventLoop::Run() {
        ChannelList channels;

        int timeout = wait_time_;//ms
        unsigned idle = 0;

        int status;
        while (not quit_) {
            alpha::TimeStamp now = poller_->Poll(timeout, &channels);
            (void)now;
            ++iteration_;
            DLOG_INFO_IF(!channels.empty()) << "channels.size() = " << channels.size()
                << ", now = " << now;

            for(auto channel : channels) {
                channel->HandleEvents();
            }
            if (channels.size() == 0) ++idle;
            else idle = 0;
            channels.clear();

            if (period_functor_) {
                status = period_functor_(iteration_);
            }

            for (const auto & functor : queued_functors_) {
                functor();
            }

            queued_functors_.clear();

            if (idle >= 100 && status == kIdle) {
                timeout = idle_time_;
            } else {
                timeout = wait_time_;
            }
        }
        LOG_INFO << "EventLoop exiting...";
    }

    void EventLoop::Quit() {
        quit_ = true;
    }

    void EventLoop::UpdateChannel(Channel * channel) {
        poller_->UpdateChannel(channel);
    }

    void EventLoop::RemoveChannel(Channel * channel) {
        poller_->RemoveChannel(channel);
    }

    void EventLoop::QueueInLoop(const EventLoop::Functor& functor) {
        queued_functors_.push_back(functor);
    }
}
