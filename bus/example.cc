/*
 * ==============================================================================
 *
 *       Filename:  example.cc
 *        Created:  12/21/14 14:26:06
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#include <unistd.h>
#include <cassert>
#include <cstring>
#include <iostream>
#include "process_bus.h"

int main () {
    const char * filepath = "/tmp/example.bus";
    const size_t kFileSize = 1 << 10;
    const int kLoopTimes = 1 << 26;
    std::string message = "Do not, for one repulse, forgo the purpose that"
        "you resolved to effort. \n";

    if (fork() == 0) {
        std::unique_ptr<alpha::ProcessBus> bus(alpha::ProcessBus::CreateFrom(
                    filepath, kFileSize));
        if (!bus) {
            std::cout << "CreateFrom failed.\n";
        }

        for (int i = 0; i < kLoopTimes; ++i) {
            while (!bus->Write(message.data(), message.size()));
        }

    } else {
        std::unique_ptr<alpha::ProcessBus> bus(alpha::ProcessBus::CreateFrom(
                    filepath, kFileSize));

        if (!bus) {
            std::cout << "CreateFrom failed.\n";
        }

        int counter = kLoopTimes;
        while (counter) {
            char * ptr = nullptr;
            int len;
            while ((ptr = bus->Read(&len)) == nullptr);
            assert (len == static_cast<int>(message.size()));
            assert (::strncmp (ptr, message.data(), len) == 0);
            --counter;
        }
    }
}
