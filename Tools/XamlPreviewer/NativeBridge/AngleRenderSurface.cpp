#define NOMINMAX
#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include <ESRenderer/OpenGlRenderer.h>

#include "AngleRenderSurface.h"

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace xaml::bridge::_details {
    std::vector<unsigned char> ReadFile(std::string_view path) {
        std::ifstream stream(std::string(path), std::ios::binary | std::ios::ate);
        if (!stream) {
            throw std::runtime_error("ANGLE renderer could not open the font file");
        }
        const auto length = stream.tellg();
        if (length <= 0) {
            throw std::runtime_error("ANGLE renderer received an empty font file");
        }
        std::vector<unsigned char> data(static_cast<size_t>(length));
        stream.seekg(0, std::ios::beg);
        if (!stream.read(
            reinterpret_cast<char*>(data.data()),
            static_cast<std::streamsize>(length))) {
            throw std::runtime_error("ANGLE renderer could not read the font file");
        }
        return data;
    }
}

namespace xaml::bridge {
    class AngleRenderSurface::Implementation {
    public:
        Implementation(
            int width,
            int height,
            std::string_view fontPath,
            std::string_view resourceRoot);
        ~Implementation();

        Implementation(const Implementation&) = delete;
        Implementation& operator=(const Implementation&) = delete;

        void Render(
            Element& root,
            unsigned char* destination,
            int destinationStride);

    private:
        int width;
        int height;
        EGLDisplay display = EGL_NO_DISPLAY;
        EGLSurface surface = EGL_NO_SURFACE;
        EGLContext context = EGL_NO_CONTEXT;
        std::unique_ptr<es_renderer::OpenGlRenderer> renderer;
    };

    AngleRenderSurface::Implementation::Implementation(
        int width,
        int height,
        std::string_view fontPath,
        std::string_view resourceRoot)
        : width(width)
        , height(height) {
        if (width <= 0 || height <= 0) {
            throw std::invalid_argument("ANGLE surface dimensions must be positive");
        }
        this->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (this->display == EGL_NO_DISPLAY
            || eglInitialize(this->display, nullptr, nullptr) == EGL_FALSE
            || eglBindAPI(EGL_OPENGL_ES_API) == EGL_FALSE) {
            throw std::runtime_error("ANGLE could not initialize EGL");
        }
        const EGLint configurationAttributes[] = {
            EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_ALPHA_SIZE, 8,
            EGL_NONE,
        };
        EGLConfig configuration = nullptr;
        EGLint configurationCount = 0;
        if (eglChooseConfig(
            this->display,
            configurationAttributes,
            &configuration,
            1,
            &configurationCount) == EGL_FALSE
            || configurationCount == 0) {
            throw std::runtime_error("ANGLE could not choose a pbuffer configuration");
        }
        const EGLint surfaceAttributes[] = {
            EGL_WIDTH, this->width,
            EGL_HEIGHT, this->height,
            EGL_NONE,
        };
        this->surface = eglCreatePbufferSurface(this->display, configuration, surfaceAttributes);
        const EGLint contextAttributes[] = {
            EGL_CONTEXT_CLIENT_VERSION, 3,
            EGL_NONE,
        };
        this->context = eglCreateContext(
            this->display,
            configuration,
            EGL_NO_CONTEXT,
            contextAttributes);
        if (this->surface == EGL_NO_SURFACE
            || this->context == EGL_NO_CONTEXT
            || eglMakeCurrent(
                this->display,
                this->surface,
                this->surface,
                this->context) == EGL_FALSE) {
            throw std::runtime_error("ANGLE could not create an OpenGL ES 3 context");
        }
        const std::vector<unsigned char> fontData = _details::ReadFile(fontPath);
        this->renderer = std::make_unique<es_renderer::OpenGlRenderer>(
            width,
            height,
            fontData.data(),
            fontData.size(),
            [root = std::string(resourceRoot)](std::string_view source) {
                return _details::ReadFile(root + "/" + std::string(source));
            });
        if (eglMakeCurrent(
            this->display,
            EGL_NO_SURFACE,
            EGL_NO_SURFACE,
            EGL_NO_CONTEXT) == EGL_FALSE) {
            throw std::runtime_error("ANGLE could not release the offscreen context");
        }
    }

    AngleRenderSurface::Implementation::~Implementation() {
        this->renderer.reset();
        if (this->display != EGL_NO_DISPLAY) {
            eglMakeCurrent(this->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            if (this->context != EGL_NO_CONTEXT) {
                eglDestroyContext(this->display, this->context);
            }
            if (this->surface != EGL_NO_SURFACE) {
                eglDestroySurface(this->display, this->surface);
            }
            eglTerminate(this->display);
        }
    }

    void AngleRenderSurface::Implementation::Render(
        Element& root,
        unsigned char* destination,
        int destinationStride) {
        if (destination == nullptr || destinationStride < this->width * 4) {
            throw std::invalid_argument("Invalid ANGLE render buffer arguments");
        }
        if (eglMakeCurrent(
            this->display,
            this->surface,
            this->surface,
            this->context) == EGL_FALSE) {
            throw std::runtime_error("ANGLE could not activate the offscreen context");
        }
        this->renderer->BeginFrame();
        xaml::Render(root, *this->renderer);
        glFinish();

        std::vector<unsigned char> pixels(
            static_cast<size_t>(this->width) * static_cast<size_t>(this->height) * 4);
        glPixelStorei(GL_PACK_ALIGNMENT, 4);
        glReadPixels(
            0,
            0,
            this->width,
            this->height,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            pixels.data());
        for (int y = 0; y < this->height; ++y) {
            const unsigned char* source = pixels.data()
                + static_cast<size_t>(this->height - y - 1) * this->width * 4;
            unsigned char* row = destination + static_cast<size_t>(y) * destinationStride;
            for (int x = 0; x < this->width; ++x) {
                row[x * 4] = source[x * 4 + 2];
                row[x * 4 + 1] = source[x * 4 + 1];
                row[x * 4 + 2] = source[x * 4];
                row[x * 4 + 3] = source[x * 4 + 3];
            }
        }
        if (eglMakeCurrent(
            this->display,
            EGL_NO_SURFACE,
            EGL_NO_SURFACE,
            EGL_NO_CONTEXT) == EGL_FALSE) {
            throw std::runtime_error("ANGLE could not release the offscreen context");
        }
    }

    AngleRenderSurface::AngleRenderSurface(
        int width,
        int height,
        std::string_view fontPath,
        std::string_view resourceRoot)
        : implementation(std::make_unique<Implementation>(width, height, fontPath, resourceRoot)) {
    }

    AngleRenderSurface::~AngleRenderSurface() = default;

    //
    // API
    //
    void AngleRenderSurface::Render(
        Element& root,
        unsigned char* destination,
        int destinationStride) {
        this->implementation->Render(root, destination, destinationStride);
    }
}