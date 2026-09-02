#define NOMINMAX
#include <GLES3/gl3.h>

#undef DrawText

#include "ESRenderer/OpenGlRenderer.h"

#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#pragma warning(disable: 4505)
#include "../../ThirdParty/stb_truetype.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace es_renderer::_details {
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

namespace es_renderer {
    class OpenGlRenderer::Implementation {
    public:
        Implementation(
            int width,
            int height,
            const unsigned char* fontData,
            size_t fontSize);
        ~Implementation();

        Implementation(const Implementation&) = delete;
        Implementation& operator=(const Implementation&) = delete;

        void BeginFrame() const;
        void BeginClip(const xaml::Rect& bounds) const;
        void EndClip() const;
        void DrawOutline(const xaml::Rect& bounds, xaml::attr::Color color);
        void DrawRoundedRectangle(
            const xaml::Rect& bounds,
            xaml::attr::Color color,
            float cornerRadius,
            bool outline,
            float thickness);
        void DrawText(
            const xaml::Rect& bounds,
            std::string_view text,
            xaml::attr::Color color,
            float fontSize,
            std::string_view fontWeight);

    private:
        struct GlyphReference {
            const stbtt_packedchar* glyphs = nullptr;
            int index = 0;
        };

        GLuint CompileShader(GLenum type, const char* source) const;
        GLuint CreateProgram(const char* vertexSource, const char* fragmentSource) const;
        void CreateFontAtlas(const unsigned char* fontData);
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
        GLuint textProgram = 0;
        GLuint solidProgram = 0;
        GLuint vertexBuffer = 0;
        GLuint fontTexture = 0;
        stbtt_packedchar asciiGlyphs[_details::AsciiGlyphCount]{};
        stbtt_packedchar cyrillicGlyphs[_details::CyrillicGlyphCount]{};
        stbtt_packedchar settingsGlyph[1]{};
    };

    OpenGlRenderer::Implementation::Implementation(
        int width,
        int height,
        const unsigned char* fontData,
        size_t fontSize)
        : width(width)
        , height(height) {
        if (width <= 0 || height <= 0 || fontData == nullptr || fontSize == 0) {
            throw std::invalid_argument("Invalid OpenGL renderer arguments");
        }

        this->textProgram = this->CreateProgram(
            _details::TextVertexShader,
            _details::TextFragmentShader);
        this->solidProgram = this->CreateProgram(
            _details::SolidVertexShader,
            _details::SolidFragmentShader);
        glGenBuffers(1, &this->vertexBuffer);
        this->CreateFontAtlas(fontData);
    }

    OpenGlRenderer::Implementation::~Implementation() {
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
    }

    void OpenGlRenderer::Implementation::BeginFrame() const {
        glViewport(0, 0, this->width, this->height);
        glDisable(GL_SCISSOR_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    GLuint OpenGlRenderer::Implementation::CompileShader(GLenum type, const char* source) const {
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

    GLuint OpenGlRenderer::Implementation::CreateProgram(
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

    void OpenGlRenderer::Implementation::CreateFontAtlas(const unsigned char* fontData) {
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
            fontData,
            0,
            _details::AtlasFontSize,
            _details::FirstAsciiGlyph,
            _details::AsciiGlyphCount,
            this->asciiGlyphs);
        const int cyrillicPacked = asciiPacked == 0 ? 0 : stbtt_PackFontRange(
            &packingContext,
            fontData,
            0,
            _details::AtlasFontSize,
            _details::FirstCyrillicGlyph,
            _details::CyrillicGlyphCount,
            this->cyrillicGlyphs);
        const int settingsPacked = cyrillicPacked == 0 ? 0 : stbtt_PackFontRange(
            &packingContext,
            fontData,
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

    void OpenGlRenderer::Implementation::EndClip() const {
        glDisable(GL_SCISSOR_TEST);
    }

    void OpenGlRenderer::Implementation::DrawOutline(
        const xaml::Rect& bounds,
        xaml::attr::Color color) {
        std::vector<float> vertices;
        vertices.reserve(8);
        this->AppendPosition(vertices, bounds.x, bounds.y);
        this->AppendPosition(vertices, bounds.x + bounds.width, bounds.y);
        this->AppendPosition(
            vertices,
            bounds.x + bounds.width,
            bounds.y + bounds.height);
        this->AppendPosition(vertices, bounds.x, bounds.y + bounds.height);

        glUseProgram(this->solidProgram);
        glUniform4f(
            glGetUniformLocation(this->solidProgram, "color"),
            color.red,
            color.green,
            color.blue,
            color.alpha);
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

    void OpenGlRenderer::Implementation::DrawRoundedRectangle(
        const xaml::Rect& bounds,
        xaml::attr::Color color,
        float cornerRadius,
        bool outline,
        float thickness) {
        constexpr int segmentsPerCorner = 8;
        constexpr float pi = 3.14159265358979323846f;
        // Радиус ограничивается половиной каждой стороны: иначе дуги углов
        // пересекаются на очень узких прямоугольниках.
        const float radius = std::min({
            cornerRadius,
            bounds.width / 2.0f,
            bounds.height / 2.0f,
        });
        std::vector<float> vertices;
        vertices.reserve((outline ? 32 : 34) * 2);
        // Для заливки центр вместе с обходом границы образует triangle fan;
        // для контура требуются только точки по периметру.
        if (!outline) {
            this->AppendPosition(
                vertices,
                bounds.x + bounds.width / 2.0f,
                bounds.y + bounds.height / 2.0f);
        }
        const float centers[][2] = {
            {bounds.x + bounds.width - radius, bounds.y + radius},
            {
                bounds.x + bounds.width - radius,
                bounds.y + bounds.height - radius,
            },
            {bounds.x + radius, bounds.y + bounds.height - radius},
            {bounds.x + radius, bounds.y + radius},
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
                bounds.x + bounds.width - radius,
                bounds.y);
        }

        glUseProgram(this->solidProgram);
        glUniform4f(
            glGetUniformLocation(this->solidProgram, "color"),
            color.red,
            color.green,
            color.blue,
            color.alpha);
        glBindBuffer(GL_ARRAY_BUFFER, this->vertexBuffer);
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
            vertices.data(),
            GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
        if (outline) {
            glLineWidth(thickness);
            glDrawArrays(GL_LINE_LOOP, 0, static_cast<GLsizei>(vertices.size() / 2));
        }
        else {
            glDrawArrays(GL_TRIANGLE_FAN, 0, static_cast<GLsizei>(vertices.size() / 2));
        }
    }

    void OpenGlRenderer::Implementation::DrawText(
        const xaml::Rect& bounds,
        std::string_view text,
        xaml::attr::Color color,
        float fontSize,
        std::string_view fontWeight) {
        if (text.empty()) {
            return;
        }
        // stb_truetype выдаёт метрики для размера atlas; scale переводит их
        // обратно в fontSize, заданный XAML-командой.
        const float scale = fontSize / _details::AtlasFontSize;
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
            bounds.x + (bounds.width - textWidth) * 0.5f) / scale;
        float cursorY = (
            bounds.y
            + (bounds.height - (maximumY - minimumY)) * 0.5f
            - minimumY) / scale;
        std::vector<float> vertices;
        vertices.reserve(static_cast<size_t>(glyphCount) * 6 * 4 * 2);
        current = text.data();
        int renderedGlyphs = 0;
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
            color.red,
            color.green,
            color.blue,
            color.alpha);
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

    void OpenGlRenderer::Implementation::BeginClip(const xaml::Rect& bounds) const {
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

    void OpenGlRenderer::Implementation::AppendPosition(
        std::vector<float>& vertices,
        float x,
        float y) const {
        // Преобразуем пиксельные координаты XAML в normalized device coordinates.
        vertices.push_back(x * 2.0f / this->width - 1.0f);
        vertices.push_back(1.0f - y * 2.0f / this->height);
    }

    void OpenGlRenderer::Implementation::AppendTextVertex(
        std::vector<float>& vertices,
        float x,
        float y,
        float textureX,
        float textureY) const {
        this->AppendPosition(vertices, x, y);
        vertices.push_back(textureX);
        vertices.push_back(textureY);
    }

    void OpenGlRenderer::Implementation::AppendTextQuad(
        std::vector<float>& vertices,
        const stbtt_aligned_quad& quad) const {
        this->AppendTextVertex(vertices, quad.x0, quad.y0, quad.s0, quad.t0);
        this->AppendTextVertex(vertices, quad.x1, quad.y0, quad.s1, quad.t0);
        this->AppendTextVertex(vertices, quad.x1, quad.y1, quad.s1, quad.t1);
        this->AppendTextVertex(vertices, quad.x0, quad.y0, quad.s0, quad.t0);
        this->AppendTextVertex(vertices, quad.x1, quad.y1, quad.s1, quad.t1);
        this->AppendTextVertex(vertices, quad.x0, quad.y1, quad.s0, quad.t1);
    }

    OpenGlRenderer::Implementation::GlyphReference
    OpenGlRenderer::Implementation::GetGlyph(uint32_t codepoint) const {
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

    OpenGlRenderer::OpenGlRenderer(
        int width,
        int height,
        const unsigned char* fontData,
        size_t fontSize)
        : implementation(std::make_unique<Implementation>(
            width,
            height,
            fontData,
            fontSize)) {
    }

    OpenGlRenderer::~OpenGlRenderer() = default;

    //
    // API
    //
    void OpenGlRenderer::BeginFrame() {
        this->implementation->BeginFrame();
    }

    //
    // IRenderBackend
    //
    void OpenGlRenderer::BeginClip(const xaml::Rect& bounds) {
        this->implementation->BeginClip(bounds);
    }

    void OpenGlRenderer::EndClip() {
        this->implementation->EndClip();
    }

    void OpenGlRenderer::DrawOutline(const xaml::Rect& bounds, xaml::attr::Color color) {
        this->implementation->DrawOutline(bounds, color);
    }

    void OpenGlRenderer::DrawRoundedRect(
        const xaml::Rect& bounds,
        xaml::attr::Color color,
        float cornerRadius) {
        this->implementation->DrawRoundedRectangle(
            bounds,
            color,
            cornerRadius,
            false,
            1.0f);
    }

    void OpenGlRenderer::DrawRoundedRectOutline(
        const xaml::Rect& bounds,
        xaml::attr::Color color,
        float cornerRadius,
        float thickness) {
        this->implementation->DrawRoundedRectangle(
            bounds,
            color,
            cornerRadius,
            true,
            thickness);
    }

    void OpenGlRenderer::DrawText(
        const xaml::Rect& bounds,
        std::string_view text,
        xaml::attr::Color color,
        float fontSize,
        std::string_view fontWeight) {
        this->implementation->DrawText(bounds, text, color, fontSize, fontWeight);
    }

    void OpenGlRenderer::DrawImage(
        const xaml::Rect&,
        std::string_view,
        xaml::attr::Color) {
    }
}