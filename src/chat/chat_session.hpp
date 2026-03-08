#pragma once

#include <mutex>
#include <string>
#include <vector>

namespace networking_chat
{

class ChatSession
{
  public:
    auto submit_message(std::string message) -> void;
    auto queue_user_message(std::string message) -> void;

    [[nodiscard]] auto transcript_snapshot() const -> std::vector<std::string>;
    [[nodiscard]] auto consume_pending_user_messages() -> std::vector<std::string>;

  private:
    mutable std::mutex mutex_{};
    std::vector<std::string> transcript_{};
    std::vector<std::string> pending_user_messages_{};
};

}  // namespace networking_chat
