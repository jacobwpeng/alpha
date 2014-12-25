/*
 * ==============================================================================
 *
 *       Filename:  logsvrd.cc
 *        Created:  12/21/14 15:56:22
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#include "logsvrd.h"
#include <unistd.h>
#include <signal.h>
#include <cstdio>
#include <sys/stat.h>
#include <dirent.h>
#include <thread>
#include <google/gflags.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include "bus/bus_manager.h"
#include "logger/log_file.h"

namespace alpha {
    namespace detail {
        static std::atomic<bool> running_;

        void SignalHandler(int signo) {
            switch (signo) {
                case SIGINT:
                case SIGQUIT:
                case SIGTERM:
                    running_ = false;
                    break;
                default:
                    break;
            }
        }

        void WorkerRoutine(std::unique_ptr<alpha::ProcessBus> bus, 
                const std::string& log_dir) {
            assert (bus);
            std::string path = bus->filepath();
            size_t pos = path.find_last_of("/");
            std::string basename;
            if (pos == std::string::npos) {
                basename = path;
            } else {
                basename = path.substr(pos + 1);
            }
            std::string fullpath = log_dir + "/" + basename;
            FILE * fp = ::fopen(fullpath.c_str(), "a");
            if (fp == NULL) return;

            const int kFlushInterval = 3;
            const int kMaxIdleLoop = 100;
            /* useconds*/
            const int kIdleSleepTime = 200 * 1000;
            const int kNormalSleepTime = 50 * 1000;

            int idle = 0;
            while (detail::running_) {
                if (bus->empty()) {
                    ++idle;
                    if (idle == kFlushInterval) {
                        ::fflush(fp);
                    }

                    if (idle < 0) {  /* overflow */
                        idle = kMaxIdleLoop;
                    }

                    ::usleep(idle >= kMaxIdleLoop ? 
                            kIdleSleepTime : kNormalSleepTime);
                } else {
                    idle = 0;
                    int len;
                    const char * buf = bus->Read(&len);
                    assert (buf);
                    ::fwrite(buf, sizeof(char), len, fp);
                }
            }
            ::fclose(fp);
        }
    }

    LogServer::LogServer() {
    }

    bool LogServer::InitFromFile(const alpha::Slice& filepath) {
        using namespace boost::property_tree;

        ptree pt;
        try {
            read_xml(filepath.ToString(), pt, 
                    boost::property_tree::xml_parser::no_comments);

            bus_mgr_ = std::unique_ptr<alpha::BusManager>(
                    new alpha::BusManager(pt.get_child("buses")));
            log_dir_ = pt.get<std::string>("logdir.<xmlattr>.path");
            for (const auto & child : pt.get_child("clients")) {
                if (child.first != "client") continue;

                int busid = child.second.get<int>("<xmlattr>.busid");
                bus_ids_.push_back(busid);
            }

            return true;
        } catch(ptree_error& e) {
            ::fprintf(stderr, "LogServer::InitFromFile failed, %s\n", e.what());
            return false;
        }
    }

    int LogServer::Run() {
        int ret = CreateLogDir();
        if (ret != 0) {
            return ret;
        }
        detail::running_ = true;
        RegisterSignalHandler();
        CreateAndWaitWorkers();
        return 0;
    }

    int LogServer::CreateLogDir() {
        DIR * dir = opendir(log_dir_.c_str());
        if (dir) {
            ::closedir(dir);
            return 0;
        } else if (ENOENT == errno) {
            int ret = ::mkdir(log_dir_.c_str(), 0777);
            if (ret != 0) {
                perror("mkdir");
            }
            return ret;
        } else {
            perror("opendir");
            return -1;
        }
    }

    void LogServer::CreateAndWaitWorkers() {
        std::vector<std::thread> threads;
        for (size_t i = 0; i < bus_ids_.size(); ++i) {
            auto bus = bus_mgr_->RestoreOrCreateBus(bus_ids_[i], true);
            threads.emplace(threads.end(), 
                    detail::WorkerRoutine, std::move(bus), log_dir_);
        }

        for (auto & thread : threads) thread.join();
    }

    void LogServer::RegisterSignalHandler() {
        ::signal(SIGINT, detail::SignalHandler);
        ::signal(SIGTERM, detail::SignalHandler);
        ::signal(SIGQUIT, detail::SignalHandler);
    }
}


int main(int argc, char * argv[]) {
    if (argc != 2) {
        ::fprintf(stderr, "Usage: %s conf\n", argv[0]);
        return -1;
    }

    alpha::LogServer server;
    if (server.InitFromFile(argv[1]) == false) {
        ::fprintf(stderr, "InitFromFile failed.\n");
        return -2;
    }

    return server.Run();
}
