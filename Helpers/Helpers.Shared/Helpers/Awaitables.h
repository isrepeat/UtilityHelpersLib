#pragma once
#include "common.h"
#include <Helpers/Logger.h>
#include <Helpers/Time.h>
#include <coroutine>

#define LOG_FUNCTION_ENTER_VERBOSE(fmt, ...)
#define LOG_FUNCTION_SCOPE_VERBOSE(fmt, ...)

namespace HELPERS_NS {
    namespace Async {

        template<typename Rep, typename Period>
        auto InvokeCallbackAfter(std::chrono::duration<Rep, Period> duration, std::function<void(std::coroutine_handle<>)> resumeCallback) noexcept {
            LOG_FUNCTION_SCOPE_VERBOSE("InvokeCallbackAfter(duration, resumeCallback)");

            struct Awaitable {
                explicit Awaitable(std::chrono::system_clock::duration duration, std::function<void(std::coroutine_handle<>)> resumeCallback)
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

                void await_suspend(std::coroutine_handle<> coroHandle) {
                    LOG_FUNCTION_SCOPE_VERBOSE("await_suspend(coroHandle)");

                    auto resumeCallbackCopy = resumeCallback; // mb no need, check if "this" is valid inside callback for all cases
                    HELPERS_NS::Timer::Once(duration, [coroHandle, resumeCallbackCopy] {
                        LOG_FUNCTION_SCOPE_VERBOSE("Timer::Once__lambda()");
                        resumeCallbackCopy(coroHandle);
                        });
                }

            private:
                std::chrono::system_clock::duration duration;
                std::function<void(std::coroutine_handle<>)> resumeCallback;
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
                coroHandlecoroHandle.resume();
                });
        }

    private:
        std::chrono::system_clock::duration duration;
    };

    return Awaitable{ duration };
}