#pragma once

#include "common.hpp"
#include "net.hpp"

#include <system_error>
#include <utility>

namespace ds_net {

class ClientSocket {
public:
    explicit ClientSocket(const AddressInfo& ai) {
        fd_ = socket(ai.family, ai.socket_type, ai.protocol);
        if (fd_ == k_invalid_fd) {
            DS_THROW_ERRNO("socket()");
        }
        const auto raw = to_raw_addr(ai.address);
        if (connect(fd_, reinterpret_cast<const sockaddr*>(&raw.storage), raw.len) == -1) {
            close(fd_);
            DS_THROW_ERRNO("connect()");
        }
    }

    ClientSocket(const ClientSocket&) = delete;
    ClientSocket& operator=(const ClientSocket&) = delete;
    ClientSocket(ClientSocket&& other) noexcept { steal_from(std::move(other)); }
    ClientSocket& operator=(ClientSocket&& other) noexcept {
        if (this != &other) steal_from(std::move(other));
        return *this;
    }
    ~ClientSocket() { destroy(); }

    auto send_message(std::string_view msg) const -> void {
        send(fd_, msg.data(), msg.length(), 0);
    }
    [[nodiscard]] auto fd() const noexcept -> FileDescriptor { return fd_; }

private:
    FileDescriptor fd_{k_invalid_fd};

    auto steal_from(ClientSocket&& other) -> void {
        destroy();
        fd_ = std::exchange(other.fd_, k_invalid_fd);
    }

    auto destroy() -> void {
        if (fd_ != k_invalid_fd) {
            close(fd_);
            fd_ = k_invalid_fd;
        }
    }
};

} // namespace ds_net