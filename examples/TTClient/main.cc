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
#include <alpha/slice.h>
#include <alpha/logger.h>
#include <alpha/event_loop.h>
#include <alpha/coroutine.h>
#include <alpha/tcp_client.h>
#include "tt_client.h"

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
    auto addr = alpha::NetAddress("10.6.224.81", 8080);
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
    std::string key = basename(argv[0]);
    std::string value = argv[0];
    alpha::Logger::Init(argv[0], alpha::Logger::LogToStderr);

    alpha::EventLoop loop;
    loop.RunEvery(1000, std::bind(BackupRoutine, &loop, key, value));
    loop.Run();
    return EXIT_SUCCESS;
}
