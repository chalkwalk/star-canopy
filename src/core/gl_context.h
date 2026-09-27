#pragma once

#include <memory>
#include <string>

namespace starcanopy {

enum class ContextKind {
  Auto,          // EGL where available, else a hidden window
  Egl,           // EGL, surfaceless or on a device: needs no display
  HiddenWindow,  // an SDL window that is never shown: needs a display
};

const char* contextKindName(ContextKind kind);
bool parseContextKind(const std::string& text, ContextKind& kind);

// The core owns a context but no window (DESIGN.md §2). The context is current
// on the creating thread for its whole life, and GL functions are loaded.
class GlContext {
public:
  virtual ~GlContext() = default;

  // Null on failure, with the reason (every attempt's, for Auto) in `error`.
  static std::unique_ptr<GlContext> create(ContextKind want, std::string& error);

  // The kind actually made, which for Auto says which path was taken.
  virtual ContextKind kind() const = 0;

  // "renderer, GL version", for logs and bug reports.
  std::string description() const;
};

}  // namespace starcanopy
