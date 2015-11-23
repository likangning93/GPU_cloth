#pragma once

#include <cstdio>
#include <cstring>

#define FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define checkGLError(msg) _checkGLErrorHelper(msg, FILENAME, __LINE__)

static
void _checkGLErrorHelper(const char *msg, const char *filename, int line) {
#if !defined(NDEBUG)
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error " << error << ": ";
        const char *e =
            error == GL_INVALID_OPERATION             ? "GL_INVALID_OPERATION" :
            error == GL_INVALID_ENUM                  ? "GL_INVALID_ENUM" :
            error == GL_INVALID_VALUE                 ? "GL_INVALID_VALUE" :
            error == GL_INVALID_INDEX                 ? "GL_INVALID_INDEX" :
            error == GL_INVALID_OPERATION             ? "GL_INVALID_OPERATION" :
            NULL;
        if (e) {
            fprintf(stderr, "%s\n", e);
        } else {
            fprintf(stderr, "%d\n", error);
        }
        exit(EXIT_FAILURE);
    }
#endif
}
