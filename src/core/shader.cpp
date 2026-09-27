#include "shader.h"

#include "shaders.h"

#include <vector>

namespace starcanopy {

namespace {

GLuint compile(GLenum type, const char* name, std::string& error) {
  const char* source = shaderSource(name);
  if (!source) {
    error = std::string("no embedded shader ") + name;
    return 0;
  }
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, nullptr);
  glCompileShader(shader);
  GLint ok = GL_FALSE;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    std::vector<char> log(length > 0 ? length : 1, '\0');
    glGetShaderInfoLog(shader, static_cast<GLsizei>(log.size()), nullptr, log.data());
    error = std::string(name) + ": " + log.data();
    glDeleteShader(shader);
    return 0;
  }
  return shader;
}

}  // namespace

Program::~Program() {
  if (id_) {
    glDeleteProgram(id_);
  }
}

bool Program::build(const char* vertexName, const char* fragmentName, std::string& error) {
  GLuint vertex = compile(GL_VERTEX_SHADER, vertexName, error);
  if (!vertex) {
    return false;
  }
  GLuint fragment = compile(GL_FRAGMENT_SHADER, fragmentName, error);
  if (!fragment) {
    glDeleteShader(vertex);
    return false;
  }
  id_ = glCreateProgram();
  glAttachShader(id_, vertex);
  glAttachShader(id_, fragment);
  glLinkProgram(id_);
  glDeleteShader(vertex);
  glDeleteShader(fragment);
  GLint ok = GL_FALSE;
  glGetProgramiv(id_, GL_LINK_STATUS, &ok);
  if (!ok) {
    GLint length = 0;
    glGetProgramiv(id_, GL_INFO_LOG_LENGTH, &length);
    std::vector<char> log(length > 0 ? length : 1, '\0');
    glGetProgramInfoLog(id_, static_cast<GLsizei>(log.size()), nullptr, log.data());
    error = std::string(vertexName) + " + " + fragmentName + ": " + log.data();
    glDeleteProgram(id_);
    id_ = 0;
    return false;
  }
  return true;
}

}  // namespace starcanopy
