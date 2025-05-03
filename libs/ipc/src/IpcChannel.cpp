#include <aether/ipc/IpcChannel.hpp>

#include <spdlog/spdlog.h>

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
                return false;

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
            std::string str;

            const int bytesBeforeWrap = std::min(IPC_BUFFER_SIZE - recvPtr, bytesReceived);

            str += std::string_view{
                reinterpret_cast<char*>(this->in_->buffer.data() + recvPtr),
                static_cast<size_t>(bytesBeforeWrap)
            };
            recvPtr += bytesBeforeWrap;

            if (recvPtr == IPC_BUFFER_SIZE)
                recvPtr = 0;

            if (bytesBeforeWrap < bytesReceived) {
                const int bytesAfterWrap = bytesReceived - bytesBeforeWrap;

                str += std::string_view{
                    reinterpret_cast<char*>(this->in_->buffer.data() + recvPtr),
                    static_cast<size_t>(bytesAfterWrap)
                };
                recvPtr += bytesAfterWrap;
            }

            spdlog::info("Receiver: Received {} bytes: {}", bytesReceived, str);

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
            if (!this->currentPacket_) {
                if (this->packets_.empty())
                    break;

                this->currentPacket_ = std::move(this->packets_.front());
                this->packets_.pop();
                this->pos = 0;

                spdlog::info("Sender: Beginning to write a packet with {} bytes", this->currentPacket_->length());
            }

            std::string& currentPacket = *this->currentPacket_;

            const int bytesToSend = std::min(static_cast<int>(currentPacket.length()) - this->pos, space);
            const int bytesToWrite = std::min(bytesToSend, IPC_BUFFER_SIZE - sendPtr);

            std::memcpy(
                    this->out_->buffer.data() + sendPtr,
                    currentPacket.data() + this->pos,
                    bytesToWrite);
            sendPtr += bytesToWrite;
            this->pos += bytesToWrite;
            didSomething = true;

            spdlog::info("Sender: Wrote {} bytes", bytesToWrite);

            // Wrap around to 0 if we reach the end of the ring buffer.
            if (sendPtr == IPC_BUFFER_SIZE) {
                sendPtr = 0;
            }

            if (this->pos == currentPacket.length()) {
                this->currentPacket_ = std::nullopt;
            }
        }

        this->out_->sendPtr = sendPtr;
        return didSomething;
    }
}