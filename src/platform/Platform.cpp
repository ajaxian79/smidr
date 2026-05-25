#include "Platform.h"

#include <GLFW/glfw3.h>

#if defined(SMIDR_HAS_X11)
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#endif

namespace smidr::platform {

void force_undecorated_window([[maybe_unused]] GLFWwindow* window) {
#if defined(SMIDR_HAS_X11)
    if (glfwGetPlatform() != GLFW_PLATFORM_X11) return;

    Display* display = glfwGetX11Display();
    const Window xwindow = glfwGetX11Window(window);
    if (!display || xwindow == 0) return;

    struct MotifWmHints {
        unsigned long flags;
        unsigned long functions;
        unsigned long decorations;
        long input_mode;
        unsigned long status;
    };

    constexpr unsigned long kDecorationsFlag = 1UL << 1;
    MotifWmHints hints{};
    hints.flags = kDecorationsFlag;
    hints.decorations = 0;

    const Atom property = XInternAtom(display, "_MOTIF_WM_HINTS", False);
    XChangeProperty(display, xwindow, property, property, 32, PropModeReplace,
                    reinterpret_cast<const unsigned char*>(&hints), 5);
    XFlush(display);
#endif
}

}  // namespace smidr::platform
