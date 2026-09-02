#define NOMINMAX
#include <EGL/egl.h>
#include <GLES3/gl3.h>

#undef DrawText

#include "AngleRenderSurface.h"

#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#pragma warning(disable: 4505)
#include "../../../../ThirdParty/stb_truetype.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace xaml::bridge::_details {
    constexpr int FirstAsciiGlyph = 32;
    constexpr int AsciiGlyphCount = 96;
    constexpr int FirstCyrillicGlyph = 0x0400;
    constexpr int CyrillicGlyphCount = 256;
    constexpr int SettingsGlyph = 0x2699;
    constexpr float AtlasFontSize = 64.0f;
    constexpr int AtlasWidth = 1024;
    constexpr int AtlasHeight = 1024;
    constexpr int MaximumTextGlyphs = 256;

    // Текстовый pipeline передаёт в шейдер позицию вершины и UV-координаты
    // альфа-канала glyph atlas. Геометрия уже приходит в NDC.
    constexpr char TextVertexShader[] = R"(#version 300 es
        layout (location = 0) in vec2 position;
        layout (location = 1) in vec2 textureCoordinate;
        out vec2 uv;
        void main() {
            uv = textureCoordinate;
            gl_Position = vec4(position, 0.0, 1.0);
        }
    )";

    constexpr char TextFragmentShader[] = R"(#version 300 es
        precision mediump float;
        in vec2 uv;
        uniform sampler2D fontAtlas;
        uniform vec4 textColor;
        out vec4 color;
        void main() {
            float alpha = texture(fontAtlas, uv).r;
            color = vec4(textColor.rgb, textColor.a * alpha);
        }
    )";

    // Solid pipeline используется для фона, границ и контуров без текстуры.
    constexpr char SolidVertexShader[] = R"(#version 300 es
        layout (location = 0) in vec2 position;
        void main() {
            gl_Position = vec4(position, 0.0, 1.0);
        }
    )";

    constexpr char SolidFragmentShader[] = R"(#version 300 es
        precision mediump float;
        uniform vec4 color;
        out vec4 fragmentColor;
        void main() {
            fragmentColor = color;
        }
    )";

    // Команды runtime хранят текст в UTF-8, а stb_truetype ожидает code point.
    // Повреждённая либо неподдерживаемая последовательность заменяется на '?'.
    uint32_t DecodeUtf8(const char*& current, const char* end) {
        const auto lead = static_cast<unsigned char>(*current++);
        if (lead < 0x80) {
            return lead;
        }
        if ((lead & 0xE0) == 0xC0 && current < end) {
            const auto second = static_cast<unsigned char>(*current++);
            return static_cast<uint32_t>(lead & 0x1F) << 6
                | static_cast<uint32_t>(second & 0x3F);
        }
        if ((lead & 0xF0) == 0xE0 && end - current >= 2) {
            const auto second = static_cast<unsigned char>(*current++);
            const auto third = static_cast<unsigned char>(*current++);
            return static_cast<uint32_t>(lead & 0x0F) << 12
                | static_cast<uint32_t>(second & 0x3F) << 6
                | static_cast<uint32_t>(third & 0x3F);
        }
        if ((lead & 0xF8) == 0xF0 && end - current >= 3) {
            const auto second = static_cast<unsigned char>(*current++);
            const auto third = static_cast<unsigned char>(*current++);
            const auto fourth = static_cast<unsigned char>(*current++);
            return static_cast<uint32_t>(lead & 0x07) << 18
                | static_cast<uint32_t>(second & 0x3F) << 12
                | static_cast<uint32_t>(third & 0x3F) << 6
                | static_cast<uint32_t>(fourth & 0x3F);
        }
        return '?';
    }
}

namespace xaml::bridge {
    class AngleRenderSurface::Implementation {
    public:
        Implementation(int width, int height, std::string_view fontPath);
        ~Implementation();

        Implementation(const Implementation&) = delete;
        Implementation& operator=(const Implementation&) = delete;

        void Render(
            const xr_command* commands,
            int commandCount,
            unsigned char* destination,
            int destinationStride);

    private:
        struct GlyphReference {
            const stbtt_packedchar* glyphs = nullptr;
            int index = 0;
        };

        GLuint CompileShader(GLenum type, const char* source) const;
        GLuint CreateProgram(const char* vertexSource, const char* fragmentSource) const;
        void CreateFontAtlas(std::string_view fontPath);
        void DrawCommand(const xr_command& command);
        void DrawOutline(const xr_command& command);
        void DrawRoundedRectangle(const xr_command& command, bool outline);
        void DrawText(const xr_command& command);
        void BeginClip(const xr_rect& bounds) const;
        void ReadPixels(unsigned char* destination, int destinationStride) const;
        void AppendPosition(std::vector<float>& vertices, float x, float y) const;
        void AppendTextVertex(
            std::vector<float>& vertices,
            float x,
            float y,
            float textureX,
            float textureY) const;
        void AppendTextQuad(std::vector<float>& vertices, const stbtt_aligned_quad& quad) const;
        GlyphReference GetGlyph(uint32_t codepoint) const;

    private:
        int width;
        int height;
        EGLDisplay display = EGL_NO_DISPLAY;
        EGLSurface surface = EGL_NO_SURFACE;
        EGLContext context = EGL_NO_CONTEXT;
        GLuint textProgram = 0;
        GLuint solidProgram = 0;
        GLuint vertexBuffer = 0;
        GLuint fontTexture = 0;
        stbtt_packedchar asciiGlyphs[_details::AsciiGlyphCount]{};
        stbtt_packedchar cyrillicGlyphs[_details::CyrillicGlyphCount]{};
        stbtt_packedchar settingsGlyph[1]{};
    };

    AngleRenderSurface::Implementation::Implementation(
        int width,
        int height,
        std::string_view fontPath)
        : width(width)
        , height(height) {
        if (width <= 0 || height <= 0) {
            throw std::invalid_argument("ANGLE surface dimensions must be positive");
        }

        // ANGLE реализует EGL/OpenGL ES поверх D3D; EGL_DEFAULT_DISPLAY не
        // создаёт собственное окно и подходит для desktop offscreen rendering.
        this->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (this->display == EGL_NO_DISPLAY || eglInitialize(this->display, nullptr, nullptr) == EGL_FALSE) {
            throw std::runtime_error("ANGLE could not initialize EGL display");
        }
        if (eglBindAPI(EGL_OPENGL_ES_API) == EGL_FALSE) {
            throw std::runtime_error("ANGLE could not bind OpenGL ES API");
        }

        // Запрашиваем RGBA pbuffer и ES 3: итоговый буфер должен без потерь
        // переноситься в WPF BitmapSource формата BGRA32.
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
            throw std::runtime_error("ANGLE could not choose an RGBA pbuffer configuration");
        }

        const EGLint surfaceAttributes[] = {
            EGL_WIDTH, this->width,
            EGL_HEIGHT, this->height,
            EGL_NONE,
        };
        // Pbuffer не требует HWND: preview получает кадр через glReadPixels.
        this->surface = eglCreatePbufferSurface(this->display, configuration, surfaceAttributes);
        if (this->surface == EGL_NO_SURFACE) {
            throw std::runtime_error("ANGLE could not create an offscreen pbuffer");
        }

        const EGLint contextAttributes[] = {
            EGL_CONTEXT_CLIENT_VERSION, 3,
            EGL_NONE,
        };
        this->context = eglCreateContext(
            this->display,
            configuration,
            EGL_NO_CONTEXT,
            contextAttributes);
        if (this->context == EGL_NO_CONTEXT
            || eglMakeCurrent(this->display, this->surface, this->surface, this->context) == EGL_FALSE) {
            throw std::runtime_error("ANGLE could not create an OpenGL ES 3 context");
        }

        this->textProgram = this->CreateProgram(
            _details::TextVertexShader,
            _details::TextFragmentShader);
        this->solidProgram = this->CreateProgram(
            _details::SolidVertexShader,
            _details::SolidFragmentShader);
        glGenBuffers(1, &this->vertexBuffer);
        this->CreateFontAtlas(fontPath);
    }

    AngleRenderSurface::Implementation::~Implementation() {
        if (this->display != EGL_NO_DISPLAY && this->context != EGL_NO_CONTEXT) {
            eglMakeCurrent(this->display, this->surface, this->surface, this->context);
            if (this->fontTexture != 0) {
                glDeleteTextures(1, &this->fontTexture);
            }
            if (this->vertexBuffer != 0) {
                glDeleteBuffers(1, &this->vertexBuffer);
            }
            if (this->textProgram != 0) {
                glDeleteProgram(this->textProgram);
            }
            if (this->solidProgram != 0) {
                glDeleteProgram(this->solidProgram);
            }
            eglMakeCurrent(this->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        }
        if (this->display != EGL_NO_DISPLAY) {
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
        const xr_command* commands,
        int commandCount,
        unsigned char* destination,
        int destinationStride) {
        if (commands == nullptr || commandCount < 0 || destination == nullptr
            || destinationStride < this->width * 4) {
            throw std::invalid_argument("Invalid ANGLE render buffer arguments");
        }
        if (eglMakeCurrent(this->display, this->surface, this->surface, this->context) == EGL_FALSE) {
            throw std::runtime_error("ANGLE could not activate the offscreen context");
        }

        // Surface живёт только на время одного вызова bridge. Каждый кадр
        // полностью перерисовывается из RecordingBackend-команд.
        glViewport(0, 0, this->width, this->height);
        glDisable(GL_SCISSOR_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        for (int index = 0; index < commandCount; ++index) {
            this->DrawCommand(commands[index]);
        }
        glFinish();
        this->ReadPixels(destination, destinationStride);
    }

    GLuint AngleRenderSurface::Implementation::CompileShader(GLenum type, const char* source) const {
        const GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);
        GLint compiled = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (compiled == GL_FALSE) {
            GLint logLength = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
            std::string log(static_cast<size_t>(std::max(1, logLength)), '\0');
            glGetShaderInfoLog(shader, logLength, nullptr, log.data());
            glDeleteShader(shader);
            throw std::runtime_error("ANGLE shader compilation failed: " + log);
        }
        return shader;
    }

    GLuint AngleRenderSurface::Implementation::CreateProgram(
        const char* vertexSource,
        const char* fragmentSource) const {
        const GLuint vertexShader = this->CompileShader(GL_VERTEX_SHADER, vertexSource);
        const GLuint fragmentShader = this->CompileShader(GL_FRAGMENT_SHADER, fragmentSource);
        const GLuint program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        GLint linked = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (linked == GL_FALSE) {
            GLint logLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
            std::string log(static_cast<size_t>(std::max(1, logLength)), '\0');
            glGetProgramInfoLog(program, logLength, nullptr, log.data());
            glDeleteProgram(program);
            throw std::runtime_error("ANGLE program linking failed: " + log);
        }
        return program;
    }

    void AngleRenderSurface::Implementation::CreateFontAtlas(std::string_view fontPath) {
        std::ifstream stream(std::string(fontPath), std::ios::binary | std::ios::ate);
        if (!stream) {
            throw std::runtime_error("ANGLE renderer could not open the font file");
        }
        const auto length = stream.tellg();
        if (length <= 0) {
            throw std::runtime_error("ANGLE renderer received an empty font file");
        }
        std::vector<unsigned char> fontData(static_cast<size_t>(length));
        stream.seekg(0, std::ios::beg);
        if (!stream.read(
            reinterpret_cast<char*>(fontData.data()),
            static_cast<std::streamsize>(length))) {
            throw std::runtime_error("ANGLE renderer could not read the font file");
        }

        // Один atlas покрывает ASCII, кириллицу и значок настроек из тестовой разметки.
        std::vector<unsigned char> atlas(_details::AtlasWidth * _details::AtlasHeight, 0);
        stbtt_pack_context packingContext{};
        const int packingStarted = stbtt_PackBegin(
            &packingContext,
            atlas.data(),
            _details::AtlasWidth,
            _details::AtlasHeight,
            0,
            1,
            nullptr);
        const int asciiPacked = packingStarted == 0 ? 0 : stbtt_PackFontRange(
            &packingContext,
            fontData.data(),
            0,
            _details::AtlasFontSize,
            _details::FirstAsciiGlyph,
            _details::AsciiGlyphCount,
            this->asciiGlyphs);
        const int cyrillicPacked = asciiPacked == 0 ? 0 : stbtt_PackFontRange(
            &packingContext,
            fontData.data(),
            0,
            _details::AtlasFontSize,
            _details::FirstCyrillicGlyph,
            _details::CyrillicGlyphCount,
            this->cyrillicGlyphs);
        const int settingsPacked = cyrillicPacked == 0 ? 0 : stbtt_PackFontRange(
            &packingContext,
            fontData.data(),
            0,
            _details::AtlasFontSize,
            _details::SettingsGlyph,
            1,
            this->settingsGlyph);
        if (packingStarted != 0) {
            stbtt_PackEnd(&packingContext);
        }
        if (asciiPacked == 0 || cyrillicPacked == 0 || settingsPacked == 0) {
            throw std::runtime_error("ANGLE renderer font atlas is too small");
        }

        glGenTextures(1, &this->fontTexture);
        glBindTexture(GL_TEXTURE_2D, this->fontTexture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_R8,
            _details::AtlasWidth,
            _details::AtlasHeight,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            atlas.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    void AngleRenderSurface::Implementation::DrawCommand(const xr_command& command) {
        switch (command.type) {
        case xr_command_type_begin_clip:
            this->BeginClip(command.bounds);
            break;
        case xr_command_type_end_clip:
            glDisable(GL_SCISSOR_TEST);
            break;
        case xr_command_type_outline:
            this->DrawOutline(command);
            break;
        case xr_command_type_rounded_rect:
            this->DrawRoundedRectangle(command, false);
            break;
        case xr_command_type_rounded_rect_outline:
            this->DrawRoundedRectangle(command, true);
            break;
        case xr_command_type_text:
            this->DrawText(command);
            break;
        case xr_command_type_image:
            break;
        default:
            throw std::runtime_error("ANGLE renderer received an unknown command");
        }
    }

    void AngleRenderSurface::Implementation::DrawOutline(const xr_command& command) {
        std::vector<float> vertices;
        vertices.reserve(8);
        this->AppendPosition(vertices, command.bounds.x, command.bounds.y);
        this->AppendPosition(vertices, command.bounds.x + command.bounds.width, command.bounds.y);
        this->AppendPosition(
            vertices,
            command.bounds.x + command.bounds.width,
            command.bounds.y + command.bounds.height);
        this->AppendPosition(vertices, command.bounds.x, command.bounds.y + command.bounds.height);

        glUseProgram(this->solidProgram);
        glUniform4f(
            glGetUniformLocation(this->solidProgram, "color"),
            command.color.red,
            command.color.green,
            command.color.blue,
            command.color.alpha);
        glBindBuffer(GL_ARRAY_BUFFER, this->vertexBuffer);
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
            vertices.data(),
            GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
        glLineWidth(2.0f);
        glDrawArrays(GL_LINE_LOOP, 0, 4);
    }

    void AngleRenderSurface::Implementation::DrawRoundedRectangle(
        const xr_command& command,
        bool outline) {
        constexpr int segmentsPerCorner = 8;
        constexpr float pi = 3.14159265358979323846f;
        // Радиус ограничивается половиной каждой стороны: иначе дуги углов
        // пересекаются на очень узких прямоугольниках.
        const float radius = std::min({
            command.value,
            command.bounds.width / 2.0f,
            command.bounds.height / 2.0f,
        });
        std::vector<float> vertices;
        vertices.reserve((outline ? 32 : 34) * 2);
        // Для заливки центр вместе с обходом границы образует triangle fan;
        // для контура требуются только точки по периметру.
        if (!outline) {
            this->AppendPosition(
                vertices,
                command.bounds.x + command.bounds.width / 2.0f,
                command.bounds.y + command.bounds.height / 2.0f);
        }
        const float centers[][2] = {
            {command.bounds.x + command.bounds.width - radius, command.bounds.y + radius},
            {
                command.bounds.x + command.bounds.width - radius,
                command.bounds.y + command.bounds.height - radius,
            },
            {command.bounds.x + radius, command.bounds.y + command.bounds.height - radius},
            {command.bounds.x + radius, command.bounds.y + radius},
        };
        for (int corner = 0; corner < 4; ++corner) {
            const float startAngle = -pi / 2.0f + static_cast<float>(corner) * pi / 2.0f;
            for (int segment = 0; segment < segmentsPerCorner; ++segment) {
                const float angle = startAngle
                    + static_cast<float>(segment) * pi / (2.0f * segmentsPerCorner);
                this->AppendPosition(
                    vertices,
                    centers[corner][0] + std::cos(angle) * radius,
                    centers[corner][1] + std::sin(angle) * radius);
            }
        }
        if (!outline) {
            this->AppendPosition(
                vertices,
                command.bounds.x + command.bounds.width - radius,
                command.bounds.y);
        }

        glUseProgram(this->solidProgram);
        glUniform4f(
            glGetUniformLocation(this->solidProgram, "color"),
            command.color.red,
            command.color.green,
            command.color.blue,
            command.color.alpha);
        glBindBuffer(GL_ARRAY_BUFFER, this->vertexBuffer);
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
            vertices.data(),
            GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
        if (outline) {
            float thickness = 1.0f;
            static_assert(sizeof(thickness) <= sizeof(command.auxiliary));
            std::memcpy(&thickness, command.auxiliary, sizeof(thickness));
            glLineWidth(thickness);
            glDrawArrays(GL_LINE_LOOP, 0, static_cast<GLsizei>(vertices.size() / 2));
        }
        else {
            glDrawArrays(GL_TRIANGLE_FAN, 0, static_cast<GLsizei>(vertices.size() / 2));
        }
    }

    void AngleRenderSurface::Implementation::DrawText(const xr_command& command) {
        const std::string_view text(command.text);
        if (text.empty()) {
            return;
        }
        // stb_truetype выдаёт метрики для размера atlas; scale переводит их
        // обратно в fontSize, заданный XAML-командой.
        const float scale = command.value / _details::AtlasFontSize;
        float textWidth = 0.0f;
        float minimumY = std::numeric_limits<float>::max();
        float maximumY = std::numeric_limits<float>::lowest();
        int glyphCount = 0;
        const char* current = text.data();
        const char* const end = text.data() + text.size();
        while (current < end && glyphCount < _details::MaximumTextGlyphs) {
            const GlyphReference reference = this->GetGlyph(_details::DecodeUtf8(current, end));
            const stbtt_packedchar& glyph = reference.glyphs[reference.index];
            textWidth += glyph.xadvance * scale;
            minimumY = std::min(minimumY, glyph.yoff * scale);
            maximumY = std::max(maximumY, glyph.yoff2 * scale);
            ++glyphCount;
        }

        // Первый проход собрал метрики всей строки. Теперь центрируем её в
        // bounds и вторым проходом формируем по два треугольника на glyph.
        float cursorX = (
            command.bounds.x + (command.bounds.width - textWidth) * 0.5f) / scale;
        float cursorY = (
            command.bounds.y
            + (command.bounds.height - (maximumY - minimumY)) * 0.5f
            - minimumY) / scale;
        std::vector<float> vertices;
        vertices.reserve(static_cast<size_t>(glyphCount) * 6 * 4 * 2);
        current = text.data();
        int renderedGlyphs = 0;
        const std::string_view fontWeight(command.auxiliary);
        const bool isBold = fontWeight == "Bold" || fontWeight == "SemiBold";
        while (current < end && renderedGlyphs < _details::MaximumTextGlyphs) {
            const GlyphReference reference = this->GetGlyph(_details::DecodeUtf8(current, end));
            stbtt_aligned_quad quad{};
            stbtt_GetPackedQuad(
                reference.glyphs,
                _details::AtlasWidth,
                _details::AtlasHeight,
                reference.index,
                &cursorX,
                &cursorY,
                &quad,
                1);
            quad.x0 *= scale;
            quad.x1 *= scale;
            quad.y0 *= scale;
            quad.y1 *= scale;
            this->AppendTextQuad(vertices, quad);
            if (isBold) {
                quad.x0 += 1.0f;
                quad.x1 += 1.0f;
                this->AppendTextQuad(vertices, quad);
            }
            ++renderedGlyphs;
        }

        glUseProgram(this->textProgram);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, this->fontTexture);
        glUniform1i(glGetUniformLocation(this->textProgram, "fontAtlas"), 0);
        glUniform4f(
            glGetUniformLocation(this->textProgram, "textColor"),
            command.color.red,
            command.color.green,
            command.color.blue,
            command.color.alpha);
        glBindBuffer(GL_ARRAY_BUFFER, this->vertexBuffer);
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
            vertices.data(),
            GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(
            1,
            2,
            GL_FLOAT,
            GL_FALSE,
            4 * sizeof(float),
            reinterpret_cast<void*>(2 * sizeof(float)));
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size() / 4));
    }

    void AngleRenderSurface::Implementation::BeginClip(const xr_rect& bounds) const {
        // Координаты XAML отсчитываются сверху, тогда как glScissor — снизу.
        // Поэтому вертикальные границы инвертируются перед включением scissor.
        const int left = std::max(0, static_cast<int>(std::floor(bounds.x)));
        const int right = std::min(
            this->width,
            static_cast<int>(std::ceil(bounds.x + bounds.width)));
        const int top = std::max(0, static_cast<int>(std::floor(bounds.y)));
        const int bottom = std::min(
            this->height,
            static_cast<int>(std::ceil(bounds.y + bounds.height)));
        glEnable(GL_SCISSOR_TEST);
        glScissor(
            left,
            this->height - bottom,
            std::max(0, right - left),
            std::max(0, bottom - top));
    }

    void AngleRenderSurface::Implementation::ReadPixels(
        unsigned char* destination,
        int destinationStride) const {
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
        // OpenGL начинает отсчёт строк снизу и возвращает RGBA; WPF ожидает
        // верхнюю строку первой и формат BGRA32.
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
    }

    void AngleRenderSurface::Implementation::AppendPosition(
        std::vector<float>& vertices,
        float x,
        float y) const {
        // Преобразуем пиксельные координаты XAML в normalized device coordinates.
        vertices.push_back(x * 2.0f / this->width - 1.0f);
        vertices.push_back(1.0f - y * 2.0f / this->height);
    }

    void AngleRenderSurface::Implementation::AppendTextVertex(
        std::vector<float>& vertices,
        float x,
        float y,
        float textureX,
        float textureY) const {
        this->AppendPosition(vertices, x, y);
        vertices.push_back(textureX);
        vertices.push_back(textureY);
    }

    void AngleRenderSurface::Implementation::AppendTextQuad(
        std::vector<float>& vertices,
        const stbtt_aligned_quad& quad) const {
        this->AppendTextVertex(vertices, quad.x0, quad.y0, quad.s0, quad.t0);
        this->AppendTextVertex(vertices, quad.x1, quad.y0, quad.s1, quad.t0);
        this->AppendTextVertex(vertices, quad.x1, quad.y1, quad.s1, quad.t1);
        this->AppendTextVertex(vertices, quad.x0, quad.y0, quad.s0, quad.t0);
        this->AppendTextVertex(vertices, quad.x1, quad.y1, quad.s1, quad.t1);
        this->AppendTextVertex(vertices, quad.x0, quad.y1, quad.s0, quad.t1);
    }

    AngleRenderSurface::Implementation::GlyphReference
    AngleRenderSurface::Implementation::GetGlyph(uint32_t codepoint) const {
        if (codepoint >= _details::FirstAsciiGlyph
            && codepoint < _details::FirstAsciiGlyph + _details::AsciiGlyphCount) {
            return {
                this->asciiGlyphs,
                static_cast<int>(codepoint) - _details::FirstAsciiGlyph,
            };
        }
        if (codepoint >= _details::FirstCyrillicGlyph
            && codepoint < _details::FirstCyrillicGlyph + _details::CyrillicGlyphCount) {
            return {
                this->cyrillicGlyphs,
                static_cast<int>(codepoint) - _details::FirstCyrillicGlyph,
            };
        }
        if (codepoint == _details::SettingsGlyph) {
            return {this->settingsGlyph, 0};
        }
        return {this->asciiGlyphs, '?' - _details::FirstAsciiGlyph};
    }

    AngleRenderSurface::AngleRenderSurface(
        int width,
        int height,
        std::string_view fontPath)
        : implementation(std::make_unique<Implementation>(width, height, fontPath)) {
    }

    AngleRenderSurface::~AngleRenderSurface() = default;

    void AngleRenderSurface::Render(
        const xr_command* commands,
        int commandCount,
        unsigned char* destination,
        int destinationStride) {
        this->implementation->Render(commands, commandCount, destination, destinationStride);
    }
}