#pragma once

#ifdef _WIN32
#define XAML_RUNTIME_BRIDGE_API __declspec(dllexport)
#else
#define XAML_RUNTIME_BRIDGE_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct xr_element xr_element;
typedef struct xr_animation_controller xr_animation_controller;
typedef struct xr_angle_surface xr_angle_surface;

typedef struct xr_rect {
    float x;
    float y;
    float width;
    float height;
} xr_rect;

typedef struct xr_color {
    float red;
    float green;
    float blue;
    float alpha;
} xr_color;

typedef enum xr_command_type {
    xr_command_type_begin_clip,
    xr_command_type_end_clip,
    xr_command_type_outline,
    xr_command_type_rounded_rect,
    xr_command_type_rounded_rect_outline,
    xr_command_type_text,
    xr_command_type_image,
} xr_command_type;

typedef struct xr_command {
    int type;
    xr_rect bounds;
    xr_color color;
    float value;
    char text[512];
    char auxiliary[128];
} xr_command;

XAML_RUNTIME_BRIDGE_API const char* xr_last_error(void);
XAML_RUNTIME_BRIDGE_API void xr_configure_logging(const char* filePath);
XAML_RUNTIME_BRIDGE_API xr_element* xr_create_element(const char* type);
XAML_RUNTIME_BRIDGE_API void xr_destroy_element(xr_element* element);
XAML_RUNTIME_BRIDGE_API int xr_add_child(xr_element* parent, xr_element* child);
XAML_RUNTIME_BRIDGE_API int xr_set_attribute(
    xr_element* element,
    const char* name,
    const char* value);
XAML_RUNTIME_BRIDGE_API int xr_add_storyboard_track(
    xr_element* element,
    int trigger,
    int property,
    float from,
    float to,
    int durationMilliseconds,
    int easing,
    float intensity,
    float spread,
    float fadeExponent);
XAML_RUNTIME_BRIDGE_API int xr_supported_attribute_count(const char* elementType);
XAML_RUNTIME_BRIDGE_API const char* xr_supported_attribute_name(
    const char* elementType,
    int index);
XAML_RUNTIME_BRIDGE_API int xr_supported_element_count(void);
XAML_RUNTIME_BRIDGE_API const char* xr_supported_element_name(int index);
XAML_RUNTIME_BRIDGE_API int xr_layout(xr_element* root, float width, float height);
XAML_RUNTIME_BRIDGE_API xr_element* xr_hit_test(xr_element* root, float x, float y);
XAML_RUNTIME_BRIDGE_API xr_element* xr_hit_test_visual(xr_element* root, float x, float y);
XAML_RUNTIME_BRIDGE_API int xr_element_bounds(const xr_element* element, xr_rect* bounds);
XAML_RUNTIME_BRIDGE_API const char* xr_element_id(const xr_element* element);
XAML_RUNTIME_BRIDGE_API int xr_handle_tap(
    xr_element* element,
    xr_animation_controller* animations);
XAML_RUNTIME_BRIDGE_API int xr_handle_pointer_down(
    xr_element* element,
    xr_animation_controller* animations);
XAML_RUNTIME_BRIDGE_API int xr_handle_pointer_up(
    xr_element* element,
    xr_animation_controller* animations);
XAML_RUNTIME_BRIDGE_API xr_animation_controller* xr_create_animation_controller(void);
XAML_RUNTIME_BRIDGE_API void xr_destroy_animation_controller(
    xr_animation_controller* animations);
XAML_RUNTIME_BRIDGE_API int xr_set_animation_playback_rate(
    xr_animation_controller* animations,
    float playbackRate);
XAML_RUNTIME_BRIDGE_API int xr_update_animations(xr_animation_controller* animations);
XAML_RUNTIME_BRIDGE_API xr_angle_surface* xr_create_angle_surface(
    int width,
    int height,
    const char* fontPath,
    const char* resourceRoot);
XAML_RUNTIME_BRIDGE_API void xr_destroy_angle_surface(xr_angle_surface* surface);
XAML_RUNTIME_BRIDGE_API int xr_render_angle_surface(
    xr_angle_surface* surface,
    const xr_element* root,
    unsigned char* destination,
    int destinationStride,
    int destinationCapacity);
XAML_RUNTIME_BRIDGE_API int xr_render(
    const xr_element* root,
    xr_command* destination,
    int capacity);
XAML_RUNTIME_BRIDGE_API int xr_render_angle(
    const xr_element* root,
    const char* fontPath,
    int width,
    int height,
    const char* resourceRoot,
    unsigned char* destination,
    int destinationStride,
    int destinationCapacity);

#ifdef __cplusplus
}
#endif