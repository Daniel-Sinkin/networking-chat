#pragma once

#include "common.hpp"

#include <array>
#include <cassert>
#include <cerrno>
#include <cstring>
#include <format>
#include <new>
#include <optional>
#include <stdexcept>
#include <variant>
#include <vector>

// POSIX
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace ds_net {

using FileDescriptor = int;
constexpr FileDescriptor k_invalid_fd{-1};

using Port = u16;
using IPv4 = u32;
using IPv6 = std::array<std::byte, 16>;

constexpr Port k_default_port{9000};

struct IP4Address {
    IPv4 ip;
    Port port;
};

struct IP6Address {
    IPv6 ip;
    Port port;
};
using SocketAddress = std::variant<IP4Address, IP6Address>;

struct RawAddress {
    sockaddr_storage storage{};
    socklen_t len{0};
};

inline auto to_raw_addr(const SocketAddress& addr) -> RawAddress {
    RawAddress raw{};
    std::visit(overloaded{
        [&](const IP4Address& a) {
            auto& sa = *reinterpret_cast<sockaddr_in*>(&raw.storage);
            sa.sin_family = AF_INET;
            sa.sin_port = htons(a.port);
            sa.sin_addr.s_addr = a.ip;
            raw.len = sizeof(sockaddr_in);
        },
        [&](const IP6Address& a) {
            auto& sa = *reinterpret_cast<sockaddr_in6*>(&raw.storage);
            sa.sin6_family = AF_INET6;
            sa.sin6_port = htons(a.port);
            std::memcpy(sa.sin6_addr.s6_addr, a.ip.data(), 16);
            raw.len = sizeof(sockaddr_in6);
        },
    }, addr);
    return raw;
}

inline auto to_socket_address(const sockaddr* sa) -> SocketAddress {
    if (!sa) throw std::runtime_error("Nullpointer in to_socket_address");
    if (sa->sa_family == AF_INET) {
        auto* v4 = std::launder(reinterpret_cast<const sockaddr_in*>(sa));
        return IP4Address{
            .ip = v4->sin_addr.s_addr,
            .port = ntohs(v4->sin_port)
        };
    } else if (sa->sa_family == AF_INET6) {
        auto* v6 = std::launder(reinterpret_cast<const sockaddr_in6*>(sa));
        std::array<std::byte, 16> addr{};
        std::memcpy(addr.data(), v6->sin6_addr.s6_addr, 16);
        return IP6Address{
            .ip = addr,
            .port = ntohs(v6->sin6_port),
        };
    } else {
        throw std::runtime_error(std::format("{} is not a supported sa_family", sa->sa_family));
    }
}

inline auto family_str(int f) -> const char* {
    switch (f) {
        case AF_INET:  return "AF_INET (IPv4)";
        case AF_INET6: return "AF_INET6 (IPv6)";
        case AF_UNSPEC: return "AF_UNSPEC";
        default: return "unknown";
    }
}

inline auto socktype_str(int t) -> const char* {
    switch (t) {
        case SOCK_STREAM: return "SOCK_STREAM (TCP)";
        case SOCK_DGRAM:  return "SOCK_DGRAM (UDP)";
        case SOCK_RAW:    return "SOCK_RAW";
        default: return "unknown";
    }
}

inline auto protocol_str(int p) -> const char* {
    switch (p) {
        case IPPROTO_TCP: return "IPPROTO_TCP";
        case IPPROTO_UDP: return "IPPROTO_UDP";
        case 0: return "0 (auto)";
        default: return "unknown";
    }
}

struct AddressInfo {
    int flags{};
    int family{};
    int socket_type{};
    int protocol{};
    SocketAddress address{};
    std::optional<std::string> canonical_name{};

    void print() const {
        std::cout << "  flags:          " << flags << "\n"
                  << "  family:         " << family_str(family) << "\n"
                  << "  socket_type:    " << socktype_str(socket_type) << "\n"
                  << "  protocol:       " << protocol_str(protocol) << "\n"
                  << "  canonical_name: " << canonical_name.value_or("unknown") << "\n";

        std::visit(overloaded{
            [](const IP4Address& addr) {
                char buf[INET_ADDRSTRLEN]{};
                inet_ntop(AF_INET, &addr.ip, buf, sizeof(buf));
                std::cout << "  address:        " << buf << ":" << addr.port << "\n";
            },
            [](const IP6Address& addr) {
                char buf[INET6_ADDRSTRLEN]{};
                inet_ntop(AF_INET6, addr.ip.data(), buf, sizeof(buf));
                std::cout << "  address:        [" << buf << "]:" << addr.port << "\n";
            },
        }, address);
    }
};

inline auto get_address_infos(
    const std::optional<std::string>& host,
    Port port,
    int family,
    int socket_type,
    int flags
) -> std::vector<AddressInfo> {
    addrinfo hints{};
    hints.ai_flags = flags;
    hints.ai_family = family;
    hints.ai_socktype = socket_type;

    const char* host_arg = host ? host->c_str() : nullptr;
    const auto port_str = std::to_string(port);
    struct addrinfo* servinfo;
    if (const auto res = getaddrinfo(host_arg, port_str.c_str(), &hints, &servinfo); res != 0) {
        std::cerr << "gai error: " << gai_strerror(res) << '\n';
        return {};
    }
    ScopeGuard sg{[&]{ freeaddrinfo(servinfo); }};
    std::vector<AddressInfo> out{};
    for (auto* p = servinfo; p; p = p->ai_next) {
        out.push_back(AddressInfo{
            .flags = p->ai_flags,
            .family = p->ai_family,
            .socket_type = p->ai_socktype,
            .protocol = p->ai_protocol,
            .address = to_socket_address(p->ai_addr),
            .canonical_name = p->ai_canonname ? std::string{p->ai_canonname} : std::string{"#MISSING"},
        });
    }
    return out;
}

inline auto get_ipv4_tcp_address_info(const std::optional<std::string>& host, Port port) -> std::optional<AddressInfo> {
    const auto address_infos = get_address_infos(host, port, AF_INET, SOCK_STREAM, AI_PASSIVE);
    if (address_infos.empty()) return std::nullopt;
    assert(address_infos.size() == 1);
    return address_infos[0];
}
inline auto get_ipv4_tcp_address_info(Port port) -> std::optional<AddressInfo> {
    return get_ipv4_tcp_address_info(std::nullopt, port);
}
inline auto get_ipv4_udp_address_info(const std::optional<std::string>& host, Port port) -> std::optional<AddressInfo> {
    const auto address_infos = get_address_infos(host, port, AF_INET, SOCK_DGRAM, AI_PASSIVE);
    if (address_infos.empty()) return std::nullopt;
    assert(address_infos.size() == 1);
    return address_infos[0];
}
inline auto get_ipv4_udp_address_info(Port port) -> std::optional<AddressInfo> {
    return get_ipv4_udp_address_info(std::nullopt, port);
}
inline auto get_ipv4_tcp_connect_info(const std::string& host, Port port) -> std::optional<AddressInfo> {
    const auto address_infos = get_address_infos(host, port, AF_INET, SOCK_STREAM, 0);
    if (address_infos.empty()) return std::nullopt;
    assert(address_infos.size() == 1);
    return address_infos[0];
}

} // namespace ds_net