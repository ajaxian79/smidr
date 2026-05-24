cmake_minimum_required(VERSION 3.20)
project(smidr
    VERSION     0.1.0
    LANGUAGES   CXX
    DESCRIPTION "Smidr — modern CSG CAD built with ImGui and Skald")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

# ── GLFW ─────────────────────────────────────────────────────────
find_package(glfw3 QUIET)
if(NOT glfw3_FOUND AND NOT TARGET glfw)
    include(FetchContent)
    set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        3.4
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(glfw)
endif()

# ── Skald (brings ImGui with it) ─────────────────────────────────
# Skald is private; use SSH alias for FetchContent, or a local path
# override via -DSKALD_SOURCE_DIR=...
include(FetchContent)
set(SKALD_BUILD_TEMPLATE OFF CACHE BOOL "" FORCE)
if(DEFINED SKALD_SOURCE_DIR)
    add_subdirectory(${SKALD_SOURCE_DIR} ${CMAKE_BINARY_DIR}/skald)
else()
    FetchContent_Declare(skald
        GIT_REPOSITORY git@github-ajaxian79:ajaxian79/skald.git
        GIT_TAG        main
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(skald)
endif()

# ── ImGui backends ───────────────────────────────────────────────
# imgui_SOURCE_DIR may not be set when skald comes via add_subdirectory;
# fall back to the FetchContent _deps cache.
if(NOT imgui_SOURCE_DIR)
    set(imgui_SOURCE_DIR "${CMAKE_BINARY_DIR}/_deps/imgui-src")
endif()
if(NOT TARGET imgui_backends)
    add_library(imgui_backends STATIC
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
    target_include_directories(imgui_backends SYSTEM PUBLIC
        ${imgui_SOURCE_DIR}/backends)
    target_link_libraries(imgui_backends PUBLIC imgui glfw OpenGL::GL)
endif()

# ── stb_image (vendored single-header) ───────────────────────────
add_library(smidr_stb STATIC src/stb_impl.cpp)
target_include_directories(smidr_stb PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# ── nlohmann/json (header-only, for .smidr document format) ──────
FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(json)

# ── Smidr core library ──────────────────────────────────────────
add_library(smidr_core STATIC
    src/core/Primitive.cpp
    src/core/Scene.cpp
    src/core/Document.cpp
    src/core/MeshGenerator.cpp)
target_include_directories(smidr_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(smidr_core PUBLIC nlohmann_json::nlohmann_json)

# ── Smidr render library ────────────────────────────────────────
add_library(smidr_render STATIC
    src/render/Shader.cpp
    src/render/Camera.cpp
    src/render/Grid.cpp
    src/render/Renderer.cpp)
target_include_directories(smidr_render PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(smidr_render PUBLIC smidr_core OpenGL::GL GLEW::GLEW)

# ── Smidr application ───────────────────────────────────────────
add_executable(smidr
    src/main.cpp
    src/app/App.cpp
    src/app/Viewport.cpp
    src/app/SceneTreePanel.cpp
    src/app/PropertiesPanel.cpp
    src/app/Toolbar.cpp)
target_link_libraries(smidr PRIVATE
    smidr_core
    smidr_render
    smidr_stb
    skald::skald
    imgui_backends
    glfw
    OpenGL::GL
    GLEW::GLEW)
target_compile_definitions(smidr PRIVATE
    SMIDR_FONT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fonts")
target_compile_options(smidr PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>)

# ── Tests ────────────────────────────────────────────────────────
option(SMIDR_BUILD_TESTS "Build tests" ON)
if(SMIDR_BUILD_TESTS)
    enable_testing()
    add_executable(smidr_tests
        tests/test_primitives.cpp
        tests/test_csg.cpp
        tests/test_document.cpp
        tests/test_main.cpp)
    target_link_libraries(smidr_tests PRIVATE smidr_core)
    add_test(NAME smidr_tests COMMAND smidr_tests)
endif()
