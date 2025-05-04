#pragma once

#include <array>
#include <atomic>
#include <mutex>
#include <optional>
#include <queue>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <aether/core/Boost.hpp>

namespace aether::ipc {
    namespace detail {
        struct Header {
            uint8_t type;
            uint32_t length;
        };

        static constexpr size_t HEADER_SIZE = 5;
    }

    static constexpr auto IPC_MAGIC = std::to_array("AetherIPC");
    static constexpr int IPC_VERSION = 0;
    static constexpr int IPC_BUFFER_SIZE = 2048;

    enum class IpcRole {
        DLL,
        LAUNCHER
    };

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

            std::array<std::byte, IPC_BUFFER_SIZE> buffer;
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

        std::optional<detail::Header> header_;
        core::StaticVector<std::byte, detail::HEADER_SIZE> headerBuf;
        std::vector<std::byte> payloadBuf;

        void reset();

        void receiveBytes(std::span<const std::byte> bytes);
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

        struct OutgoingPacket {
            std::vector<std::byte> data;
            size_t pos = 0;
        };

        std::optional<OutgoingPacket> outgoing_;

        std::queue<std::string> packets_;
    };
}