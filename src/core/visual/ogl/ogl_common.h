#pragma once
#ifdef WIN32 
#if defined(_M_X64)
#define GLEW_STATIC
#endif
#include "GL/glew.h"
#else
#ifndef GL_UNPACK_ROW_LENGTH
#define GL_UNPACK_ROW_LENGTH 0x0CF2
#endif
#ifdef __APPLE__
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#else
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#endif
#endif

bool TVPCheckGLExtension(const std::string &extname);
