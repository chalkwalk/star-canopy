#pragma once

#include <glad/gl.h>

#include <initializer_list>
#include <string>

namespace starcanopy {

// A GLSL 1.50 program built from embedded shader files, the way the labs built
// them: each file holds both stages, the vertex shader under INCLUDE_VS and the
// fragment shader under INCLUDE_FS, and shared code such as field.glsl comes
// first. Both stages see every file, after a header of "#version 150" and the
// stage's define, passed to GL as separate strings with nothing between them.
class Program {
public:
  Program() = default;
  ~Program();
  Program(const Program&) = delete;
  Program& operator=(const Program&) = delete;

  // The fragment outputs are bound to colour attachments 0, 1, ... in order,
  // before linking: GLSL 1.50 has no layout(location) for them. False, with the
  // compiler's or linker's log, on failure.
  bool build(std::initializer_list<const char*> files, std::initializer_list<const char*> outputs,
             std::string& error);

  GLuint id() const { return id_; }
  GLint uniform(const char* name) const { return glGetUniformLocation(id_, name); }

private:
  GLuint id_ = 0;
};

}  // namespace starcanopy
