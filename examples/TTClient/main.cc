/*
 * =============================================================================
 *
 *       Filename:  main.cc
 *        Created:  04/29/15 16:50:38
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

//#include "tt_client.h"
#include <libgen.h>
#include <map>
#include <memory>
#include <gflags/gflags.h>
#include <alpha/slice.h>
#include <alpha/logger.h>
#include <alpha/event_loop.h>
#include <alpha/coroutine.h>
#include <alpha/tcp_client.h>
#include "tt_client.h"

DEFINE_string(server_ip, "10.6.224.81", "Remote tokyotyrant server ip");
DEFINE_int32(server_port, 8080, "Remote tokyotyrant server port");

class BackupCoroutine final : public alpha::Coroutine {
    public:
        BackupCoroutine(tokyotyrant::Client* client);
        virtual void Routine() override;

    private:
        tokyotyrant::Client* client_;
};

BackupCoroutine::BackupCoroutine(tokyotyrant::Client* client)
    :client_(client) {
}

void BackupCoroutine::Routine() {
    auto addr = alpha::NetAddress(FLAGS_server_ip, FLAGS_server_port);
    if (!client_->Connected()) {
        bool ok = client_->Connnect(addr);
        if (!ok) {
            LOG_ERROR << "Connnect to " << addr << " failed.";
            return;
        }
    }
    std::string stat;
    int err = client_->Stat(&stat);
    if (err) {
        LOG_WARNING << "Stat failed, err = " << err;
    } else {
        LOG_INFO << "stat = \n" << stat;
    }
}

void BackupRoutine(alpha::EventLoop* loop, const std::string& key, 
        const std::string& value) {
    static std::unique_ptr<tokyotyrant::Client> client;
    static std::unique_ptr<BackupCoroutine> co;

    if (client == nullptr) {
        client.reset (new tokyotyrant::Client(loop));
    }

    if (co == nullptr) {
        co.reset (new BackupCoroutine(client.get()));
        client->SetCoroutine(co.get());
    }
    if (co->IsSuspended()) {
        co->Resume();
    }

    if (co->IsDead()) {
        co.reset();
    }
}

int main(int argc, char* argv[]) {
    std::string usage = "Show remote tokyotyrant server stat";
    gflags::SetUsageMessage(usage);
    gflags::ParseCommandLineFlags(&argc, &argv, false);
    std::string key = basename(argv[0]);
    std::string value = argv[0];
    alpha::Logger::Init(argv[0], alpha::Logger::LogToStderr);

    alpha::EventLoop loop;
    loop.RunEvery(1000, std::bind(BackupRoutine, &loop, key, value));
    loop.Run();
    return EXIT_SUCCESS;
}
