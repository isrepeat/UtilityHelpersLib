#include "StandardShaders.h"

namespace es_renderer::_details {
    // Текстовый pipeline передаёт в шейдер позицию вершины и UV-координаты
    // альфа-канала glyph atlas. Геометрия уже приходит в NDC.
    const char TextVertexShader[] = R"(#version 300 es
        layout (location = 0) in vec2 position;
        layout (location = 1) in vec2 textureCoordinate;
        out vec2 uv;
        void main() {
            uv = textureCoordinate;
            gl_Position = vec4(position, 0.0, 1.0);
        }
    )";

    const char TextFragmentShader[] = R"(#version 300 es
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

    // Программа заливки используется для фона, границ и контуров без текстуры.
    const char SolidVertexShader[] = R"(#version 300 es
        layout (location = 0) in vec2 position;
        void main() {
            gl_Position = vec4(position, 0.0, 1.0);
        }
    )";

    const char SolidFragmentShader[] = R"(#version 300 es
        precision mediump float;
        uniform vec4 color;
        out vec4 fragmentColor;
        void main() {
            fragmentColor = color;
        }
    )";

    const char ImageVertexShader[] = R"(#version 300 es
        layout (location = 0) in vec2 position;
        layout (location = 1) in vec2 textureCoordinate;
        out vec2 uv;
        void main() { uv = textureCoordinate; gl_Position = vec4(position, 0.0, 1.0); }
    )";
    const char ImageFragmentShader[] = R"(#version 300 es
        precision mediump float;
        in vec2 uv;
        uniform sampler2D imageTexture;
        uniform vec4 tint;
        out vec4 color;
        void main() {
            float alpha = texture(imageTexture, uv).a;
            color = vec4(tint.rgb, tint.a * alpha);
        }
    )";
}