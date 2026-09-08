#pragma once

#include "XamlRuntime/Animation.h"
#include "XamlRuntime/XamlLayout.h"

#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace xaml {
    struct ListItemSnapshot {
        Element* element = nullptr;
        const void* dataContext = nullptr;
        Rect bounds;
    };

    struct ScrollOffsetSnapshot {
        float horizontal = 0.0f;
        float vertical = 0.0f;
        bool isPresent = false;
    };

    struct ListRemovalTransition {
        std::string listId;
        std::string scrollViewerId;
        std::vector<Rect> previousBounds;
        ScrollOffsetSnapshot scrollOffsets;
        Size scrollExtent;
        size_t removedIndex = 0;
        bool isPresent = false;
    };

    std::vector<ListItemSnapshot> CaptureListItems(Element& root, std::string_view itemId);
    ScrollOffsetSnapshot CaptureScrollOffsets(Element& root, std::string_view scrollViewerId);
    void RestoreScrollOffsets(Element& root, std::string_view scrollViewerId, const ScrollOffsetSnapshot& offsets);
    void AnimateListRemoval(
        Element& root,
        std::string_view itemId,
        size_t removedIndex,
        const std::vector<ListItemSnapshot>& previousItems,
        AnimationController& animations,
        std::chrono::milliseconds duration);
    Element* FindListItem(Element& root, const void* dataContext);
    ListRemovalTransition CaptureListRemovalTransition(Element& source);
    void RestoreListRemovalTransitionOffsets(Element& root, const ListRemovalTransition& transition);
    void AnimateListRemovalTransition(
        Element& root,
        const ListRemovalTransition& transition,
        AnimationController& animations,
        std::chrono::milliseconds duration);
}