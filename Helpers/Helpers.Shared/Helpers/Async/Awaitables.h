#pragma once
#include <Helpers/common.h>
#include <Helpers/Logger.h>
#include <Helpers/Time.h>
#include "CoTask.h"
#include <coroutine>
#include <memory>

// Don't forget return original definitions for these macros at the end of file
#define LOG_FUNCTION_ENTER_VERBOSE(fmt, ...)
#define LOG_FUNCTION_SCOPE_VERBOSE(fmt, ...)

namespace HELPERS_NS {
    namespace Async {
        // NOTE: CoTaskT::promise_type must implement GetTask() method that returns weak_ptr<CoTaskT>
        template <typename CoTaskT>
        auto InvokeCallbackAfter(
            std::chrono::milliseconds duration,
            std::function<void(std::weak_ptr<CoTaskT>)> resumeCallback) noexcept
        {
            LOG_FUNCTION_SCOPE_VERBOSE("InvokeCallbackAfter(duration, resumeCallback)");

            struct Awaitable {
                explicit Awaitable(
                    std::chrono::milliseconds duration,
                    std::function<void(std::weak_ptr<CoTaskT>)> resumeCallback)
                    : duration{ duration }
                    , resumeCallback{ resumeCallback }
                {
                    LOG_FUNCTION_ENTER_VERBOSE("Awaitable(duration, resumeCallback)");
                }

                ~Awaitable() {
                    LOG_FUNCTION_ENTER_VERBOSE("~Awaitable()");
                }

                bool await_ready() const noexcept {
                    LOG_FUNCTION_ENTER_VERBOSE("await_ready()");
                    return duration.count() <= 0;
                }

                void await_resume() noexcept {
                    LOG_FUNCTION_SCOPE_VERBOSE("await_resume()");
                }

                void await_suspend(std::coroutine_handle<typename CoTaskT::promise_type> coroHandle) {
                    LOG_FUNCTION_SCOPE_VERBOSE("await_suspend(coroHandle)");
                    
                    auto resumeCallbackCopy = resumeCallback; // mb no need copy, check if "this" is valid inside callback for all cases
                    std::weak_ptr<CoTaskT> coTaskWeak = coroHandle.promise().GetTask();
#ifdef _DEBUG
                    if (auto coTask = coTaskWeak.lock()) {
                        assert(*coTask == coroHandle);
                    }
#endif
                    HELPERS_NS::Timer::Once(duration, [resumeCallbackCopy, coTaskWeak] {
                        LOG_FUNCTION_SCOPE_VERBOSE("Timer::Once__lambda()");
                        resumeCallbackCopy(coTaskWeak);
                        });
                }

            private:
                std::chrono::system_clock::duration duration;
                std::function<void(std::weak_ptr<CoTaskT>)> resumeCallback;
            };

            return Awaitable{ duration, resumeCallback };
        }
    } // namespace Async
} // namespace HELPERS_NS


// NOTE: Do not encapsulate overloaded global operator co_await in namespace by design to simplify use
template<typename Rep, typename Period>
auto operator co_await(std::chrono::duration<Rep, Period> duration) noexcept {
    LOG_FUNCTION_SCOPE_VERBOSE("operator co_await(duration)");

    struct Awaitable {
        explicit Awaitable(std::chrono::system_clock::duration duration)
            : duration{ duration }
        {
            LOG_FUNCTION_ENTER_VERBOSE("Awaitable(duration)");
        }

        ~Awaitable() {
            LOG_FUNCTION_ENTER_VERBOSE("~Awaitable()");
        }

        bool await_ready() const noexcept {
            LOG_FUNCTION_ENTER_VERBOSE("await_ready()");
            return duration.count() <= 0;
        }

        void await_resume() noexcept {
            LOG_FUNCTION_SCOPE_VERBOSE("await_resume()");
        }

        void await_suspend(std::coroutine_handle<> coroHandle) {
            LOG_FUNCTION_SCOPE_VERBOSE("await_suspend(coroHandle)");

            HELPERS_NS::Timer::Once(duration, [coroHandle] {
                LOG_FUNCTION_SCOPE_VERBOSE("Timer::Once__lambda()");
                coroHandle.resume();
                });
        }

    private:
        std::chrono::system_clock::duration duration;
    };

    return Awaitable{ duration };
}

#if !defined(DISABLE_VERBOSE_LOGGING)
#define LOG_FUNCTION_ENTER_VERBOSE(fmt, ...) LOG_FUNCTION_ENTER(fmt, __VA_ARGS__)
#define LOG_FUNCTION_SCOPE_VERBOSE(fmt, ...) LOG_FUNCTION_SCOPE(fmt, __VA_ARGS__)
#else
#define LOG_FUNCTION_ENTER_VERBOSE(fmt, ...)
#define LOG_FUNCTION_SCOPE_VERBOSE(fmt, ...)
#endif