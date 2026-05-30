cmake_minimum_required(VERSION 3.20)
project(smidr VERSION 0.1.0 LANGUAGES C CXX
    DESCRIPTION "Smidr — vertically integrated CSG CAD")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenGL REQUIRED)
find_package(X11 QUIET)

# ── Warning suppression for absorbed code ────────────────────
function(smidr_suppress_warnings target)
    target_compile_options(${target} PRIVATE
        $<$<COMPILE_LANG_AND_ID:C,GNU,Clang>:-w>
        $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:-w>)
endfunction()

# ── GLFW (absorbed, Linux/X11 only) ─────────────────────────
set(SMIDR_GLFW_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/glfw)
add_library(smidr_glfw_obj OBJECT
    ${SMIDR_GLFW_DIR}/src/init.c
    ${SMIDR_GLFW_DIR}/src/window.c
    ${SMIDR_GLFW_DIR}/src/monitor.c
    ${SMIDR_GLFW_DIR}/src/input.c
    ${SMIDR_GLFW_DIR}/src/context.c
    ${SMIDR_GLFW_DIR}/src/vulkan.c
    ${SMIDR_GLFW_DIR}/src/platform.c
    ${SMIDR_GLFW_DIR}/src/egl_context.c
    ${SMIDR_GLFW_DIR}/src/osmesa_context.c
    ${SMIDR_GLFW_DIR}/src/x11_init.c
    ${SMIDR_GLFW_DIR}/src/x11_monitor.c
    ${SMIDR_GLFW_DIR}/src/x11_window.c
    ${SMIDR_GLFW_DIR}/src/xkb_unicode.c
    ${SMIDR_GLFW_DIR}/src/glx_context.c
    ${SMIDR_GLFW_DIR}/src/posix_time.c
    ${SMIDR_GLFW_DIR}/src/posix_thread.c
    ${SMIDR_GLFW_DIR}/src/posix_module.c
    ${SMIDR_GLFW_DIR}/src/posix_poll.c
    ${SMIDR_GLFW_DIR}/src/null_init.c
    ${SMIDR_GLFW_DIR}/src/null_monitor.c
    ${SMIDR_GLFW_DIR}/src/null_window.c
    ${SMIDR_GLFW_DIR}/src/null_joystick.c
    ${SMIDR_GLFW_DIR}/src/linux_joystick.c)
target_compile_definitions(smidr_glfw_obj PRIVATE _GLFW_X11)
target_include_directories(smidr_glfw_obj PRIVATE
    ${SMIDR_GLFW_DIR}/include
    ${SMIDR_GLFW_DIR}/src)
smidr_suppress_warnings(smidr_glfw_obj)

# ── Platform static lib ──────────────────────────────────────
add_library(smidr_platform STATIC src/platform/Platform.cpp)
target_sources(smidr_platform PRIVATE $<TARGET_OBJECTS:smidr_glfw_obj>)
target_include_directories(smidr_platform PUBLIC
    ${SMIDR_GLFW_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(smidr_platform PUBLIC ${CMAKE_DL_LIBS})
if(X11_FOUND)
    target_include_directories(smidr_platform PRIVATE ${X11_INCLUDE_DIR})
    target_link_libraries(smidr_platform PUBLIC ${X11_LIBRARIES})
    target_compile_definitions(smidr_platform PUBLIC SMIDR_HAS_X11=1)
endif()

# ── GLAD (GL 3.2 core loader stub) ──────────────────────────
add_library(smidr_glad_obj OBJECT src/render/glad/glad.c)
target_include_directories(smidr_glad_obj PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/render)
smidr_suppress_warnings(smidr_glad_obj)

# ── ImGui (absorbed, OBJECT lib) ─────────────────────────────
set(SMIDR_IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/imgui)
add_library(smidr_imgui_obj OBJECT
    ${SMIDR_IMGUI_DIR}/imgui.cpp
    ${SMIDR_IMGUI_DIR}/imgui_draw.cpp
    ${SMIDR_IMGUI_DIR}/imgui_tables.cpp
    ${SMIDR_IMGUI_DIR}/imgui_widgets.cpp
    ${SMIDR_IMGUI_DIR}/imgui_impl_glfw.cpp
    ${SMIDR_IMGUI_DIR}/imgui_impl_opengl3.cpp)
target_include_directories(smidr_imgui_obj PUBLIC
    ${SMIDR_IMGUI_DIR}
    ${SMIDR_GLFW_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/render)
# ImGui uses its own built-in GL loader (imgui_impl_opengl3_loader.h).
# Smidr's render layer uses glad/glad.h (glcorearb.h) in separate TUs.
smidr_suppress_warnings(smidr_imgui_obj)

# ── GUI (absorbed Skald + ImGui objects) ─────────────────────
add_library(smidr_gui STATIC
    src/gui/Theme.cpp
    src/gui/Widgets.cpp
    src/gui/Fonts.cpp)
target_sources(smidr_gui PRIVATE $<TARGET_OBJECTS:smidr_imgui_obj>)
target_include_directories(smidr_gui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${SMIDR_IMGUI_DIR})
target_link_libraries(smidr_gui PUBLIC smidr_platform OpenGL::GL)

# ── JSON (header-only interface) ─────────────────────────────
add_library(smidr_json INTERFACE)
target_include_directories(smidr_json INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/io)

# ── Core (geometry kernel) ───────────────────────────────────
add_library(smidr_core STATIC
    src/core/Primitive.cpp
    src/core/Scene.cpp
    src/core/Document.cpp
    src/core/MeshGenerator.cpp
    src/core/RayCast.cpp
    src/core/CommandHistory.cpp
    src/core/Material.cpp
    src/core/CSGEval.cpp
    src/core/PrimitivesAdvanced.cpp
    src/core/Analysis.cpp
    src/core/NURBS.cpp
    src/core/NMG.cpp
    src/core/Constraints.cpp
    src/core/BREPIntersect.cpp
    src/core/PrimitivesExtra.cpp
    src/core/MeshOps.cpp
    src/core/Octree.cpp
    src/core/BREPBool.cpp
    src/core/SDF.cpp)
target_include_directories(smidr_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(smidr_core PUBLIC smidr_json)

# ── Render ───────────────────────────────────────────────────
add_library(smidr_render STATIC
    src/render/Shader.cpp
    src/render/Camera.cpp
    src/render/Grid.cpp
    src/render/Renderer.cpp
    src/render/RayTracer.cpp
    src/render/Shaders.cpp
    src/render/ShadersExtra.cpp)
target_sources(smidr_render PRIVATE $<TARGET_OBJECTS:smidr_glad_obj>)
target_include_directories(smidr_render PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/render)
target_link_libraries(smidr_render PUBLIC smidr_core OpenGL::GL)

# ── I/O ──────────────────────────────────────────────────────
add_library(smidr_io STATIC
    src/io/stb_impl.cpp
    src/io/MeshExport.cpp
    src/io/FormatImport.cpp
    src/io/FormatDXF.cpp
    src/io/FormatVRML.cpp
    src/io/FormatGLTF.cpp
    src/io/FormatIGES.cpp
    src/io/FormatPLYWrite.cpp
    src/io/FormatSTEP.cpp)
target_include_directories(smidr_io PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(smidr_io PUBLIC smidr_core)

# ── Executable ───────────────────────────────────────────────
add_executable(smidr
    src/main.cpp
    src/app/App.cpp
    src/app/Chrome.cpp
    src/app/TitleBar.cpp
    src/app/MenuBar.cpp
    src/app/StatusBar.cpp
    src/app/Splash.cpp
    src/app/Viewport.cpp
    src/app/SceneTreePanel.cpp
    src/app/PropertiesPanel.cpp
    src/app/Toolbar.cpp
    src/app/Console.cpp
    src/app/CommandRegistry.cpp
    src/app/Gizmo.cpp
    src/app/LightsAndAnim.cpp
    src/app/ScriptEngine.cpp)
target_link_libraries(smidr PRIVATE
    smidr_core smidr_render smidr_gui smidr_io smidr_platform
    OpenGL::GL ${CMAKE_DL_LIBS})
target_compile_definitions(smidr PRIVATE
    SMIDR_FONT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/src/fonts"
    SMIDR_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets")
target_compile_options(smidr PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>)

# ── Tests (no GL deps) ───────────────────────────────────────
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
