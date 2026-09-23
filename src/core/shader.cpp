#include "shader.h"

#include "shaders.h"

#include <vector>

namespace starcanopy {

namespace {

GLuint compile(GLenum type, const char* header, std::initializer_list<const char*> files,
               std::string& error) {
  std::vector<const char*> sources{header};
  for (const char* name : files) {
    const char* source = shaderSource(name);
    if (!source) {
      error = std::string("no embedded shader ") + name;
      return 0;
    }
    sources.push_back(source);
  }
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, static_cast<GLsizei>(sources.size()), sources.data(), nullptr);
  glCompileShader(shader);
  GLint ok = GL_FALSE;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    std::vector<char> log(length > 0 ? length : 1, '\0');
    glGetShaderInfoLog(shader, static_cast<GLsizei>(log.size()), nullptr, log.data());
    error = std::string(*(files.end() - 1)) +
            (type == GL_VERTEX_SHADER ? " (vertex): " : " (fragment): ") + log.data();
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

bool Program::build(std::initializer_list<const char*> files,
                    std::initializer_list<const char*> outputs, std::string& error) {
  GLuint vertex = compile(GL_VERTEX_SHADER, "#version 150\n#define INCLUDE_VS 1\n", files, error);
  if (!vertex) {
    return false;
  }
  GLuint fragment =
      compile(GL_FRAGMENT_SHADER, "#version 150\n#define INCLUDE_FS 1\n", files, error);
  if (!fragment) {
    glDeleteShader(vertex);
    return false;
  }
  id_ = glCreateProgram();
  glAttachShader(id_, vertex);
  glAttachShader(id_, fragment);
  GLuint location = 0;
  for (const char* name : outputs) {
    glBindFragDataLocation(id_, location++, name);
  }
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
    error = std::string(*(files.end() - 1)) + " (link): " + log.data();
    glDeleteProgram(id_);
    id_ = 0;
    return false;
  }
  return true;
}

}  // namespace starcanopy
