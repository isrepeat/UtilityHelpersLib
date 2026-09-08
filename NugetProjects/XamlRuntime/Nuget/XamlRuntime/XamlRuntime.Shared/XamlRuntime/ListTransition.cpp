#include "XamlRuntime/ListTransition.h"

#include <algorithm>

namespace xaml::_details {
    void CollectListItems(Element& element, std::string_view itemId, std::vector<ListItemSnapshot>& result) {
        if (element.Id() == itemId) {
            result.push_back({&element, element.DataContext(), element.Bounds()});
        }
        for (const auto& child : element.Children()) {
            CollectListItems(*child, itemId, result);
        }
    }

    Element* FindElement(Element& element, std::string_view id) {
        if (element.Id() == id) {
            return &element;
        }
        for (const auto& child : element.Children()) {
            if (Element* const found = FindElement(*child, id)) {
                return found;
            }
        }
        return nullptr;
    }

    Element* FindParentListItem(Element& source) {
        Element* item = &source;
        for (Element* parent = source.Parent(); parent != nullptr; parent = parent->Parent()) {
            if (parent->Type() == ElementType::listView) {
                return item;
            }
            item = parent;
        }
        return nullptr;
    }

    Element* FindParentScrollViewer(Element& source) {
        for (Element* parent = source.Parent(); parent != nullptr; parent = parent->Parent()) {
            if (parent->Type() == ElementType::scrollViewer) {
                return parent;
            }
        }
        return nullptr;
    }

    Element* FindListItem(Element& element, const void* dataContext) {
        if (element.Type() == ElementType::listView) {
            for (const auto& item : element.Children()) {
                if (item->DataContext() == dataContext) {
                    return item.get();
                }
            }
        }
        for (const auto& child : element.Children()) {
            if (Element* const item = xaml::_details::FindListItem(*child, dataContext)) {
                return item;
            }
        }
        return nullptr;
    }
}

namespace xaml {
    std::vector<ListItemSnapshot> CaptureListItems(Element& root, std::string_view itemId) {
        std::vector<ListItemSnapshot> result;
        _details::CollectListItems(root, itemId, result);
        return result;
    }

    ScrollOffsetSnapshot CaptureScrollOffsets(Element& root, std::string_view scrollViewerId) {
        Element* const scrollViewer = _details::FindElement(root, scrollViewerId);
        if (scrollViewer == nullptr || scrollViewer->Type() != ElementType::scrollViewer) {
            return {};
        }
        return {scrollViewer->HorizontalOffset(), scrollViewer->VerticalOffset(), true};
    }

    void RestoreScrollOffsets(Element& root, std::string_view scrollViewerId, const ScrollOffsetSnapshot& offsets) {
        if (!offsets.isPresent) {
            return;
        }
        Element* const scrollViewer = _details::FindElement(root, scrollViewerId);
        if (scrollViewer == nullptr || scrollViewer->Type() != ElementType::scrollViewer) {
            return;
        }
        scrollViewer->SetHorizontalOffset(offsets.horizontal);
        scrollViewer->SetVerticalOffset(offsets.vertical);
    }

    void AnimateListRemoval(
        Element& root,
        std::string_view itemId,
        size_t removedIndex,
        const std::vector<ListItemSnapshot>& previousItems,
        AnimationController& animations,
        std::chrono::milliseconds duration) {
        if (removedIndex >= previousItems.size()) {
            return;
        }
        const std::vector<ListItemSnapshot> currentItems = CaptureListItems(root, itemId);
        const size_t count = std::min(currentItems.size(), previousItems.size() - removedIndex - 1);
        for (size_t index = 0; index < count; ++index) {
            Element& current = *currentItems[removedIndex + index].element;
            const float offsetY = previousItems[removedIndex + index + 1].bounds.y - current.Bounds().y;
            current.SetRenderOffsetY(offsetY);
            animations.Animate(current, AnimatedProperty::renderOffsetY, offsetY, 0.0f, duration);
        }
    }

    Element* FindListItem(Element& root, const void* dataContext) {
        return _details::FindListItem(root, dataContext);
    }

    ListRemovalTransition CaptureListRemovalTransition(Element& source) {
        Element* const item = _details::FindParentListItem(source);
        if (item == nullptr || item->Parent() == nullptr || item->Parent()->Id().empty()) {
            return {};
        }
        Element& list = *item->Parent();
        ListRemovalTransition transition;
        transition.listId = list.Id();
        const auto& items = list.Children();
        for (size_t index = 0; index < items.size(); ++index) {
            transition.previousBounds.push_back(items[index]->Bounds());
            if (items[index].get() == item) {
                transition.removedIndex = index;
                transition.isPresent = true;
            }
        }
        if (!transition.isPresent) {
            return {};
        }
        if (Element* const scrollViewer = _details::FindParentScrollViewer(list);
            scrollViewer != nullptr && !scrollViewer->Id().empty()) {
            transition.scrollViewerId = scrollViewer->Id();
            transition.scrollOffsets = {
                scrollViewer->HorizontalOffset(),
                scrollViewer->VerticalOffset(),
                true};
            transition.scrollExtent = scrollViewer->Extent();
        }
        return transition;
    }

    void RestoreListRemovalTransitionOffsets(Element& root, const ListRemovalTransition& transition) {
        if (!transition.isPresent || transition.scrollViewerId.empty()) {
            return;
        }
        Element* const scrollViewer = _details::FindElement(root, transition.scrollViewerId);
        if (scrollViewer != nullptr && scrollViewer->Type() == ElementType::scrollViewer) {
            scrollViewer->HoldScrollExtent(transition.scrollExtent);
        }
        RestoreScrollOffsets(root, transition.scrollViewerId, transition.scrollOffsets);
        const Rect rootBounds = root.Bounds();
        if (rootBounds.width > 0.0f && rootBounds.height > 0.0f) {
            layout(root, {rootBounds.width, rootBounds.height});
        }
    }

    void AnimateListRemovalTransition(
        Element& root,
        const ListRemovalTransition& transition,
        AnimationController& animations,
        std::chrono::milliseconds duration) {
        if (!transition.isPresent || transition.listId.empty()
            || transition.removedIndex >= transition.previousBounds.size()) {
            return;
        }
        Element* const list = _details::FindElement(root, transition.listId);
        if (list == nullptr || list->Type() != ElementType::listView) {
            return;
        }
        const auto& currentItems = list->Children();
        if (!transition.scrollViewerId.empty()) {
            if (Element* const scrollViewer = _details::FindElement(root, transition.scrollViewerId);
                scrollViewer != nullptr && scrollViewer->Type() == ElementType::scrollViewer) {
                animations.ReleaseScrollExtentAfter(*scrollViewer, duration);
            }
        }
        const size_t count = std::min(
            currentItems.size(),
            transition.previousBounds.size() - transition.removedIndex - 1);
        for (size_t index = 0; index < count; ++index) {
            Element& current = *currentItems[transition.removedIndex + index];
            const float offsetY = transition.previousBounds[transition.removedIndex + index + 1].y
                - current.Bounds().y;
            current.SetRenderOffsetY(offsetY);
            animations.Animate(current, AnimatedProperty::renderOffsetY, offsetY, 0.0f, duration);
        }
    }
}