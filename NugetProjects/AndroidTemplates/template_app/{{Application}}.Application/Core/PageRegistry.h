#pragma once
#include "../Interface/IPageNavigator.h"
#include "../Interface/IPage.h"
#include "../Model/ApplicationRepository.h"
#include "./AppSessionController.h"

#include <memory>
#include <tuple>

namespace {{application}}::application::core {
    struct PageContext final {
        interface::IPageNavigator& navigator;
        model::ApplicationRepository& repository;
        interface::IHostCommandDispatcher& hostCommands;
        const AppSessionController& controller;
    };

    template <typename... TPages>
    class PageRegistry final {
    public:
        explicit PageRegistry(PageContext& context)
            : pages(std::make_unique<TPages>(context)...) {
        }

        template <typename THandler>
        void ForEach(THandler&& handler) const {
            std::apply([&handler](const auto&... pages) {
                (handler(*pages), ...);
            }, this->pages);
        }

        interface::IPage* Find(std::string_view name) const {
            interface::IPage* result = nullptr;
            this->ForEach([&](interface::IPage& page) {
                if (page.Name() == name) {
                    result = &page;
                }
            });
            return result;
        }

    private:
        std::tuple<std::unique_ptr<TPages>...> pages;
    };
}