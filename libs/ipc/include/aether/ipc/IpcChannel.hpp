#pragma once

#include <array>
#include <atomic>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <string_view>
#include <vector>

namespace aether::ipc {
    enum class IpcRole {
        DLL,
        LAUNCHER
    };

    static constexpr auto IPC_MAGIC = std::to_array("AetherIPC");
    static constexpr int IPC_VERSION = 0;
    static constexpr int IPC_BUFFER_SIZE = 4;

    struct IpcData {
        struct Mailbox {
            std::remove_const_t<decltype(IPC_MAGIC)> magic;
            int version;

            // Sender sets to true to indicate that it's listening
            // Receiver sets to false when it thinks the Sender disconnected
            // Sender also sets to false when the connection is gracefully terminated
            bool active;
            // Receiver sets to true to indicate the connection is being reset, Sender sets to false to ack
            bool reset;
            // Sender sets to true to ask Receiver to reset the connection, Sender sets to false to ack
            bool requestReset;

            // Sender moves sendPtr up until before recvPtr
            int sendPtr;
            // Receiver moves recvPtr up until it equals sendPtr
            int recvPtr;

            std::array<uint8_t, IPC_BUFFER_SIZE> buffer;
        } dll, launcher;
    };

    class Receiver {
    public:
        explicit Receiver(IpcData::Mailbox* in);

        Receiver(const Receiver&) = delete;

        bool update();

    private:
        IpcData::Mailbox* in_;
        bool didReset_ = false;

        void reset();
    };

    class Sender {
    public:
        explicit Sender(IpcData::Mailbox* out);

        Sender(const Sender&) = delete;

        bool update();

        void test(std::string_view str) {
            std::lock_guard _{ this->testMtx_ };
            this->packets_.emplace(str);
        }

    private:
        IpcData::Mailbox* out_;

        std::mutex testMtx_;

        std::optional<std::string> currentPacket_;
        int pos = 0;

        std::queue<std::string> packets_;
    };
}