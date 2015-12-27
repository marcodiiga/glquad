#ifndef HEADER_SHADERUTILS_HPP
#define HEADER_SHADERUTILS_HPP

#include <GLXW/glxw.h>
#include <iostream>
#include <algorithm>
#include <string>
#include <fstream>
#include <sstream>
#include <memory>

// Caveat: make sure to have a valid GL context before invoking these classes

class Shader {
public:

  Shader(GLenum type) : shader_type(type) {
    id = glCreateShader(type);
  }

  void loadFromFile(const std::string& shader_filename) {
    compiled = false;
    std::ifstream file;
    file.open(shader_filename.c_str());
    if (!file.good())
      throw std::runtime_error("Cannot open file");

    std::stringstream ss;
    ss << file.rdbuf();
    file.close();
    source = ss.str();

    auto ptr = source.c_str();
    glShaderSource(id, 1, &ptr, NULL);
  }

  void loadFromString(const std::string& shader_source) {
    compiled = false;
    source = shader_source;
    auto ptr = source.c_str();
    glShaderSource(id, 1, &ptr, NULL);
  }

  void compile() {
    glCompileShader(id);

    GLint compile_status;
    glGetShaderiv(id, GL_COMPILE_STATUS, &compile_status);
    
    if (compile_status == GL_FALSE) {
      GLint log_length;
      glGetShaderiv(id, GL_INFO_LOG_LENGTH, &log_length);

      std::unique_ptr<GLchar> info_log = std::make_unique<GLchar>(log_length + 1);
      glGetShaderInfoLog(id, log_length, NULL, info_log.get());

      std::stringstream ss;
      ss << "Shader " << id << " failed compiling: " << info_log.get() << std::endl;
      std::cerr << ss.str();

      std::exit(1);
    }
    compiled = true;
  }

  GLuint getId() const {
    return id;
  }

  GLenum getType() const {
    return shader_type;
  }

  bool isCompiled() const {
    return compiled;
  }

private:
  GLenum shader_type;
  GLuint id; // Shader id
  std::string source; // Shader source code
  bool compiled = false;
};

class ShaderProgram {
public:
  ShaderProgram() {
    id = glCreateProgram(); // Again: MAKE SURE a valid GL context is available
  }

  ~ShaderProgram() {
    if (shaders_detached == false)
      detachAllShaders();
    for (auto& shader : shaders)
      glDeleteShader(shader.getId());
    glDeleteProgram(id);
  }

  void addShader(Shader&& shader) {
    glAttachShader(id, shader.getId());
    GLenum res = glGetError();
    if (res != GL_NO_ERROR) {
      std::cerr << "Attaching shader failed with error " << res << std::endl;
      std::exit(1);
    }
    if (shader.getType() == GL_VERTEX_SHADER)
      vertex_shader_present = true;
    else if (shader.getType() == GL_FRAGMENT_SHADER)
      fragment_shader_present = true;
    shaders.emplace_back(std::move(shader));    
  }

  void linkProgram() {
    // Both a vertex and a fragment shader must be present
    if (vertex_shader_present == true && fragment_shader_present == true) {

      std::for_each(shaders.begin(), shaders.end(), [](auto shader) {
        if (shader.isCompiled() == false)
          shader.compile();
      });

      glLinkProgram(id);

      GLint link_status;
      glGetProgramiv(id, GL_LINK_STATUS, &link_status);

      if (link_status == GL_FALSE) {
        // Debug a failed linking
        std::string info_log(200, 0);
        GLsizei out_len;
        glGetProgramInfoLog(this->getId(), 200, &out_len, (GLchar*)info_log.c_str());

        std::stringstream ss;
        ss << "Shaders linking failed, info log returned: " << info_log;
        std::cerr << ss.str();

        std::exit(1);
      }
    } else {
      std::cerr << "At least a vertex and a fragment shaders are needed to " \
        "perform linking" << std::endl;
      std::exit(1);
    }
  }

  void detachAllShaders() {
    for (auto& shader : shaders)
      glDetachShader(id, shader.getId());
    shaders_detached = true;
  }

  GLuint getId() const {
    return id;
  }

  void validateProgram() {
    glValidateProgram(id);
    GLint ret_status;
    glGetProgramiv(id, GL_VALIDATE_STATUS, &ret_status);
    if (ret_status == GL_FALSE)
      throw std::runtime_error("Validation failed");
  }

private:
  GLuint id;
  std::vector<Shader> shaders;
  bool vertex_shader_present = false;
  bool fragment_shader_present = false;
  bool shaders_detached = false;
};

#endif // HEADER_SHADERUTILS_HPP
