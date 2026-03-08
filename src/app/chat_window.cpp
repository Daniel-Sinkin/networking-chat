#include "chat_window.hpp"

#include "chat_session.hpp"

#include <imgui.h>

#include <string>
#include <string_view>

namespace networking_chat
{
namespace
{

[[nodiscard]] auto trim_message(std::string_view value) -> std::string
{
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos)
    {
        return {};
    }

    const auto last = value.find_last_not_of(" \t\r\n");
    return std::string{value.substr(first, last - first + 1u)};
}

auto submit_input_if_ready(ChatSession& session, ChatWindowState& state) -> void
{
    auto message = trim_message(state.input_buffer.data());
    if (message.empty())
    {
        return;
    }

    session.queue_user_message(std::move(message));
    state.input_buffer.fill('\0');
    state.focus_input_on_next_frame = true;
}

}  // namespace

auto render_chat_window(ChatSession& session, ChatWindowState& state) -> void
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    constexpr auto window_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove |
                                  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar;

    if (!ImGui::Begin("Networking Chat", nullptr, window_flags))
    {
        ImGui::End();
        return;
    }

    ImGui::TextUnformatted("Networking Chat");
    ImGui::Separator();

    const auto messages = session.transcript_snapshot();
    const bool has_new_messages = messages.size() > state.last_rendered_message_count;
    const float footer_height = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y;

    if (ImGui::BeginChild(
            "chat_transcript", ImVec2(0.0F, -footer_height), true, ImGuiWindowFlags_None
        ))
    {
        if (messages.empty())
        {
            ImGui::TextDisabled("No messages yet.");
        }
        else
        {
            for (const auto& message : messages)
            {
                ImGui::TextWrapped("%s", message.c_str());
                ImGui::Spacing();
            }

            if (has_new_messages)
            {
                ImGui::SetScrollHereY(1.0F);
            }
        }
    }
    ImGui::EndChild();
    state.last_rendered_message_count = messages.size();

    if (state.focus_input_on_next_frame)
    {
        ImGui::SetKeyboardFocusHere();
        state.focus_input_on_next_frame = false;
    }

    const float button_width = 84.0F;
    ImGui::PushItemWidth(-button_width - ImGui::GetStyle().ItemSpacing.x);
    const bool submitted_with_enter = ImGui::InputTextWithHint(
        "##chat_input",
        "Type a message...",
        state.input_buffer.data(),
        state.input_buffer.size(),
        ImGuiInputTextFlags_EnterReturnsTrue
    );
    ImGui::PopItemWidth();

    const auto current_input = trim_message(state.input_buffer.data());
    const bool can_send = !current_input.empty();

    ImGui::SameLine();
    if (!can_send)
    {
        ImGui::BeginDisabled();
    }
    const bool clicked_send = ImGui::Button("Send", ImVec2(button_width, 0.0F));
    if (!can_send)
    {
        ImGui::EndDisabled();
    }

    if ((submitted_with_enter || clicked_send) && can_send)
    {
        submit_input_if_ready(session, state);
    }

    ImGui::End();
}

}  // namespace networking_chat
