#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include <aether/ipc/IpcChannel.hpp>

namespace ipc = aether::ipc;

using namespace std::chrono_literals;

int main() {
    ipc::IpcData data;

    ipc::Receiver receiver{ &data.dll };
    ipc::Sender sender{ &data.dll };

    std::jthread recvThread([&](std::stop_token token) {
        while (!token.stop_requested()) {
            receiver.update();
            std::this_thread::sleep_for(5ms);
        }
    });

    std::jthread sendThread([&](std::stop_token token) {
        while (!token.stop_requested()) {
            sender.update();
            std::this_thread::sleep_for(5ms);
        }
    });

    while (true) {
        std::string line;
        std::getline(std::cin, line);

        if (line == "exit")
            break;

        try {
            sender.test(line);
        } catch (std::exception&) {
            // ignore
        }
    }

    return 0;
}