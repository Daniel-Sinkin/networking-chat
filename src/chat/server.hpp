#pragma once

#include "common.hpp"
#include "net.hpp"

#include <cstddef>
#include <optional>
#include <system_error>
#include <utility>
#include <vector>

namespace ds_net {

class ClientConnection {
public:
    explicit ClientConnection(FileDescriptor fd) : fd_(fd) {}
    ClientConnection(const ClientConnection&) = delete;
    ClientConnection& operator=(const ClientConnection&) = delete;
    ClientConnection(ClientConnection&& other) noexcept { steal_from(std::move(other)); }
    ClientConnection& operator=(ClientConnection&& other) noexcept {
        if (this != &other) steal_from(std::move(other));
        return *this;
    }
    ~ClientConnection() { destroy(); }
    
    [[nodiscard]] auto receive_data() -> std::optional<std::vector<std::byte>> {
        auto n = recv(fd_, buffer_.data(), buffer_.size(), 0);
        if (n == -1) throw std::system_error(errno, std::system_category(), "recv() failed");
        if (n == 0) { destroy(); return std::nullopt; }
        return std::vector<std::byte>{buffer_.begin(), buffer_.begin() + n};
    }

    [[nodiscard]] auto is_connected() const noexcept -> bool { return is_connected_; }

private:
    FileDescriptor fd_{k_invalid_fd};
    static constexpr usize k_recv_buffer_size{1024};
    std::array<std::byte, k_recv_buffer_size> buffer_{};
    bool is_connected_{true};

    auto steal_from(ClientConnection&& other) -> void {
        destroy();
        fd_ = std::exchange(other.fd_, k_invalid_fd);
    }

    auto destroy() -> void {
        if (fd_ != k_invalid_fd) {
            close(fd_);
            fd_ = k_invalid_fd;
        }
        is_connected_ = false;
    }
};

class ServerSocket {
public:
    explicit ServerSocket(const AddressInfo& ai) {
        fd_ = socket(ai.family, ai.socket_type, ai.protocol);
        if (fd_ == k_invalid_fd) {
            throw std::system_error(errno, std::system_category(), "socket() failed");
        }
        const auto raw = to_raw_addr(ai.address);
        const auto address = reinterpret_cast<const sockaddr*>(&raw.storage);
        int opt = 1;
        setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        if (bind(fd_, address, raw.len) == -1) {
            close(fd_);
            throw std::system_error(errno, std::system_category(), "bind() failed");
        }
        if (listen(fd_, k_backlog) == -1) {
            close(fd_);
            throw std::system_error(errno, std::system_category(), "listen() failed");
        }
    }
    ServerSocket(const ServerSocket&) = delete;
    ServerSocket& operator=(const ServerSocket&) = delete;
    ServerSocket(ServerSocket&& other) noexcept { steal_from(std::move(other)); }
    ServerSocket& operator=(ServerSocket&& other) noexcept {
        if (this != &other) steal_from(std::move(other));
        return *this;
    }
    ~ServerSocket() { destroy(); }

    auto accept_connection() -> void {
        sockaddr_storage client_addr{};
        socklen_t addr_len = sizeof(client_addr);
        int client_fd = accept(fd_, reinterpret_cast<sockaddr*>(&client_addr), &addr_len);
        if (client_fd == k_invalid_fd) {
            throw std::system_error(errno, std::system_category(), "accept() failed");
        }
        connections_.emplace_back(client_fd);
    }

    auto receive_string() -> std::optional<std::string> {
        if(connections_.empty()) {
            Logger::get_logger("server").logln("Can't receive without connections!");
            return std::nullopt;
        }
        const auto data = *connections_[0].receive_data();
        std::string data_str{reinterpret_cast<const char*>(data.data()), data.size()};
        return data_str;
    }

private:
    static constexpr int k_backlog{5};
    FileDescriptor fd_{k_invalid_fd};
    std::vector<ClientConnection> connections_{};

    auto steal_from(ServerSocket&& other) -> void {
        destroy();
        fd_ = std::exchange(other.fd_, k_invalid_fd);
        connections_ = std::move(other.connections_);
    }

    auto destroy() -> void {
        if (fd_ != k_invalid_fd) {
            close(fd_);
            fd_ = k_invalid_fd;
        }
    }
};

} // namespace ds_net