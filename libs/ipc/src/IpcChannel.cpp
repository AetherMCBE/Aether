#include <aether/ipc/IpcChannel.hpp>

#include <spdlog/spdlog.h>

// 1 byte for packet type
// 4 bytes for payload length
// payload

namespace aether::ipc {
    Receiver::Receiver(IpcData::Mailbox* in) : in_{in} {
        this->in_->magic = IPC_MAGIC;
        this->in_->version = IPC_VERSION;
        this->in_->active = false;

        this->reset();
    }

    bool Receiver::update() {
        bool didSomething = false;

        this->in_->active = true;

        if (this->in_->requestReset) {
            spdlog::info("Receiver: Got reset request");
            this->reset();
            didSomething = true;
        }

        // If didReset_ is true, we're waiting for the sender to acknowledge by setting `reset` back to false.
        if (this->didReset_) {
            if (this->in_->reset)
                return didSomething;

            spdlog::info("Receiver: Sender acknowledged the reset signal");
            this->didReset_ = false;
            didSomething = true;
        }

        const int sendPtr = this->in_->sendPtr;
        int recvPtr = this->in_->recvPtr;

        int bytesReceived = sendPtr - recvPtr;
        if (bytesReceived < 0)
            bytesReceived += IPC_BUFFER_SIZE;

        if (bytesReceived != 0) {
            const int bytesBeforeWrap = std::min(IPC_BUFFER_SIZE - recvPtr, bytesReceived);

            this->receiveBytes({
                this->in_->buffer.data() + recvPtr,
                static_cast<size_t>(bytesBeforeWrap)
            });
            recvPtr += bytesBeforeWrap;

            if (recvPtr == IPC_BUFFER_SIZE)
                recvPtr = 0;

            if (bytesBeforeWrap < bytesReceived) {
                const int bytesAfterWrap = bytesReceived - bytesBeforeWrap;

                this->receiveBytes({
                    this->in_->buffer.data() + recvPtr,
                    static_cast<size_t>(bytesAfterWrap)
                });
                recvPtr += bytesAfterWrap;
            }

            didSomething = true;
        }

        this->in_->recvPtr = recvPtr;
        return didSomething;
    }

    void Receiver::reset() {
        spdlog::info("Receiver: Sending reset signal");

        this->in_->recvPtr = 0;
        this->in_->reset = true;
        this->in_->requestReset = false;
        this->didReset_ = true;
    }

    void Receiver::receiveBytes(std::span<const std::byte> bytes) {
        spdlog::info("Receiver: Read {} bytes", bytes.size());

        // TODO: Instead of a pos, set bytes to a subspan of itself until it's empty
        size_t pos = 0;

        // Try to parse a header, then a payload, and so on until we run out of bytes
        while (pos < bytes.size()) {
            if (!this->header_) {
                size_t amount = std::min(detail::HEADER_SIZE - this->headerBuf.size(), bytes.size() - pos);
                auto subspan = bytes.subspan(pos, amount);
                this->headerBuf.append_range(subspan);
                pos += amount;

                if (this->headerBuf.size() == detail::HEADER_SIZE) {
                    this->header_ = detail::Header {
                        .type = std::bit_cast<uint8_t>(headerBuf[0]),
                        .length = std::bit_cast<uint32_t>(std::array{
                            headerBuf[1],
                            headerBuf[2],
                            headerBuf[3],
                            headerBuf[4],
                        })
                    };
                    this->headerBuf.clear();

                    // TODO: resize() the payloadBuf and use a pos variable
                }
            } else {
                size_t payloadSize = this->header_->length;
                size_t amount = std::min(payloadSize - this->payloadBuf.size(), bytes.size() - pos);
                auto subspan = bytes.subspan(pos, amount);
                this->payloadBuf.append_range(subspan);
                pos += amount;

                if (this->payloadBuf.size() == payloadSize) {
                    spdlog::info("Receiver: Received a packet with a payload of {} bytes: {}",
                        payloadSize,
                        std::string_view{
                            std::bit_cast<const char*>(this->payloadBuf.data()),
                            this->payloadBuf.size()
                        });

                    // TODO: Do something with the packet

                    this->payloadBuf.clear();
                    this->header_ = std::nullopt;
                }
            }
        }
    }



    Sender::Sender(IpcData::Mailbox* out) : out_{out} {}

    bool Sender::update() {
        bool didSomething = false;

        if (this->out_->reset) {
            spdlog::info("Sender: got reset signal from Receiver");
            this->out_->sendPtr = 0;
            this->out_->reset = false;
            didSomething = true;
        }

        const int recvPtr = this->out_->recvPtr;
        int sendPtr = this->out_->sendPtr;

        std::lock_guard lk{ this->testMtx_ };

        while (true) {
            // Determine how much space the ring buffer has left.
            int space = recvPtr - 1 - sendPtr;
            if (space < 0)
                space += IPC_BUFFER_SIZE;

            if (space == 0)
                break;

            // If we aren't currently serializing a packet, get a packet from the queue
            if (!this->outgoing_) {
                if (this->packets_.empty())
                    break;

                std::string& text = this->packets_.front();
                spdlog::info("Sender: beginning to write a packet with a payload {} bytes", text.size());

                std::vector<std::byte> bytes{ detail::HEADER_SIZE + text.size() };

                bytes[0] = std::byte{0};
                auto length = static_cast<uint32_t>(text.size());
                std::memcpy(bytes.data() + 1, std::bit_cast<std::array<std::byte, 4>>(length).data(), 4);
                std::memcpy(bytes.data() + 5, text.data(), text.size());

                this->outgoing_ = OutgoingPacket { std::move(bytes) };
                this->packets_.pop();
            }

            auto& [data, pos] = *this->outgoing_;

            const int bytesToSend = std::min(static_cast<int>(data.size() - pos), space);
            const int bytesToWrite = std::min(bytesToSend, IPC_BUFFER_SIZE - sendPtr);

            std::memcpy(
                    this->out_->buffer.data() + sendPtr,
                    data.data() + pos,
                    bytesToWrite);
            sendPtr += bytesToWrite;
            pos += bytesToWrite;
            didSomething = true;

            spdlog::info("Sender: Wrote {} bytes", bytesToWrite);

            // Wrap around to 0 if we reach the end of the ring buffer.
            if (sendPtr == IPC_BUFFER_SIZE) {
                sendPtr = 0;
            }

            if (pos == data.size()) {
                this->outgoing_ = std::nullopt;
            }
        }

        this->out_->sendPtr = sendPtr;
        return didSomething;
    }

    // TODO: Ensure a packet length won't overflow for a signed int
}