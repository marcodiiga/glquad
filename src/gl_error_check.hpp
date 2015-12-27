#ifndef HEADER_GLERRORCHECK_HPP
#define HEADER_GLERRORCHECK_HPP

#include <GLXW/glxw.h>
#include <iostream>
#include <string>

namespace {
  std::string glErrorString(GLenum error_code) {
    switch (error_code) {
      case GL_NO_ERROR:
        return "No error";
      case GL_INVALID_ENUM:
        return "Invalid enum";
      case GL_INVALID_VALUE:
        return "Invalid value";
      case GL_INVALID_OPERATION:
        return "Invalid operation";
      case GL_STACK_OVERFLOW:
        return "Stack overflow";
      case GL_STACK_UNDERFLOW:
        return "Stack underflow";
      case GL_OUT_OF_MEMORY:
        return "Out of memory";
      default:
        return std::string();
    }
  }
}



#define GL_ERROR_CHECK(x) do {                                                \
  x;                                                                          \
  GLenum res = glGetError();                                                  \
  if (res != GL_NO_ERROR)                                                     \
    std::cerr << "[GLERROR] " << __FILE__ << ":" << __LINE__ <<               \
              " - " << glErrorString(res) << std::endl;                       \
} while (0)                                  


#endif // HEADER_GLERRORCHECK_HPP
