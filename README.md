# Networking Chat

A minimal Dear ImGui chat shell for wiring a GUI to your own networking layer. The project now focuses on a single chat transcript view, a text input, and a small API surface for injecting messages into the UI.

## What is included

- A single-window chat UI with a scrollable transcript
- A message input row with `Send` button support
- Enter-to-send while the input field is focused and non-empty
- A thread-safe `ChatSession` API for pushing messages into the transcript
- A queue of pending user-authored messages that your networking code can consume later

## Core API

```cpp
#include "chat_session.hpp"

networking_chat::ChatSession session;
session.submit_message("Remote peer connected");
```

If you want to forward what the local user typed into your own server/client code, poll:

```cpp
auto outgoing = session.consume_pending_user_messages();
```

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/networking_chat
```

## Dependencies

- C++20 compiler
- GLFW (fetched automatically by CMake)
- Dear ImGui docking branch (fetched automatically by CMake)
