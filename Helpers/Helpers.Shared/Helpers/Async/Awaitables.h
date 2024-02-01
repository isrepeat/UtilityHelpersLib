#pragma once
#include <Helpers/common.h>
#include <Helpers/Logger.h>
#include <Helpers/Signal.h>
#include <Helpers/Time.h>
#include "CoTask.h"
#include <coroutine>
#include <memory>

// Don't forget return original definitions for these macros at the end of file
#define LOG_FUNCTION_ENTER_VERBOSE(fmt, ...)
#define LOG_FUNCTION_SCOPE_VERBOSE(fmt, ...)
#define LOG_FUNCTION_ENTER_VERBOSE_C(fmt, ...)
#define LOG_FUNCTION_SCOPE_VERBOSE_C(fmt, ...)

namespace HELPERS_NS {
    namespace Async {

        // TODO: Add type_traits to detect check if caller promise_type does not have Ctor(..., resumeCallback)
        // NOTE: Must be called from coroutine that accept resumeCallback in params.
        //       (resumeCallback will passed to this task promise_type Ctor)
        template <typename PromiseT>
        auto ResumeAfter(std::chrono::milliseconds duration) noexcept {
            LOG_FUNCTION_SCOPE_VERBOSE("ResumeAfter(duration, resumeCallback)");

            struct Awaitable {
                explicit Awaitable(std::chrono::milliseconds duration)
                    : duration{ duration }
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

                void await_suspend(std::coroutine_handle<PromiseT> callerPromiseCoroHandle) {
                    LOG_FUNCTION_SCOPE_VERBOSE("await_suspend(coroHandle)");

                    std::weak_ptr<CoTaskBase> coTaskWeak = callerPromiseCoroHandle.promise().get_task();
                    auto resumeCallback = callerPromiseCoroHandle.promise().get_resume_callback();
#ifdef _DEBUG
                    if (auto coTask = coTaskWeak.lock()) {
                        assert(*coTask == callerPromiseCoroHandle);
                    }
#endif
                    if (LOG_ASSERT(resumeCallback, "resumeCallback is empty!")) {
                        LOG_WARNING_D("resume callerPromiseCoroHandle ...");
                        callerPromiseCoroHandle.resume();
                        LOG_DEBUG_D("callerPromiseCoroHandle finished");
                        return;
                    }

                    HELPERS_NS::Timer::Once(duration, [resumeCallback, coTaskWeak] {
                        LOG_FUNCTION_SCOPE_VERBOSE("Timer::Once__lambda()");
                        resumeCallback(coTaskWeak);
                        });
                }

                void await_resume() noexcept {
                    LOG_FUNCTION_SCOPE_VERBOSE("await_resume()");
                }

            private:
                std::chrono::system_clock::duration duration;
            };

            return Awaitable{ duration };
        }
    } // namespace Async
} // namespace HELPERS_NS


// NOTE: Do not encapsulate overloaded global operator co_await in namespace by design to simplify use
// TODO: Add guards for expired promise / task
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

#define LOG_FUNCTION_ENTER_VERBOSE_C(fmt, ...) LOG_FUNCTION_ENTER_C(fmt, __VA_ARGS__)
#define LOG_FUNCTION_SCOPE_VERBOSE_C(fmt, ...) LOG_FUNCTION_SCOPE_C(fmt, __VA_ARGS__)
#else
#define LOG_FUNCTION_ENTER_VERBOSE(fmt, ...)
#define LOG_FUNCTION_SCOPE_VERBOSE(fmt, ...)

#define LOG_FUNCTION_ENTER_VERBOSE_C(fmt, ...)
#define LOG_FUNCTION_ENTER_VERBOSE_C(fmt, ...)
#endif