#pragma once

#include "common.hpp"
#include "net.hpp"
#include <sys/epoll.h>

#include <cstddef>
#include <optional>
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
        if (n == -1) DS_THROW_ERRNO("recv()");
        if (n == 0) { destroy(); return std::nullopt; }
        return std::vector<std::byte>{buffer_.begin(), buffer_.begin() + n};
    }

    [[nodiscard]] auto is_connected() const noexcept -> bool { return is_connected_; }
    [[nodiscard]] auto fd() const noexcept -> FileDescriptor { return fd_; }

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
            DS_THROW_ERRNO("socket()");
        }
        const auto raw = to_raw_addr(ai.address);
        const auto address = reinterpret_cast<const sockaddr*>(&raw.storage);
        int opt = 1;
        setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        if (bind(fd_, address, raw.len) == -1) {
            close(fd_);
            DS_THROW_ERRNO("bind()");
        }
        if (listen(fd_, k_backlog) == -1) {
            close(fd_);
            DS_THROW_ERRNO("listen()");
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

    auto accept_connection() -> FileDescriptor {
        sockaddr_storage client_addr{};
        socklen_t addr_len = sizeof(client_addr);
        int client_fd = accept(fd_, reinterpret_cast<sockaddr*>(&client_addr), &addr_len);
        if (client_fd == k_invalid_fd) {
            DS_THROW_ERRNO("accept()");
        }
        connections_.insert_or_assign(client_fd, ClientConnection{client_fd});
        return client_fd;
    }

    [[nodiscard]] auto fd() const noexcept -> FileDescriptor { return fd_; }

    auto get_connection(FileDescriptor fd) const -> const ClientConnection& {
        return connections_.at(fd);
    }
    auto get_connection(FileDescriptor fd) -> ClientConnection& {
        return connections_.at(fd);
    }
private:
    static constexpr int k_backlog{5};
    FileDescriptor fd_{k_invalid_fd};
    std::unordered_map<FileDescriptor, ClientConnection> connections_{};

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

class EventPoller {
    public:
        EventPoller() {
            if (fd_ = epoll_create1(0); fd_ == k_invalid_fd) {
                DS_THROW_ERRNO("epoll_ctl epoll_create1()");
            }
        }
        EventPoller(const EventPoller&) = delete;
        EventPoller& operator=(const EventPoller&) = delete;
        EventPoller(EventPoller&& other) noexcept { steal_from(std::move(other)); }
        EventPoller& operator=(EventPoller&& other) noexcept {
            if (this != &other) steal_from(std::move(other));
            return *this;
        }
        ~EventPoller() { destroy(); }
    
        auto add(FileDescriptor fd, u32 events = EPOLLIN) -> void {
            epoll_event ev{};
            ev.events = events;
            ev.data.fd = fd;
            if (epoll_ctl(fd_, EPOLL_CTL_ADD, fd, &ev) == -1) {
                DS_THROW_ERRNO("epoll_ctl ADD");
            }
        }
    
        auto remove(FileDescriptor fd) -> void {
            if (epoll_ctl(fd_, EPOLL_CTL_DEL, fd, nullptr) == -1) {
                DS_THROW_ERRNO("epoll_ctl DEL");
            }
        }
    
        auto wait(int timeout_ms = -1) -> std::span<epoll_event> {
            int n = epoll_wait(fd_, events_.data(), static_cast<int>(events_.size()), timeout_ms);
            if (n == -1) {
                DS_THROW_ERRNO("epoll_wait()");
            }
            return {events_.data(), static_cast<usize>(n)};
        }
    
    private:
        FileDescriptor fd_{k_invalid_fd};
        static constexpr usize k_max_events{64};
        std::array<epoll_event, k_max_events> events_{};
    
        auto steal_from(EventPoller&& other) -> void {
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