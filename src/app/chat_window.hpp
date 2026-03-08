#pragma once

#include <array>
#include <cstddef>

namespace networking_chat
{

class ChatSession;

struct ChatWindowState
{
    std::array<char, 1024> input_buffer{};
    std::size_t last_rendered_message_count{0u};
    bool focus_input_on_next_frame{true};
};

auto render_chat_window(ChatSession& session, ChatWindowState& state) -> void;

}  // namespace networking_chat
