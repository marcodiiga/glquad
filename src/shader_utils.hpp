#ifndef HEADER_SHADERUTILS_HPP
#define HEADER_SHADERUTILS_HPP

#include <GL/glew.h>
#include <algorithm>
#include <string>
#include <fstream>
#include <sstream>
#include <memory>

// Caveat: make sure to have a valid GL context before invoking these classes

class Shader {
public:

  enum ShaderType { VERTEX_SHADER, FRAGMENT_SHADER, GEOMETRY_SHADER };

  Shader(const GLenum& type) {
    switch (type) {
      case GL_VERTEX_SHADER:
        shader_type = VERTEX_SHADER;
        break;
      case GL_FRAGMENT_SHADER:
        shader_type = VERTEX_SHADER;
        break;
      default:
        shader_type = GEOMETRY_SHADER;
    }
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
      throw std::runtime_error(ss.str());
    }
    compiled = true;
  }

  GLuint getId() const {
    return id;
  }

  ShaderType getType() const {
    return shader_type;
  }

  bool isCompiled() const {
    return compiled;
  }

private:
  ShaderType shader_type;
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
    if (shader.getType() == Shader::ShaderType::VERTEX_SHADER)
      vertex_shader_present = true;
    shaders.emplace_back(std::move(shader));    
  }

  void linkProgram() {
    // Both a vertex and a fragment shader must be present
    if (!(vertex_shader_present == true && fragment_shader_present == true)) {

      std::for_each(shaders.begin(), shaders.end(), [](auto shader) {
        if (shader.isCompiled() == false)
          shader.compile();
      });

      glLinkProgram(id);

      GLint link_status;
      glGetProgramiv(id, GL_LINK_STATUS, &link_status);
      if (link_status == GL_FALSE)
        throw std::runtime_error("Shaders linking failed");
    }
    else
      throw std::runtime_error("At least a vertex and a fragment shaders are \
                                needed to perform linking");
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