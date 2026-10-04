#include "SessionApi.h"

#include <Helpers.Logging/Logging.h>
#include <XamlRuntime/Input.h>

#include "../Bridge/PreviewPluginSdkTypes.h"
#include "../Bridge/ElementTree.h"
#include "../Bridge/Diagnostic.h"
#include "../Bridge/TextBuffer.h"
#include "./PreviewPluginApi.h"

#include <string_view>
#include <stdexcept>
#include <cstring>
#include <string>

namespace {{application}}::preview::api {
    using namespace AndroidAppPreviewerPluginSDK;

    //
    // API
    //
    xp_session* SessionApi::xp_create_session(
        int width,
        int height
    ) {
        try {
            {{application}}::preview::bridge::LastError().clear();
            return {{application}}::preview::api::PreviewPluginApi::CreateSession(width, height);
        } catch (const std::exception& error) {
            {{application}}::preview::bridge::LastError() = error.what();
            return nullptr;
        }
    }

    void SessionApi::xp_destroy_session(xp_session* session) {
        {{application}}::preview::api::PreviewPluginApi::DestroySession(session);
    }

    int SessionApi::xp_session_load_page(
        xp_session* session,
        const char* page
    ) {
        try {
            {{application}}::preview::bridge::LastError().clear();
            if (session == nullptr) {
                throw std::invalid_argument("Session is required");
            }
            return LoadPage(*session, page) ? 1 : 0;
        } catch (const std::exception& error) {
            {{application}}::preview::bridge::LastError() = error.what();
            return 0;
        }
    }

    int SessionApi::xp_session_current_page(
        xp_session* session,
        char* page,
        int capacity
    ) {
        try {
            {{application}}::preview::bridge::LastError().clear();
            if (session == nullptr || page == nullptr || capacity <= 0) {
                throw std::invalid_argument("Session, page buffer and positive capacity are required");
            }
            const std::string_view name = session->value.CurrentPage();
            {{application}}::preview::bridge::TextBuffer::Write(
                name,
                page,
                static_cast<size_t>(capacity),
                "Page buffer is too small");
            return 1;
        } catch (const std::exception& error) {
            {{application}}::preview::bridge::LastError() = error.what();
            return 0;
        }
    }

    int SessionApi::xp_session_is_transitioning(xp_session* session) {
        if (session == nullptr) {
            return 0;
        }
        return 0;
    }

    int SessionApi::xp_session_navigate_preview_route(
        xp_session* session,
        const char* target
    ) {
        try {
            bridge::LastError().clear();
            if (session == nullptr || target == nullptr) {
                throw std::invalid_argument("Session and page are required");
            }
            return LoadPage(*session, target) ? 1 : 0;
        } catch (const std::exception& error) {
            bridge::LastError() = error.what();
            return 0;
        }
    }

    int SessionApi::xp_session_navigate_preview_route_path(
        xp_session* session,
        const char* path
    ) {
        try {
            bridge::LastError().clear();
            if (session == nullptr || path == nullptr) {
                throw std::invalid_argument("Session and page are required");
            }
            return LoadPage(*session, path) ? 1 : 0;
        } catch (const std::exception& error) {
            bridge::LastError() = error.what();
            return 0;
        }
    }

    int SessionApi::xp_session_preview_route_graph(
        xp_session* session,
        char* graph,
        int capacity
    ) {
        try {
            {{application}}::preview::bridge::LastError().clear();
            if (session == nullptr || graph == nullptr || capacity <= 0) {
                throw std::invalid_argument("Session, graph buffer and positive capacity are required");
            }
            const std::string value = session->value.Navigation().BuildGraphJson();
            {{application}}::preview::bridge::TextBuffer::Write(
                value,
                graph,
                static_cast<size_t>(capacity),
                "Preview route graph buffer is too small");
            return 1;
        } catch (const std::exception& error) {
            {{application}}::preview::bridge::LastError() = error.what();
            return 0;
        }
    }

    int SessionApi::xp_session_preview_page_title(
        xp_session* session,
        const char* page,
        char* title,
        int capacity
    ) {
        try {
            {{application}}::preview::bridge::LastError().clear();
            if (session == nullptr || page == nullptr || title == nullptr || capacity <= 0) {
                throw std::invalid_argument("Session, page, title buffer and positive capacity are required");
            }
            const std::string_view value = session->value.PageTitle(page);
            if (value.empty()) {
                throw std::invalid_argument("Unknown template page");
            }
            {{application}}::preview::bridge::TextBuffer::Write(
                value,
                title,
                static_cast<size_t>(capacity),
                "Preview page title buffer is too small");
            return 1;
        } catch (const std::exception& error) {
            {{application}}::preview::bridge::LastError() = error.what();
            return 0;
        }
    }

    int SessionApi::xp_session_apply_preview_scenario(
        xp_session* session,
        const char* page,
        const char* json
    ) {
        try {
            {{application}}::preview::bridge::LastError().clear();
            if (session == nullptr) {
                throw std::invalid_argument("Session is required");
            }
            return ApplyScenario(*session, page, json) ? 1 : 0;
        } catch (const std::exception& error) {
            {{application}}::preview::bridge::LastError() = error.what();
            return 0;
        }
    }

    int SessionApi::xp_session_export_preview_state(xp_session* session) {
        bridge::LastError() = "The template does not define persistent state";
        return 0;
    }

    int SessionApi::xp_session_can_save_preview_state(xp_session* session) {
        bridge::LastError().clear();
        return 0;
    }

    int SessionApi::xp_session_reload_markup(
        xp_session* session,
        const char* page,
        const char* markup,
        const char* sourcePath
    ) {
        try {
            {{application}}::preview::bridge::LastError().clear();
            if (session == nullptr) {
                throw std::invalid_argument("Session is required");
            }
            return ReloadMarkup(*session, page, markup, sourcePath) ? 1 : 0;
        } catch (const std::exception& error) {
            {{application}}::preview::bridge::LastError() = error.what();
            return 0;
        }
    }

    int SessionApi::xp_session_resize(
        xp_session* session,
        int width,
        int height
    ) {
        try {
            {{application}}::preview::bridge::LastError().clear();
            if (session == nullptr || width <= 0 || height <= 0) {
                throw std::invalid_argument("Session and positive dimensions are required");
            }
            ClearInspectionWireframe(*session);
            session->value.Resize(width, height);
            return 1;
        } catch (const std::exception& error) {
            {{application}}::preview::bridge::LastError() = error.what();
            return 0;
        }
    }

    int SessionApi::xp_session_set_animation_playback_rate(
        xp_session* session,
        float value
    ) {
        try {
            {{application}}::preview::bridge::LastError().clear();
            if (session == nullptr) {
                throw std::invalid_argument("Session is required");
            }
            session->value.SetAnimationPlaybackRate(value);
            return 1;
        } catch (const std::exception& error) {
            {{application}}::preview::bridge::LastError() = error.what();
            return 0;
        }
    }

    int SessionApi::xp_session_set_status(
        xp_session* session,
        const char* value
    ) {
        bridge::LastError() = "The template does not define a status model";
        return 0;
    }

    int SessionApi::xp_session_pointer_down(
        xp_session* session,
        float x,
        float y
    ) {
        if (session == nullptr) {
            return 0;
        }
        session->value.PointerDown(x, y);
        return 1;
    }

    int SessionApi::xp_session_pointer_move(
        xp_session* session,
        float x,
        float y
    ) {
        if (session == nullptr) {
            return 0;
        }
        session->value.PointerMove(x, y);
        return 1;
    }

    int SessionApi::xp_session_pointer_up(
        xp_session* session,
        float x,
        float y
    ) {
        if (session == nullptr) {
            return 0;
        }
        session->value.PointerUp(x, y);
        return 1;
    }

    int SessionApi::xp_session_pointer_cancel(xp_session* session) {
        if (session == nullptr) {
            return 0;
        }
        session->value.CancelPointer();
        return 1;
    }

    int SessionApi::xp_session_cursor_kind(
        xp_session* session,
        float x,
        float y
    ) {
        if (session == nullptr) {
            return 0;
        }
        xaml::Element& root = session->value.Root();
        xaml::Element* const visual = xaml::HitTestVisual(root, x, y);
        if (visual == nullptr) {
            return 0;
        }
        xaml::Element* const interactive = xaml::HitTest(root, x, y);
        if (interactive != nullptr && interactive->Type() != xaml::ElementType::scrollViewer) {
            return 1;
        }
        for (xaml::Element* element = visual; element != nullptr; element = element->Parent()) {
            if (element->Type() == xaml::ElementType::scrollViewer) {
                return 2;
            }
        }
        return 0;
    }

    int SessionApi::xp_session_inspect(
        xp_session* session,
        float x,
        float y,
        xp_session_inspection_result* result
    ) {
        if (session == nullptr || result == nullptr) {
            return 0;
        }
        return Inspect(*session, x, y, *result) ? 1 : 0;
    }

    int SessionApi::xp_session_set_inspection_wireframe(
        xp_session* session,
        float thickness,
        int lineStyle,
        xp_color color,
        xp_color marginColor,
        xp_color paddingColor
    ) {
        if (session == nullptr) {
            return 0;
        }
        return SetInspectionWireframe(
            *session,
            thickness,
            lineStyle,
            color,
            marginColor,
            paddingColor) ? 1 : 0;
    }

    int SessionApi::xp_session_set_selected_wireframe(
        xp_session* session,
        float thickness,
        int lineStyle,
        xp_color color,
        xp_color marginColor,
        xp_color paddingColor
    ) {
        if (session == nullptr || thickness <= 0.0f || (lineStyle != 0 && lineStyle != 1)) {
            return 0;
        }
        session->selectedWireframe = {
            thickness,
            lineStyle == 0 ? xaml::attr::WireframeLineStyle::solid : xaml::attr::WireframeLineStyle::dashed,
            {color.red, color.green, color.blue, color.alpha},
            {marginColor.red, marginColor.green, marginColor.blue, marginColor.alpha},
            {paddingColor.red, paddingColor.green, paddingColor.blue, paddingColor.alpha},
        };
        if (session->selectedElement != nullptr) {
            if (session->selectedElementLifetime.expired()) {
                ClearSelectedWireframe(*session);
            } else {
                session->selectedElement->SetSelectedWireframe(session->selectedWireframe);
            }
        }
        return 1;
    }

    int SessionApi::xp_session_clear_inspection_wireframe(xp_session* session) {
        if (session == nullptr) {
            return 0;
        }
        ClearInspectionWireframe(*session);
        return 1;
    }

    int SessionApi::xp_session_clear_selected_inspection_element(xp_session* session) {
        if (session == nullptr) {
            return 0;
        }
        ClearSelectedWireframe(*session);
        return 1;
    }

    int SessionApi::xp_session_select_inspection_element(
        xp_session* session,
        const char* sourcePath,
        int line,
        int column
    ) {
        if (session == nullptr || sourcePath == nullptr || line <= 0 || column <= 0) {
            return 0;
        }
        LOG_INFO(
            "AndroidAppPreviewer.Inspection",
            "Source selection requested at {}:{} in {}",
            line,
            column,
            sourcePath);
        xaml::Element* const element = {{application}}::preview::bridge::ElementTree::FindElementAtSource(
            session->value.Root(),
            sourcePath,
            line,
            column);
        if (element == nullptr) {
            LOG_INFO("AndroidAppPreviewer.Inspection", "Source selection found no element");
            return 0;
        }
        // Не стираем текущий выбор, пока новая позиция редактора не сопоставлена
        // с элементом preview: перевод фокуса после клика меняет caret.
        ClearSelectedWireframe(*session);
        SetSelectedWireframe(*session, *element);
        const xaml::Rect& bounds = element->Bounds();
        LOG_INFO(
            "AndroidAppPreviewer.Inspection",
            "Source selection resolved to {}:{} id='{}' bounds=({}, {}, {}, {})",
            element->SourceLine(),
            element->SourceColumn(),
            element->Id(),
            bounds.x,
            bounds.y,
            bounds.width,
            bounds.height);
        return 1;
    }

    int SessionApi::xp_session_pin_inspection_element(xp_session* session) {
        if (session == nullptr || session->inspectionElement == nullptr) {
            return 0;
        }
        if (session->inspectionElementLifetime.expired()) {
            ClearInspectionWireframe(*session);
            return 0;
        }
        SetSelectedWireframe(*session, *session->inspectionElement);
        return 1;
    }

    int SessionApi::xp_session_update(xp_session* session) {
        if (session == nullptr) {
            return 0;
        }
        Update(*session);
        return 1;
    }

    int SessionApi::xp_session_render_angle_surface(
        xp_session* session,
        xp_angle_surface* surface,
        unsigned char* destination,
        int destinationStride,
        int destinationCapacity
    ) {
        try {
            {{application}}::preview::bridge::LastError().clear();
            if (session == nullptr || surface == nullptr) {
                throw std::invalid_argument("Session and surface are required");
            }
            return Render(
                *session,
                *surface,
                destination,
                destinationStride,
                destinationCapacity) ? 1 : 0;
        } catch (const std::exception& error) {
            {{application}}::preview::bridge::LastError() = error.what();
            return 0;
        }
    }

    //
    // Internal
    //
    bool SessionApi::LoadPage(
        xp_session& session,
        const char* page) {
        if (page == nullptr) {
            throw std::invalid_argument("Page is required");
        }
        ClearInspectionWireframe(session);
        ClearSelectedWireframe(session);
        if (!session.value.LoadPage(page)) {
            throw std::invalid_argument("Unknown {{Application}} page");
        }
        return true;
    }

    bool SessionApi::ApplyScenario(
        xp_session& session,
        const char* page,
        const char* json) {
        bridge::LastError() = "The template does not define preview scenarios";
        return 0;
    }

    bool SessionApi::ReloadMarkup(
        xp_session& session,
        const char* page,
        const char* markup,
        const char* sourcePath) {
        if (page == nullptr || markup == nullptr || sourcePath == nullptr) {
            throw std::invalid_argument("Page, markup and source path are required");
        }
        ClearInspectionWireframe(session);
        ClearSelectedWireframe(session);
        return session.value.ReloadMarkup(page, markup, sourcePath, {{application}}::preview::bridge::LastError());
    }

    bool SessionApi::Inspect(
        xp_session& session,
        float x,
        float y,
        xp_session_inspection_result& result) {
        xaml::Element* element = {{application}}::preview::bridge::ElementTree::HitTestVisual(
            session.value.Root(), x, y);
        while (element != nullptr && element->SourceLine() <= 0) {
            element = element->Parent();
        }
        if (element == nullptr) {
            ClearInspectionWireframe(session);
            return false;
        }
        SetInspectionWireframe(session, *element);
        const xaml::Rect bounds = element->Bounds();
        result = { element->SourceLine(), element->SourceColumn() };
        std::strncpy(result.sourcePath, element->SourcePath().c_str(), sizeof(result.sourcePath) - 1);
        result.sourcePath[sizeof(result.sourcePath) - 1] = '\0';
        result.bounds = { bounds.x, bounds.y, bounds.width, bounds.height };
        return true;
    }

    bool SessionApi::SetInspectionWireframe(
        xp_session& session,
        float thickness,
        int lineStyle,
        xp_color color,
        xp_color marginColor,
        xp_color paddingColor) {
        if (thickness <= 0.0f || (lineStyle != 0 && lineStyle != 1)) {
            return false;
        }
        session.inspectionWireframe = {
            thickness,
            lineStyle == 0 ? xaml::attr::WireframeLineStyle::solid : xaml::attr::WireframeLineStyle::dashed,
            {color.red, color.green, color.blue, color.alpha},
            {marginColor.red, marginColor.green, marginColor.blue, marginColor.alpha},
            {paddingColor.red, paddingColor.green, paddingColor.blue, paddingColor.alpha},
        };
        if (session.inspectionElement != nullptr) {
            if (session.inspectionElementLifetime.expired()) {
                ClearInspectionWireframe(session);
            }
            else {
                session.inspectionElement->SetInspectionWireframe(session.inspectionWireframe);
            }
        }
        return true;
    }

    bool SessionApi::Update(xp_session& session) {
        return session.value.Update();
    }

    bool SessionApi::Render(
        xp_session& session,
        xp_angle_surface& surface,
        unsigned char* destination,
        int destinationStride,
        int destinationCapacity) {
        if (destination == nullptr || destinationStride < surface.width * 4
            || destinationCapacity / destinationStride < surface.height) {
            throw std::invalid_argument("Invalid {{Application}} ANGLE render arguments");
        }
        surface.value.Render(session.value.Root(), destination, destinationStride);
        return true;
    }

    void SessionApi::ClearInspectionWireframe(xp_session& session) {
        if (session.inspectionElement != nullptr && !session.inspectionElementLifetime.expired()) {
            session.inspectionElement->ClearInspectionWireframe();
        }
        session.inspectionElement = nullptr;
        session.inspectionElementLifetime.reset();
    }

    void SessionApi::ClearSelectedWireframe(xp_session& session) {
        if (session.selectedElement != nullptr && !session.selectedElementLifetime.expired()) {
            session.selectedElement->ClearSelectedWireframe();
        }
        session.selectedElement = nullptr;
        session.selectedElementLifetime.reset();
    }

    void SessionApi::SetInspectionWireframe(
        xp_session& session,
        xaml::Element& element) {
        if (session.inspectionElement != &element) {
            ClearInspectionWireframe(session);
            session.inspectionElement = &element;
            session.inspectionElementLifetime = element.LifetimeToken();
        }
        element.SetInspectionWireframe(session.inspectionWireframe);
    }

    void SessionApi::SetSelectedWireframe(
        xp_session& session,
        xaml::Element& element) {
        if (session.selectedElement != &element) {
            ClearSelectedWireframe(session);
            session.selectedElement = &element;
            session.selectedElementLifetime = element.LifetimeToken();
        }
        element.SetSelectedWireframe(session.selectedWireframe);
    }
} // namespace {{application}}::preview::api