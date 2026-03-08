#include "chat_session.hpp"

#include <algorithm>
#include <cctype>
#include <string_view>
#include <utility>

namespace networking_chat
{
namespace
{

[[nodiscard]] auto is_blank(std::string_view value) -> bool
{
    return std::all_of(value.begin(), value.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
}

}  // namespace

auto ChatSession::submit_message(std::string message) -> void
{
    if (message.empty() || is_blank(message))
    {
        return;
    }

    std::scoped_lock lock{mutex_};
    transcript_.push_back(std::move(message));
}

auto ChatSession::queue_user_message(std::string message) -> void
{
    if (message.empty() || is_blank(message))
    {
        return;
    }

    std::scoped_lock lock{mutex_};
    transcript_.push_back(message);
    pending_user_messages_.push_back(std::move(message));
}

auto ChatSession::transcript_snapshot() const -> std::vector<std::string>
{
    std::scoped_lock lock{mutex_};
    return transcript_;
}

auto ChatSession::consume_pending_user_messages() -> std::vector<std::string>
{
    std::scoped_lock lock{mutex_};
    auto messages = std::move(pending_user_messages_);
    pending_user_messages_.clear();
    return messages;
}

}  // namespace networking_chat
