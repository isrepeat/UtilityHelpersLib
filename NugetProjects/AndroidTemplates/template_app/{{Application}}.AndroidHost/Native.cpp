#if defined(__ANDROID__)
#include <jni.h>
#endif

#if defined(__ANDROID__)
#include <ESRenderer/OpenGlRenderer.h>
#endif

#include "../{{Application}}.Application/Core/ApplicationSession.h"

#include <stdexcept>
#include <iterator>
#include <fstream>
#include <memory>
#include <vector>

namespace _details {
    struct NativeHost final {
        {{application}}::application::core::ApplicationSession session{};
#if defined(__ANDROID__)
        std::unique_ptr<es_renderer::OpenGlRenderer> renderer;
#endif
    };
#if defined(__ANDROID__)
    NativeHost& Host(jlong handle) {
        if (handle == 0) {
            throw std::invalid_argument("Native session is required");
        }
        return *reinterpret_cast<NativeHost*>(handle);
    }

    void ReportError(JNIEnv* environment, const std::exception& error) {
        environment->ThrowNew(environment->FindClass("java/lang/IllegalStateException"), error.what());
    }

    std::vector<unsigned char> ReadFont() {
        std::ifstream stream("/system/fonts/Roboto-Regular.ttf", std::ios::binary);
        if (!stream) {
            throw std::runtime_error("System Roboto font is unavailable");
        }
        return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
    }
#endif
}

#if defined(__ANDROID__)
extern "C" JNIEXPORT jlong JNICALL Java_{{JniPackage}}_MainPage_nativeCreate(JNIEnv* environment, jobject) {
    try {
        return reinterpret_cast<jlong>(new _details::NativeHost{});
    } catch (const std::exception& error) {
        _details::ReportError(environment, error);
        return 0;
    }
}

extern "C" JNIEXPORT void JNICALL Java_{{JniPackage}}_MainPage_nativeDestroy(JNIEnv*, jobject, jlong handle) {
    delete reinterpret_cast<_details::NativeHost*>(handle);
}

extern "C" JNIEXPORT void JNICALL Java_{{JniPackage}}_MainPage_nativeSurface(JNIEnv* environment, jobject, jlong handle, jint width, jint height) {
    try {
        auto& host = _details::Host(handle);
        const auto font = _details::ReadFont();
        host.renderer = std::make_unique<es_renderer::OpenGlRenderer>(width, height,
            font.data(), font.size(), font.data(), font.size(), font.data(), font.size(),
            es_renderer::OpenGlRenderer::ShaderProgramSources{}, es_renderer::OpenGlRenderer::ResourceLoader{});
        xaml::SetTextGlyphMetrics(host.renderer->TextGlyphMetrics());
        host.session.Initialize({static_cast<float>(width), static_cast<float>(height)});
    } catch (const std::exception& error) {
        _details::ReportError(environment, error);
    }
}

extern "C" JNIEXPORT void JNICALL Java_{{JniPackage}}_MainPage_nativeReleaseSurface(JNIEnv* environment, jobject, jlong handle) {
    try {
        auto& host = _details::Host(handle);
        host.session.CancelPointer();
        host.renderer.reset();
    } catch (const std::exception& error) {
        _details::ReportError(environment, error);
    }
}

extern "C" JNIEXPORT void JNICALL Java_{{JniPackage}}_MainPage_nativeRender(JNIEnv* environment, jobject, jlong handle) {
    try {
        auto& host = _details::Host(handle);
        if (host.renderer) {
            host.session.Update();
            host.renderer->BeginFrame();
            host.session.Render(*host.renderer);
        }
    } catch (const std::exception& error) {
        _details::ReportError(environment, error);
    }
}

extern "C" JNIEXPORT void JNICALL Java_{{JniPackage}}_MainPage_nativePointer(JNIEnv* environment, jobject, jlong handle, jint action, jfloat x, jfloat y) {
    try {
        auto& host = _details::Host(handle);
        if (!host.renderer) {
            return;
        }
        switch (action) {
        case 0: host.session.PointerDown(x, y); break;
        case 1: host.session.PointerUp(x, y); break;
        case 2: host.session.PointerMove(x, y); break;
        default: host.session.CancelPointer(); break;
        }
    } catch (const std::exception& error) {
        _details::ReportError(environment, error);
    }
}

extern "C" JNIEXPORT jboolean JNICALL Java_{{JniPackage}}_MainPage_nativeBack(JNIEnv* environment, jobject, jlong handle) {
    try {
        return _details::Host(handle).session.Pages().NavigateBack();
    } catch (const std::exception& error) {
        _details::ReportError(environment, error);
        return false;
    }
}
#else
extern "C" int ApplicationVersion() {
    _details::NativeHost host;
    host.session.Initialize({480, 800});
    return host.session.Pages().CurrentPageName() == "MainPage" ? 1 : 0;
}
#endif