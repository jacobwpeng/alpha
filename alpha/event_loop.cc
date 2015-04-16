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

#include <type_traits>
#include <algorithm>
#include "logger.h"
#include "channel.h"
#include "poller.h"

namespace alpha
{
    static_assert(std::is_same<EventLoop::Functor, TimerManager::TimerFunctor>::value,
            "Mismatch functor type");

    EventLoop::EventLoop()
        :quit_(false), iteration_(0), wait_time_(20), idle_time_(100) {
        poller_.reset(new Poller);
        timer_manager_.reset (new TimerManager);
    }

    EventLoop::~EventLoop() = default;

    void EventLoop::Run() {
        ChannelList channels;

        static const int kMaxLoopBeforeIdle = 100;
        int timeout = wait_time_;//ms
        unsigned idle = 0;

        int status;
        while (not quit_) {
            alpha::TimeStamp now = poller_->Poll(timeout, &channels);
            ++iteration_;
            DLOG_INFO_IF(!channels.empty()) << "channels.size() = " << channels.size()
                << ", now = " << now;
            //先处理网络消息
            std::for_each(channels.begin(), channels.end(), [](Channel* channel){
                    channel->HandleEvents();
                    });
            idle = channels.empty() ? idle + 1 : 0;
            channels.clear();

            if (period_functor_) {
                status = period_functor_(iteration_);
            }

            
            std::vector<Functor> queued_functors;
            std::swap(queued_functors, queued_functors_);
            //再处理延时调用函数
            std::for_each(queued_functors.begin(), queued_functors.end(), 
                    [](const Functor& f){ f(); });

            auto timer_functors = timer_manager_->Step(now);
            //最后处理超时函数
            std::for_each(timer_functors.begin(), timer_functors.end(), 
                    [](const Functor& f){ f(); });

            timeout = wait_time_;
            if (idle >= kMaxLoopBeforeIdle && status == kIdle) {
                timeout = idle_time_;
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

    TimerManager::TimerId EventLoop::RunAt(alpha::TimeStamp ts, const Functor& f) {
        return timer_manager_->AddTimer(ts, f);
    }

    TimerManager::TimerId EventLoop::RunAfter(uint32_t milliseconds, const Functor& f) {
        alpha::TimeStamp expire_time = alpha::Now() + milliseconds;
        return RunAt(expire_time, f);
    }

    TimerManager::TimerId EventLoop::RunEvery(uint32_t milliseconds, const Functor& f) {
        alpha::TimeStamp expire_time = alpha::Now() + milliseconds;
        return timer_manager_->AddPeriodicalTimer(expire_time, milliseconds, f);
    }

    void EventLoop::RemoveTimer(TimerManager::TimerId id) {
        timer_manager_->RemoveTimer(id);
    }

    bool EventLoop::Expired(TimerManager::TimerId id) const {
        return timer_manager_->Expired(id);
    }

    void EventLoop::QueueInLoop(const EventLoop::Functor& functor) {
        queued_functors_.push_back(functor);
    }
}
