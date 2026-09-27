#include "gl_context.h"

#include <glad/gl.h>
#include <SDL3/SDL.h>

#include <cstring>

#ifdef STARCANOPY_HAVE_EGL
#include <EGL/egl.h>
#include <EGL/eglext.h>
#endif

namespace starcanopy {

namespace {

bool loadedVersionIsEnough(int version, std::string& error) {
  if (version == 0) {
    error = "could not load OpenGL functions";
    return false;
  }
  if (GLAD_VERSION_MAJOR(version) * 10 + GLAD_VERSION_MINOR(version) < 33) {
    error = "OpenGL " + std::to_string(GLAD_VERSION_MAJOR(version)) + "." +
            std::to_string(GLAD_VERSION_MINOR(version)) + " found; 3.3 core is needed";
    return false;
  }
  return true;
}

#ifdef STARCANOPY_HAVE_EGL

bool hasExtension(const char* list, const char* name) {
  if (!list) {
    return false;
  }
  size_t n = std::strlen(name);
  for (const char* p = list; (p = std::strstr(p, name)) != nullptr; p += n) {
    bool starts = p == list || p[-1] == ' ';
    bool ends = p[n] == '\0' || p[n] == ' ';
    if (starts && ends) {
      return true;
    }
  }
  return false;
}

class EglContext : public GlContext {
public:
  ~EglContext() override {
    if (display_ != EGL_NO_DISPLAY) {
      eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
      if (context_ != EGL_NO_CONTEXT) {
        eglDestroyContext(display_, context_);
      }
      eglTerminate(display_);
    }
  }

  ContextKind kind() const override { return ContextKind::Egl; }

  bool init(std::string& error) {
    display_ = openDisplay(error);
    if (display_ == EGL_NO_DISPLAY) {
      return false;
    }
    // Rendering only ever goes to our own framebuffers, so no surface is made.
    if (!hasExtension(eglQueryString(display_, EGL_EXTENSIONS), "EGL_KHR_surfaceless_context")) {
      error = "EGL display lacks EGL_KHR_surfaceless_context";
      return false;
    }
    if (!eglBindAPI(EGL_OPENGL_API)) {
      error = "EGL cannot bind desktop OpenGL";
      return false;
    }
    const EGLint configAttribs[] = {
      EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
      EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
      EGL_NONE,
    };
    EGLConfig config;
    EGLint count = 0;
    if (!eglChooseConfig(display_, configAttribs, &config, 1, &count) || count == 0) {
      error = "no EGL config for desktop OpenGL";
      return false;
    }
    const EGLint contextAttribs[] = {
      EGL_CONTEXT_MAJOR_VERSION, 3,
      EGL_CONTEXT_MINOR_VERSION, 3,
      EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
      EGL_NONE,
    };
    context_ = eglCreateContext(display_, config, EGL_NO_CONTEXT, contextAttribs);
    if (context_ == EGL_NO_CONTEXT) {
      error = "eglCreateContext failed for OpenGL 3.3 core";
      return false;
    }
    if (!eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, context_)) {
      error = "eglMakeCurrent failed";
      return false;
    }
    return loadedVersionIsEnough(gladLoadGL(reinterpret_cast<GLADloadfunc>(eglGetProcAddress)), error);
  }

private:
  // An initialised display: Mesa's surfaceless platform first, then each EGL
  // device in turn, which is how NVIDIA's driver offers a context with no
  // display server. Every device is tried, not just the first, because a GPU
  // the user may not open (no access to /dev/dri) is listed ahead of Mesa's
  // software device.
  static EGLDisplay openDisplay(std::string& error) {
    const char* client = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    auto getPlatformDisplay = reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(
        eglGetProcAddress("eglGetPlatformDisplayEXT"));
    if (!getPlatformDisplay) {
      error = "EGL lacks eglGetPlatformDisplayEXT";
      return EGL_NO_DISPLAY;
    }
    auto initialised = [](EGLDisplay d) {
      if (d == EGL_NO_DISPLAY) {
        return false;
      }
      if (!eglInitialize(d, nullptr, nullptr)) {
        eglTerminate(d);
        return false;
      }
      return true;
    };
    if (hasExtension(client, "EGL_MESA_platform_surfaceless")) {
      EGLDisplay d = getPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, nullptr);
      if (initialised(d)) {
        return d;
      }
    }
    auto queryDevices = reinterpret_cast<PFNEGLQUERYDEVICESEXTPROC>(
        eglGetProcAddress("eglQueryDevicesEXT"));
    if (hasExtension(client, "EGL_EXT_platform_device") && queryDevices) {
      EGLDeviceEXT devices[16];
      EGLint count = 0;
      if (queryDevices(16, devices, &count)) {
        for (EGLint i = 0; i < count; i++) {
          EGLDisplay d = getPlatformDisplay(EGL_PLATFORM_DEVICE_EXT, devices[i], nullptr);
          if (initialised(d)) {
            return d;
          }
        }
      }
    }
    error = "no surfaceless or device EGL display would initialise";
    return EGL_NO_DISPLAY;
  }

  EGLDisplay display_ = EGL_NO_DISPLAY;
  EGLContext context_ = EGL_NO_CONTEXT;
};

#endif  // STARCANOPY_HAVE_EGL

class HiddenWindowContext : public GlContext {
public:
  ~HiddenWindowContext() override {
    if (context_) {
      SDL_GL_DestroyContext(context_);
    }
    if (window_) {
      SDL_DestroyWindow(window_);
    }
    if (videoInitialised_) {
      SDL_QuitSubSystem(SDL_INIT_VIDEO);
    }
  }

  ContextKind kind() const override { return ContextKind::HiddenWindow; }

  bool init(std::string& error) {
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
      error = std::string("SDL video: ") + SDL_GetError();
      return false;
    }
    videoInitialised_ = true;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    // macOS offers 3.2+ core only to forward-compatible contexts.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    window_ = SDL_CreateWindow("StarCanopy", 16, 16, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (!window_) {
      error = std::string("SDL window: ") + SDL_GetError();
      return false;
    }
    context_ = SDL_GL_CreateContext(window_);
    if (!context_) {
      error = std::string("SDL GL context: ") + SDL_GetError();
      return false;
    }
    if (!SDL_GL_MakeCurrent(window_, context_)) {
      error = std::string("SDL make current: ") + SDL_GetError();
      return false;
    }
    return loadedVersionIsEnough(gladLoadGL(reinterpret_cast<GLADloadfunc>(SDL_GL_GetProcAddress)), error);
  }

private:
  bool videoInitialised_ = false;
  SDL_Window* window_ = nullptr;
  SDL_GLContext context_ = nullptr;
};

template <typename T>
std::unique_ptr<GlContext> tryCreate(std::string& error) {
  auto context = std::make_unique<T>();
  if (!context->init(error)) {
    return nullptr;
  }
  return context;
}

}  // namespace

const char* contextKindName(ContextKind kind) {
  switch (kind) {
    case ContextKind::Auto: return "auto";
    case ContextKind::Egl: return "egl";
    case ContextKind::HiddenWindow: return "window";
  }
  return "?";
}

bool parseContextKind(const std::string& text, ContextKind& kind) {
  for (ContextKind k : {ContextKind::Auto, ContextKind::Egl, ContextKind::HiddenWindow}) {
    if (text == contextKindName(k)) {
      kind = k;
      return true;
    }
  }
  return false;
}

std::unique_ptr<GlContext> GlContext::create(ContextKind want, std::string& error) {
  std::string eglError = "EGL: not built in";
  std::unique_ptr<GlContext> context;
  if (want == ContextKind::Egl || want == ContextKind::Auto) {
#ifdef STARCANOPY_HAVE_EGL
    eglError.clear();
    context = tryCreate<EglContext>(eglError);
    if (!eglError.empty()) {
      eglError = "EGL: " + eglError;
    }
#endif
    if (context || want == ContextKind::Egl) {
      error = eglError;
      return context;
    }
  }
  std::string windowError;
  context = tryCreate<HiddenWindowContext>(windowError);
  error = want == ContextKind::Auto ? eglError + "; hidden window: " + windowError : windowError;
  return context;
}

std::string GlContext::description() const {
  auto str = [](GLenum name) {
    const GLubyte* s = glGetString(name);
    return s ? std::string(reinterpret_cast<const char*>(s)) : std::string("?");
  };
  return str(GL_RENDERER) + ", OpenGL " + str(GL_VERSION) + " (" + contextKindName(kind()) + ")";
}

}  // namespace starcanopy
