#pragma once

// Smidr GL loader — uses glcorearb.h with GL_GLEXT_PROTOTYPES on Linux.
// This provides all GL 3.2 core functions without a runtime loader.
// For cross-platform builds, replace this with generated GLAD.

#define GL_GLEXT_PROTOTYPES
#include <GL/glcorearb.h>

// gladLoadGLLoader is a no-op on Linux with GL_GLEXT_PROTOTYPES —
// functions are resolved at link time from libGL.so.
typedef void* (*GLADloadproc)(const char* name);

inline int gladLoadGLLoader(GLADloadproc) {
    return 1;
}
