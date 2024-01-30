#pragma once
#include <Helpers/common.h>
#include <Helpers/Logger.h>
#include <Helpers/Thread.h>
#include <coroutine>
#include <queue>

// Don't forget return original definitions for these macros at the end of file
//#define LOG_FUNCTION_ENTER_VERBOSE(fmt, ...)
//#define LOG_FUNCTION_SCOPE_VERBOSE(fmt, ...)

namespace std {
    template<typename T>
    struct coroutine_traits<std::shared_ptr<T>> {
        using promise_type = typename T::promise_type;
    };

    template<typename T, typename Caller>
    struct coroutine_traits<std::shared_ptr<T>, Caller> {
        using promise_type = typename T::promise_type;
    };
}

namespace HELPERS_NS {
    namespace Async {
        struct initial_suspend_never {
            auto initial_suspend() const noexcept {
                LOG_FUNCTION_SCOPE_VERBOSE("initial_suspend()");
                return std::suspend_never{}; // execute co-function body and return control to caller after first "suspend point"
            }
        };
        struct initial_suspend_always {
            auto initial_suspend() const noexcept {
                LOG_FUNCTION_SCOPE_VERBOSE("initial_suspend()");
                return std::suspend_always{}; // return control to caller immediatly (do not execute co-function body)
            }
        };

        // Base (universal) class that encapsulate logic to work with coroutine_handle:
        // * InitialSuspendBaseT must implement initial_suspend() method
        template<typename InitialSuspendBaseT>
        class CoTask {
        public:
            struct Promise : public InitialSuspendBaseT {
                using ObjectRet_t = std::shared_ptr<CoTask>;

                Promise() {
                    LOG_FUNCTION_ENTER_VERBOSE("Promise()");
                }
                ~Promise() {
                    LOG_FUNCTION_ENTER_VERBOSE("~Promise()");
                }
                ObjectRet_t get_return_object() noexcept {
                    LOG_FUNCTION_SCOPE_VERBOSE("get_return_object()");
                    auto coTaskShared = std::make_shared<CoTask>(std::coroutine_handle<Promise>::from_promise(*this), token);
                    coTaskWeak = coTaskShared;
                    return coTaskShared;
                }
                auto initial_suspend() const noexcept {
                    LOG_FUNCTION_SCOPE_VERBOSE("initial_suspend()");
                    return this->InitialSuspendBaseT::initial_suspend();
                }
                void unhandled_exception() {
                    LOG_FUNCTION_SCOPE_VERBOSE("unhandled_exception()");
                }
                auto final_suspend() const noexcept {
                    LOG_FUNCTION_SCOPE_VERBOSE("final_suspend()");
                    // "suspend always" at finish mean that we need to destroy coroutine_handle manually,
                    //  so return suspend_always to avoid double destroying problem in CoTask RAII class.
                    return std::suspend_always{};
                }
                void return_void() {
                    LOG_FUNCTION_ENTER_VERBOSE("return_void()");
                }
                std::weak_ptr<CoTask> GetTask() {
                    return coTaskWeak;
                }
                std::weak_ptr<int> GetToken() {
                    return token;
                }
            private:
                std::weak_ptr<CoTask> coTaskWeak;
                std::shared_ptr<int> token = std::make_shared<int>();
            };

            using Ret_t = Promise::ObjectRet_t;
            using promise_type = Promise;


            CoTask() {
                LOG_FUNCTION_ENTER_VERBOSE("CoTask()");
            }
            explicit CoTask(std::coroutine_handle<> coroHandle, std::weak_ptr<int> promiseToken)
                : coroHandle{ coroHandle }
                , promiseToken{ promiseToken }
            {
                LOG_FUNCTION_ENTER_VERBOSE("CoTask(coroHandle, promiseToken)");
            }
            ~CoTask() {
                LOG_FUNCTION_ENTER_VERBOSE("~CoTask()");
                if (coroHandle) {
                    coroHandle.destroy(); // after this call promise_type will destroy (if final_suspend return suspend_always)
                }
            }
            CoTask(CoTask&& other)
                : coroHandle{ other.coroHandle }
                , promiseToken{ other.promiseToken }
            {
                other.coroHandle = nullptr;
            }
            CoTask& operator=(CoTask&& other) {
                if (this != &other) {
                    coroHandle = other.coroHandle;
                    promiseToken = other.promiseToken;
                    other.coroHandle = nullptr;
                }
                return *this;
            }

            CoTask(CoTask const&) = delete;
            CoTask& operator=(CoTask const&) = delete;


            void resume() {
                LOG_FUNCTION_SCOPE_VERBOSE("resume()");

                if (canceled) {
                    LOG_WARNING_D("task canceled");
                    return;
                }
                if (promiseToken.expired()) {
                    LOG_WARNING_D("promiseToken expired!");
                    return;
                }
                if (!coroHandle) {
                    LOG_WARNING_D("coroHandle is empty!");
                    return;
                }
                coroHandle.resume();
            }

            void cancel() {
                LOG_FUNCTION_ENTER_VERBOSE("cancel()");
                canceled = true;
            }

            bool operator==(std::coroutine_handle<> otherCoroHandle) {
                return coroHandle == otherCoroHandle;
            }

        protected:
            std::coroutine_handle<> coroHandle;
            std::weak_ptr<int> promiseToken;
            std::atomic<bool> canceled = false;
        };
    } // namespace Async
} // namespace HELPERS_NS


#if !defined(DISABLE_VERBOSE_LOGGING)
#define LOG_FUNCTION_ENTER_VERBOSE(fmt, ...) LOG_FUNCTION_ENTER(fmt, __VA_ARGS__)
#define LOG_FUNCTION_SCOPE_VERBOSE(fmt, ...) LOG_FUNCTION_SCOPE(fmt, __VA_ARGS__)
#else
#define LOG_FUNCTION_ENTER_VERBOSE(fmt, ...)
#define LOG_FUNCTION_SCOPE_VERBOSE(fmt, ...)
#endif