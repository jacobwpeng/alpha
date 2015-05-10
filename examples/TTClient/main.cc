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
#include <sstream>
#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include <alpha/slice.h>
#include <alpha/logger.h>
#include <alpha/event_loop.h>
#include <alpha/coroutine.h>
#include <alpha/tcp_client.h>
#include "tt_client.h"

DEFINE_string(server_ip, "10.6.224.81", "Remote tokyotyrant server ip");
DEFINE_int32(server_port, 8080, "Remote tokyotyrant server port");

class RandomStream {
    public:
        RandomStream() {
            fp_ = ::fopen("/dev/urandom", "rb");
            assert (fp_);
        }

        ~RandomStream() {
            if (fp_) {
                ::fclose(fp_);
            }
        }

        std::string ReadBytes(size_t size) {
            std::string res(size, '\0');
            ::fread(&res[0], 1, size, fp_);
            return res;
        }

        template<typename IntegerType>
        typename std::enable_if<std::is_integral<IntegerType>::value,
                 IntegerType>::type Read() {
            IntegerType res;
            ::fread(&res, sizeof(IntegerType), 1, fp_);
            return res;
        }

    private:
        FILE* fp_;
};

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
    client_->Connnect(addr);
    while (!client_->Connected()) {
        LOG_ERROR << "Connnect to " << addr << " failed.";
        Yield();
    }
    RandomStream stream;

    auto err = client_->Vanish();
    assert (!err);

    int64_t rnum;
    err = client_->RecordNumber(&rnum);
    assert (!err);
    assert (rnum == 0);

    // Put normal key
    std::string key("1048576");
    std::string value = stream.ReadBytes(1024);

    err = client_->Put(key, value);
    assert (!err);
    std::string stored;
    err = client_->Get(key, &stored);
    assert (!err);
    assert (stored == value);
    err = client_->RecordNumber(&rnum);
    assert (!err);
    assert (rnum == 1);

    // MultiGet single key
    std::map<std::string, std::string> single;
    std::vector<alpha::Slice> keys;
    keys.push_back(key);
    err = client_->MultiGet(keys.begin(), keys.end(), &single);
    assert (!err);
    assert (!single.empty());
    assert (single.find(key) != single.end());
    assert (single[key] == value);

    //Put keep
    std::string new_value = stream.ReadBytes(2048);
    err = client_->PutKeep(key, new_value);
    assert (err == tokyotyrant::kExisting);
    err = client_->RecordNumber(&rnum);
    assert (!err);
    assert (rnum == 1);
    err = client_->Get(key, &stored);
    assert (!err);
    assert (stored == value);
    assert (stored != new_value);

    //Put cat
    std::string concat_value = stream.ReadBytes(1234);
    err = client_->PutCat(key, concat_value);
    assert (!err);
    err = client_->RecordNumber(&rnum);
    assert (!err);
    assert (rnum == 1);
    err = client_->Get(key, &stored);
    assert (!err);
    assert (stored == value + concat_value);

    //ValueSize
    int32_t vsize;
    err = client_->ValueSize(key, &vsize);
    assert (!err);
    assert (static_cast<size_t>(vsize) == value.size() + concat_value.size());

    //PutNR
    value = stream.ReadBytes(3456);
    err = client_->PutNR(key, value);
    assert (!err);
    err = client_->RecordNumber(&rnum);
    assert (!err);
    assert (rnum == 1);
    err = client_->Get(key, &stored);
    assert (!err);
    if (stored != value) {
        LOG_WARNING << "Something might be wrong!";
    }

    //Put large keys
    value = stream.ReadBytes(1<<23);
    err = client_->Put(key, value);
    assert (!err);

    err = client_->Get(key, &stored);
    assert (!err);
    assert (stored == value);

    //Out
    err = client_->Out(key);
    assert (!err);
    err = client_->RecordNumber(&rnum);
    assert (!err);
    assert (rnum == 0);
    err = client_->Get(key, &stored);
    assert (err == tokyotyrant::kNoRecord);
    assert (stored.empty());

    //Create test data
    const auto key_count = stream.Read<uint32_t>() % 3000 + 1000;
    //const auto key_count = stream.Read<uint32_t>() % 5 + 1;
    std::map<std::string, std::string> m;
    for (auto i = 0u; i < key_count; ++i) {
        auto ksize = stream.Read<uint32_t>() % 300 + 10;
        auto vsize = stream.Read<uint32_t>() % 300 + 10;
        auto key = stream.ReadBytes(ksize);
        auto val = stream.ReadBytes(vsize);
        m[key] = val;
        err = client_->Put(key, val);
        assert (!err);
    }
    err = client_->RecordNumber(&rnum);
    assert (!err);
    assert (rnum == static_cast<int64_t>(m.size()));
    LOG_INFO << "rnum = " << rnum;

    //MultiGet
    keys.clear();
    std::transform(m.begin(), m.end(), std::back_inserter(keys),
            [](const std::pair<std::string, std::string>& p) {
                return p.first;
    });

    std::map<std::string, std::string> restored;
    err = client_->MultiGet(keys.begin(), keys.end(), &restored);
    assert (!err);

    assert (restored.size() == m.size());
    assert (std::equal(m.begin(), m.end(), restored.begin()));

    //Stat
    std::string tmp;
    err = client_->Stat(&tmp);
    assert (!err);
    alpha::Slice stat(tmp);
    auto pos = stat.find("rnum");
    assert (pos != alpha::Slice::npos);
    stat = stat.RemovePrefix(pos + 4);
    auto first = stat.find("\t");
    auto last = stat.find("\n");
    assert (first != alpha::Slice::npos);
    assert (last != alpha::Slice::npos);
    stat = stat.subslice(first + 1, last - first - 1);
    assert (std::to_string(rnum) == stat.ToString());

    //Clear all data
    err = client_->Vanish();
    assert (!err);

    //GetForwardMatchKeys
    m.clear();
    m["foo"] = "bar";
    m["football"] = "hard";
    m["folly"] = "facebook";
    m["far"] = "LA";

    for (const auto& p : m) {
        err = client_->Put(p.first, p.second);
        assert (!err);
    }
    err = client_->RecordNumber(&rnum);
    assert (!err);
    assert (rnum == static_cast<int64_t>(m.size()));

    std::vector<std::string> matches;
    LOG_INFO << "GetForwardMatchKeys";
    err = client_->GetForwardMatchKeys("foo", 5, std::back_inserter(matches));
    assert (!err);
    assert (matches.size() == static_cast<size_t>(2));
    assert (std::find(matches.begin(), matches.end(), "foo") != matches.end());
    assert (std::find(matches.begin(), matches.end(), "football") != matches.end());

    matches.clear();
    err = client_->GetForwardMatchKeys("f", std::numeric_limits<int32_t>::max()
            , std::back_inserter(matches));
    assert (!err);
    assert (matches.size() == m.size());
    for (const auto& p : m) {
        assert (std::find(matches.begin(), matches.end(), p.first) != matches.end());
        (void)p;
    }
    assert (std::find(matches.begin(), matches.end(), "foo") != matches.end());
    assert (std::find(matches.begin(), matches.end(), "football") != matches.end());

    LOG_INFO << "All tests passed";
}

void BackupRoutine(alpha::EventLoop* loop, alpha::Coroutine* co) {

    co->Resume();
    //if (client == nullptr) {
    //    client.reset (new tokyotyrant::Client(loop));
    //}

    //if (co == nullptr) {
    //    co.reset (new BackupCoroutine(client.get()));
    //    client->SetCoroutine(co.get());
    //    co->Resume();
    //}

    if (co->IsDead()) {
        //co.reset();
        loop->Quit();
    }
}

int main(int argc, char* argv[]) {
    std::string usage = "Show remote tokyotyrant server stat";
    gflags::SetUsageMessage(usage);
    gflags::ParseCommandLineFlags(&argc, &argv, false);
    alpha::Logger::Init(argv[0]);

    alpha::EventLoop loop;
    loop.TrapSignal(SIGINT, [&loop]{
        loop.Quit();
    });
    std::unique_ptr<tokyotyrant::Client> client(new tokyotyrant::Client(&loop));
    std::unique_ptr<BackupCoroutine> co(new BackupCoroutine(client.get()));
    client->SetCoroutine(co.get());
    loop.RunAfter(1000, std::bind(BackupRoutine, &loop, co.get()));
    loop.Run();
    return EXIT_SUCCESS;
}
