#pragma once
#include <stdint.h>

#ifdef _WIN32
#define ANDROID_APP_PREVIEWER_PLUGIN_API __declspec(dllexport)
#else
#define ANDROID_APP_PREVIEWER_PLUGIN_API
#endif

#ifdef __cplusplus
namespace AndroidAppPreviewerPluginSDK {
extern "C" {
#endif

enum { 
    xaml_previewer_plugin_abi_version = 3
};

// All strings crossing this ABI are UTF-8 and copied into caller-owned buffers.
// All exported operations use the xp_* prefix; plugin-owned handles are opaque.
ANDROID_APP_PREVIEWER_PLUGIN_API uint32_t xp_get_abi_version(void);
ANDROID_APP_PREVIEWER_PLUGIN_API const char* xp_get_last_error(void);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_get_plugin_info(char* pluginInfoJson, int capacity);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_get_initial_page_id(void* session, char* pageId, int capacity);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_get_navigation_graph(void* session, char* graphJson, int capacity);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_navigate(void* session, const char* navigationRequestJson);
typedef struct xp_element xp_element;
typedef struct xp_animation_controller xp_animation_controller;
typedef struct xp_interaction_controller xp_interaction_controller;
typedef struct xp_angle_surface xp_angle_surface;
typedef struct xp_session xp_session;

typedef struct xp_rect {
    float x;
    float y;
    float width;
    float height;
} xp_rect;

typedef struct xp_session_inspection_result {
    int line;
    int column;
    char sourcePath[1024];
    xp_rect bounds;
} xp_session_inspection_result;

typedef struct xp_color {
    float red;
    float green;
    float blue;
    float alpha;
} xp_color;

typedef struct xp_interaction_result {
    int kind;
    int direction;
    xp_element* target;
    int item_index;
} xp_interaction_result;

typedef enum xp_command_type {
    xp_command_type_begin_clip,
    xp_command_type_end_clip,
    xp_command_type_outline,
    xp_command_type_rounded_rect,
    xp_command_type_rounded_rect_outline,
    xp_command_type_text,
    xp_command_type_image,
} xp_command_type;

typedef struct xp_command {
    int type;
    xp_rect bounds;
    xp_color color;
    float value;
    char text[512];
    char auxiliary[128];
} xp_command;

ANDROID_APP_PREVIEWER_PLUGIN_API const char* xp_last_error(void);
ANDROID_APP_PREVIEWER_PLUGIN_API xp_session* xp_create_session(int width, int height);
ANDROID_APP_PREVIEWER_PLUGIN_API void xp_destroy_session(xp_session* session);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_session_load_page(xp_session* session, const char* page);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_session_current_page(xp_session* session, char* page, int capacity);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_session_is_transitioning(xp_session* session);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_session_navigate_preview_route(xp_session* session, const char* target);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_session_navigate_preview_route_path(xp_session* session, const char* path);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_session_preview_route_graph(xp_session* session, char* graph, int capacity);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_session_preview_page_title(xp_session* session, const char* page, char* title, int capacity);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_session_apply_preview_scenario(
    xp_session* session,
    const char* page,
    const char* json);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_session_export_preview_state(xp_session* session);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_session_can_save_preview_state(xp_session* session);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_session_reload_markup(xp_session* session, const char* page, const char* markup, const char* sourcePath);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_session_resize(xp_session* session, int width, int height);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_session_set_animation_playback_rate(xp_session* session, float value);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_session_set_status(xp_session* session, const char* value);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_session_pointer_down(xp_session* session, float x, float y);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_session_pointer_move(xp_session* session, float x, float y);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_session_pointer_up(xp_session* session, float x, float y);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_session_pointer_cancel(xp_session* session);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_session_cursor_kind(xp_session* session, float x, float y);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_session_inspect(
    xp_session* session,
    float x,
    float y,
    xp_session_inspection_result* result);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_session_set_inspection_wireframe(
    xp_session* session,
    float thickness,
    int lineStyle,
    xp_color color,
    xp_color marginColor,
    xp_color paddingColor);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_session_set_selected_wireframe(
    xp_session* session,
    float thickness,
    int lineStyle,
    xp_color color,
    xp_color marginColor,
    xp_color paddingColor);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_session_clear_inspection_wireframe(xp_session* session);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_session_clear_selected_inspection_element(xp_session* session);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_session_select_inspection_element(
    xp_session* session,
    const char* sourcePath,
    int line,
    int column);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_session_pin_inspection_element(xp_session* session);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_session_update(xp_session* session);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_session_render_angle_surface(
    xp_session* session,
    xp_angle_surface* surface,
    unsigned char* destination,
    int destinationStride,
    int destinationCapacity);
ANDROID_APP_PREVIEWER_PLUGIN_API void xp_configure_logging(const char* filePath);
ANDROID_APP_PREVIEWER_PLUGIN_API void xp_log_info(const char* message);
ANDROID_APP_PREVIEWER_PLUGIN_API xp_element* xp_create_element(const char* type);
ANDROID_APP_PREVIEWER_PLUGIN_API void xp_destroy_element(xp_element* element);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_items_remove_item(xp_element* target);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_add_child(xp_element* parent, xp_element* child);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_set_attribute(
    xp_element* element,
    const char* name,
    const char* value);
ANDROID_APP_PREVIEWER_PLUGIN_API xp_element* xp_find_element(xp_element* root, const char* id);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_find_element_count(xp_element* root, const char* id);
ANDROID_APP_PREVIEWER_PLUGIN_API xp_element* xp_find_element_at(xp_element* root, const char* id, int index);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_add_storyboard_animation(xp_element* element, int trigger,
    const char* name, const char* const* keys, const char* const* values, int count);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_attach_animations(xp_element* root, xp_animation_controller* animations);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_set_page_transition(
    xp_element* root,
    xp_animation_controller* animations,
    const char* from,
    const char* to,
    int backward,
    int visible);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_add_storyboard_track(
    xp_element* element,
    int trigger,
    int property,
    float from,
    float to,
    int durationMilliseconds,
    int easing);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_add_visual_state_track(
    xp_element* scope,
    const char* groupName,
    const char* stateName,
    const char* targetName,
    int property,
    float from,
    float to,
    int durationMilliseconds,
    int easing);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_go_to_visual_state(
    xp_element* scope,
    const char* groupName,
    const char* stateName,
    int useTransitions);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_supported_attribute_count(const char* elementType);
ANDROID_APP_PREVIEWER_PLUGIN_API const char* xp_supported_attribute_name(
    const char* elementType,
    int index);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_supported_element_count(void);
ANDROID_APP_PREVIEWER_PLUGIN_API const char* xp_supported_element_name(int index);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_layout(xp_element* root, float width, float height);
ANDROID_APP_PREVIEWER_PLUGIN_API xp_element* xp_hit_test(xp_element* root, float x, float y);
ANDROID_APP_PREVIEWER_PLUGIN_API xp_element* xp_hit_test_visual(xp_element* root, float x, float y);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_hit_test_cursor_kind(xp_element* root, float x, float y);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_element_bounds(const xp_element* element, xp_rect* bounds);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_get_scroll_offsets(
    xp_element* root,
    const char* scroll_viewer_id,
    float* horizontal_offset,
    float* vertical_offset);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_set_scroll_offsets(
    xp_element* root,
    const char* scroll_viewer_id,
    float horizontal_offset,
    float vertical_offset);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_scroll_by(xp_element* root, float x, float y, float horizontalDelta, float verticalDelta);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_scroll_begin(
    xp_element* root,
    xp_animation_controller* animations,
    float x,
    float y);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_scroll_drag(xp_animation_controller* animations, float verticalDelta);
ANDROID_APP_PREVIEWER_PLUGIN_API void xp_scroll_end(xp_animation_controller* animations);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_set_render_offset_x(xp_element* element, float value);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_animate_render_offset_x(
    xp_element* element,
    xp_animation_controller* animations,
    float value,
    int duration_milliseconds);
ANDROID_APP_PREVIEWER_PLUGIN_API const char* xp_element_id(const xp_element* element);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_handle_tap(
    xp_element* element,
    xp_animation_controller* animations);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_handle_pointer_down(
    xp_element* element,
    xp_animation_controller* animations);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_handle_pointer_up(
    xp_element* element,
    xp_animation_controller* animations);
ANDROID_APP_PREVIEWER_PLUGIN_API xp_animation_controller* xp_create_animation_controller(void);
ANDROID_APP_PREVIEWER_PLUGIN_API void xp_destroy_animation_controller(
    xp_animation_controller* animations);
ANDROID_APP_PREVIEWER_PLUGIN_API xp_interaction_controller* xp_create_interaction_controller(void);
ANDROID_APP_PREVIEWER_PLUGIN_API void xp_destroy_interaction_controller(
    xp_interaction_controller* controller);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_interaction_pointer_down(
    xp_interaction_controller* controller,
    xp_element* root,
    xp_animation_controller* animations,
    float x,
    float y);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_interaction_pointer_move(
    xp_interaction_controller* controller,
    float x,
    float y);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_interaction_pointer_up(
    xp_interaction_controller* controller,
    xp_element* root,
    xp_animation_controller* animations,
    float x,
    float y,
    xp_interaction_result* result);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_interaction_scroll_wheel(
    xp_interaction_controller* controller,
    xp_element* root,
    float x,
    float y,
    float horizontal_delta,
    float vertical_delta);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_interaction_update(xp_interaction_controller* controller);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_set_animation_playback_rate(
    xp_animation_controller* animations,
    float playbackRate);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_update_animations(xp_animation_controller* animations);
ANDROID_APP_PREVIEWER_PLUGIN_API xp_angle_surface* xp_create_angle_surface(
    int width,
    int height,
    const char* fontPath,
    const char* resourceRoot);
ANDROID_APP_PREVIEWER_PLUGIN_API void xp_destroy_angle_surface(xp_angle_surface* surface);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_render_angle_surface(
    xp_angle_surface* surface,
    const xp_element* root,
    unsigned char* destination,
    int destinationStride,
    int destinationCapacity);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_render(
    const xp_element* root,
    xp_command* destination,
    int capacity);
ANDROID_APP_PREVIEWER_PLUGIN_API int xp_render_angle(
    const xp_element* root,
    const char* fontPath,
    int width,
    int height,
    const char* resourceRoot,
    unsigned char* destination,
    int destinationStride,
    int destinationCapacity);


#ifdef __cplusplus
} // extern "C"
} // namespace AndroidAppPreviewerPluginSDK
#endif