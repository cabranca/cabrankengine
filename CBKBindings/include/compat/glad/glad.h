// Compatibility shim: under emscripten/WebGL2, redirect <glad/glad.h> to <GLES3/gl3.h>.
// The full DSA function set glad provides on desktop is unavailable in GLES 3.0;
// CBK_OPENGL_ES guards in the platform layer cover the call-site differences.
#pragma once

#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
#error "This compat shim is only intended for emscripten builds"
#endif
