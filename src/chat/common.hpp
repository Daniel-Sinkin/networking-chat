#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>

#define DS_THROW_ERRNO(func_name) \
    throw std::system_error(errno, std::system_category(), func_name " failed")

namespace ds_net {
using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using i8  = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using f32 = float;
using f64 = double;

using usize = std::size_t;
using isize = std::ptrdiff_t;

template<typename... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

template <typename F>
    requires std::invocable<F>
struct ScopeGuard {
    F fn;
    ~ScopeGuard() { fn(); };
};

class Logger {
public:
    auto logln(std::string_view msg) -> void {
        log(msg); std::cout << '\n';
    }
    auto log(std::string_view msg) -> void {
        std::cout << "[" << name_ << "] " << msg;
    }
    static auto get_logger(const std::string& name) -> Logger& {
        static std::unordered_map<std::string, Logger> loggers;
        auto [it, _] = loggers.emplace(name, Logger{name});
        return it->second;
    }
    static auto get_logger() -> Logger& {
        static Logger default_logger{"default"};
        return default_logger;
    }
private:
    Logger(std::string_view name) : name_(std::string{name}) {}
    std::string name_{};
};

} // namespace ds_net