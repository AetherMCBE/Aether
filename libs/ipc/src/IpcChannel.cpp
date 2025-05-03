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

        if (recvPtr == sendPtr)
            return didSomething;

        int count = sendPtr - recvPtr;
        if (count < 0)
            count += IPC_BUFFER_SIZE;

        spdlog::info("Receiver: Received {} bytes", count);

        recvPtr = sendPtr;

        this->in_->recvPtr = recvPtr;
        return true;
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
        if (this->test_ > 0) {
            // Maximum amount of bytes we can send before having to wait for the Receiver
            int space = recvPtr - 1 - sendPtr;
            if (space < 0)
                space += IPC_BUFFER_SIZE;

            const int bytesToSend = std::min(this->test_, space);

            if (bytesToSend > 0) {
                //int bytesUntilWrap = IPC_BUFFER_SIZE - 1 - sendPtr + 1;
                sendPtr = (sendPtr + bytesToSend) % IPC_BUFFER_SIZE;

                this->test_ -= bytesToSend;
                spdlog::info("Sender: Sent {} bytes", bytesToSend);

                didSomething = true;
            }
        }

        this->out_->sendPtr = sendPtr;
        return didSomething;
    }
}