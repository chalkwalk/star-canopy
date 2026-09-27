#pragma once

#include <glad/gl.h>

#include <string>

namespace starcanopy {

class Program {
public:
  Program() = default;
  ~Program();
  Program(const Program&) = delete;
  Program& operator=(const Program&) = delete;

  // Builds from the embedded shaders named, e.g. "fullscreen.vert". On
  // failure, returns false with the compiler's or linker's log in `error`.
  bool build(const char* vertexName, const char* fragmentName, std::string& error);

  GLuint id() const { return id_; }
  GLint uniform(const char* name) const { return glGetUniformLocation(id_, name); }

private:
  GLuint id_ = 0;
};

}  // namespace starcanopy
