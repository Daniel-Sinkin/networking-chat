cmake_minimum_required(VERSION 3.28)
project(networking_chat LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "Disable online FetchContent updates" FORCE)

set(COMMON_WARNINGS
  -Wall
  -Wextra
  -Wpedantic
  -Werror
  -Wshadow
  -Wconversion
  -Wfloat-conversion
  -Wsign-conversion
  -Wcast-qual
  -Wcast-align
  -Wformat=2
  -Wundef
  -Wnull-dereference
  -Wnon-virtual-dtor
  -Woverloaded-virtual
  -Wold-style-cast
  -Wzero-as-null-pointer-constant
  -Wdouble-promotion
  -Wimplicit-fallthrough
  -Wreturn-type
)

include(FetchContent)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.3.9
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        docking
)
FetchContent_MakeAvailable(imgui)

add_library(imgui_lib STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui_lib PUBLIC
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui_lib PUBLIC glfw)

if(APPLE)
  target_link_libraries(imgui_lib PUBLIC "-framework OpenGL")
else()
  find_package(OpenGL REQUIRED)
  target_link_libraries(imgui_lib PUBLIC OpenGL::GL)
endif()

add_library(networking_chat_core STATIC
  src/chat/chat_session.cpp
)
target_include_directories(networking_chat_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/chat)
target_compile_options(networking_chat_core PRIVATE ${COMMON_WARNINGS})

add_library(networking_chat_gui STATIC
  src/app/chat_window.cpp
  src/app/imgui_theme.cpp
)
target_include_directories(networking_chat_gui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/app)
target_link_libraries(networking_chat_gui PUBLIC imgui_lib networking_chat_core)
target_compile_options(networking_chat_gui PRIVATE ${COMMON_WARNINGS})
target_compile_definitions(networking_chat_gui PUBLIC
  NETWORKING_CHAT_PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}"
)

add_executable(networking_chat
  src/app/main.cpp
)
target_link_libraries(networking_chat PRIVATE networking_chat_gui)
target_compile_options(networking_chat PRIVATE ${COMMON_WARNINGS})

if(APPLE)
  target_compile_definitions(imgui_lib PRIVATE GL_SILENCE_DEPRECATION)
  target_compile_definitions(networking_chat_gui PRIVATE GL_SILENCE_DEPRECATION)
  target_compile_definitions(networking_chat PRIVATE GL_SILENCE_DEPRECATION)
endif()
