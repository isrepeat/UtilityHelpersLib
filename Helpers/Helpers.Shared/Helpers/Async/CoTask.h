#pragma once
#include <Helpers/common.h>
#include <Helpers/Logger.h>
#include <Helpers/Thread.h>
#include <coroutine>
#include <queue>

// Don't forget return original definitions for these macros at the end of file
#define LOG_FUNCTION_ENTER_VERBOSE(fmt, ...)
#define LOG_FUNCTION_SCOPE_VERBOSE(fmt, ...)
#define LOG_FUNCTION_ENTER_VERBOSE_C(fmt, ...)
#define LOG_FUNCTION_SCOPE_VERBOSE_C(fmt, ...)

namespace std {
    template<typename T, typename... Args>
    struct coroutine_traits<std::shared_ptr<T>, Args...> {
        using promise_type = typename T::promise_type;
    };

    template<typename T, typename Caller, typename... Args>
    struct coroutine_traits<std::shared_ptr<T>, Caller*, Args...> {
        using promise_type = typename T::promise_type;
    };
}

namespace HELPERS_NS {
    namespace Async {
        struct suspend {
            static constexpr bool call_await_suspend = false; // std::suspend_always
            static constexpr bool resume_immediately = true; // std::suspend_never
        };

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


        /*------------------------*/
        /*      PROMISE BASE      */
        /*------------------------*/
        class CoTaskBase;

        template <typename PromiseImplT>
        class CoTask;

        template <typename PromiseImplT>
        class Promise {
            CLASS_FULLNAME_LOGGING_INLINE_IMPLEMENTATION(Promise);
        public:
            using ObjectRet_t = std::shared_ptr<CoTask<PromiseImplT>>;

            struct FinalAwaiter {
                bool await_ready() noexcept {
                    return suspend::call_await_suspend;
                }
                // NOTE: promiseCoroHandle associated with current co-function context where was created Promise
                void await_suspend(std::coroutine_handle<PromiseImplT> promiseCoroHandle) noexcept {
                    if (promiseCoroHandle.promise().continuation) {
                        promiseCoroHandle.promise().continuation.resume();
                    }
                }

                void await_resume() noexcept {
                    std::terminate();
                }
            };

            // Used for non-class functions
            Promise(std::wstring instanceName, std::function<void(std::weak_ptr<CoTaskBase>)> resumeCallback)
                : resumeCallback{ resumeCallback }
            {
                this->SetFullClassName(instanceName);
                LOG_FUNCTION_ENTER_VERBOSE_C(L"Promise(instanceName, resumeCallback)");
            }

            // Used for Caller class methods
            template <typename Caller>
            Promise(Caller&, std::wstring instanceName, std::function<void(std::weak_ptr<CoTaskBase>)> resumeCallback)
                : resumeCallback{ resumeCallback }
            {
                this->SetFullClassName(instanceName);
                LOG_FUNCTION_ENTER_VERBOSE_C(L"Promise(Caller, instanceName, resumeCallback)");
            }

            ~Promise() {
                LOG_FUNCTION_ENTER_VERBOSE_C("~Promise()");
            }

            ObjectRet_t get_return_object() noexcept {
                LOG_FUNCTION_SCOPE_VERBOSE_C("get_return_object()");
                auto coTaskShared = std::make_shared<CoTask<PromiseImplT>>(
                    std::coroutine_handle<PromiseImplT>::from_promise(static_cast<PromiseImplT&>(*this)),
                    token,
                    this->GetFullClassNameW()
                    );
                coTaskWeak = coTaskShared;
                return coTaskShared;
            }
            auto initial_suspend() const noexcept {
                LOG_FUNCTION_SCOPE_VERBOSE_C("initial_suspend()");
                return std::suspend_always{};
            }
            void unhandled_exception() {
                LOG_FUNCTION_SCOPE_VERBOSE_C("unhandled_exception()");
            }
            auto final_suspend() const noexcept {
                LOG_FUNCTION_SCOPE_VERBOSE_C("final_suspend()");
                // NOTE: "suspend always" at finish mean that we need to destroy coroutine_handle manually.
                //        This is need to avoid double destroying problem in CoTask RAII class.
                return FinalAwaiter{};
            }
            void return_void() {
                LOG_FUNCTION_ENTER_VERBOSE_C("return_void()");
            }

            void set_resume_callback(std::function<void(std::weak_ptr<CoTaskBase>)> resumeCallback) {
                this->resumeCallback = resumeCallback;
            }
            void set_continuation(std::coroutine_handle<> continuation) {
                this->continuation = continuation;
            }

            std::function<void(std::weak_ptr<CoTaskBase>)> get_resume_callback() {
                return resumeCallback;
            }
            std::weak_ptr<CoTaskBase> get_task() {
                return coTaskWeak;
            }
            std::weak_ptr<int> get_token() {
                return token;
            }

        private:
            std::function<void(std::weak_ptr<CoTaskBase>)> resumeCallback;
            std::coroutine_handle<> continuation;
            std::weak_ptr<CoTaskBase> coTaskWeak;
            std::shared_ptr<int> token = std::make_shared<int>();
        };


        /*------------------------------*/
        /*   PROMISES IMPLEMENTATIONS   */
        /*------------------------------*/
        class PromiseDefault : public Promise<PromiseDefault> {
        public:
            using _MyBase = Promise<PromiseDefault>;

#ifdef __INTELLISENSE__ // https://stackoverflow.com/questions/67209981/weird-error-from-visual-c-no-default-constructor-for-promise-type
            PromiseDefault(); // Do not use default Ctor, just mark it for intelli sense
#endif
            PromiseDefault(std::wstring instanceName)
                : _MyBase(instanceName, nullptr)
            {
                LOG_FUNCTION_ENTER_VERBOSE(L"PromiseDefault(instanceName = {})", instanceName);
            }
            template <typename Caller>
            PromiseDefault(Caller& caller, std::wstring instanceName)
                : _MyBase(caller, instanceName, nullptr)
            {
                LOG_FUNCTION_ENTER_VERBOSE(L"PromiseDefault(Caller, instanceName = {})", instanceName);
            }
        };

        class PromiseRoot : public Promise<PromiseRoot> {
        public:
            using _MyBase = Promise<PromiseRoot>;

#ifdef __INTELLISENSE__ // https://stackoverflow.com/questions/67209981/weird-error-from-visual-c-no-default-constructor-for-promise-type
            PromiseRoot(); // Do not use default Ctor, just mark it for intelli sense
#endif
            PromiseRoot(std::wstring instanceName, std::function<void(std::weak_ptr<CoTaskBase>)> resumeCallback)
                :_MyBase(instanceName, resumeCallback)
            {
                LOG_FUNCTION_ENTER_VERBOSE(L"PromiseRoot(instanceName = {}, resumeCallback)", instanceName);
            }
            template <typename Caller>
            PromiseRoot(Caller& caller, std::wstring instanceName, std::function<void(std::weak_ptr<CoTaskBase>)> resumeCallback)
                : _MyBase(caller, instanceName, resumeCallback)
            {
                LOG_FUNCTION_ENTER_VERBOSE(L"Promise(Caller, instanceName = {}, resumeCallback)", instanceName);
            }
        };

        /*------------------*/
        /*   AWAITER BASE   */
        /*------------------*/
        template <typename PromiseImplT>
        class AwaiterBase { // Universal Awaiter class. You need overload await_suspend for all type of promises that can be used. 
        public:
            AwaiterBase(std::coroutine_handle<PromiseImplT> promiseCoroHandle)
                : promiseCoroHandle{ promiseCoroHandle }
            {
            }
            bool await_ready() noexcept {
                //return !promiseCoroHandle || promiseCoroHandle.done();
                return suspend::call_await_suspend;
            }
            std::coroutine_handle<PromiseImplT> await_suspend(std::coroutine_handle<PromiseDefault> callerPromiseCoroHandle) noexcept {
                return await_suspend_internal<PromiseDefault>(callerPromiseCoroHandle);
            }
            std::coroutine_handle<PromiseImplT> await_suspend(std::coroutine_handle<PromiseRoot> callerPromiseCoroHandle) noexcept {
                return await_suspend_internal<PromiseRoot>(callerPromiseCoroHandle);
            }
            void await_resume() {
                LOG_FUNCTION_ENTER_VERBOSE("await_resume()");
            }

        private:
            // NOTE: callerPromiseCoroHandle associated with outter co-function context where called operator co_await CoTask{}
            template <typename PromiseT>
            std::coroutine_handle<PromiseImplT> await_suspend_internal(std::coroutine_handle<PromiseT> callerPromiseCoroHandle) noexcept {
                LOG_FUNCTION_SCOPE_VERBOSE("await_suspend_internal(callerPromiseCoroHandle)");
                // 1. Remember callerPromiseCoroHandle (it is resumed when promiseCoroHandle finished in Promise::final_awaiter);
                // 2. Resume promiseCoroHandle;
                promiseCoroHandle.promise().set_resume_callback(callerPromiseCoroHandle.promise().get_resume_callback());
                promiseCoroHandle.promise().set_continuation(callerPromiseCoroHandle);
                return promiseCoroHandle;
                // NOTE: control returned to caller only when promiseCoroHandle finished
            }

        private:
            std::coroutine_handle<PromiseImplT> promiseCoroHandle;
        };


        /*------------------*/
        /*   CO-TASK BASE   */
        /*------------------*/
        // Base (universal) class that encapsulates logic to work with coroutine_handle
        class CoTaskBase {
            CLASS_FULLNAME_LOGGING_INLINE_IMPLEMENTATION(CoTaskBase);
        public:
            CoTaskBase() {
                LOG_FUNCTION_ENTER_VERBOSE("CoTaskBase()");
            }
            CoTaskBase(std::coroutine_handle<> promiseCoroHandle, std::weak_ptr<int> promiseToken, std::wstring instanceName)
                : promiseCoroHandle{ promiseCoroHandle }
                , promiseToken{ promiseToken }
            {
                this->SetFullClassName(instanceName);
                LOG_FUNCTION_ENTER_VERBOSE_C("CoTaskBase(promiseCoroHandle, promiseToken, ...)");
            }
            ~CoTaskBase() {
                LOG_FUNCTION_ENTER_VERBOSE_C("~CoTaskBase()");
                if (promiseCoroHandle) {
                    promiseCoroHandle.destroy(); // promise_type will be destroyed
                }
            }

            void resume() {
                LOG_FUNCTION_SCOPE_VERBOSE_C("resume()");
                if (canceled) {
                    LOG_WARNING_D("task canceled");
                    return;
                }
                if (promiseToken.expired()) {
                    LOG_WARNING_D("promiseToken expired!");
                    return;
                }
                if (!promiseCoroHandle) {
                    LOG_WARNING_D("promiseCoroHandle is empty!");
                    return;
                }
                promiseCoroHandle.resume();
            }

            void cancel() {
                LOG_FUNCTION_ENTER_VERBOSE_C("cancel()");
                canceled = true;
            }

            bool operator==(std::coroutine_handle<> otherCoroHandle) {
                return promiseCoroHandle == otherCoroHandle;
            }

        protected:
            std::coroutine_handle<> promiseCoroHandle;
            std::weak_ptr<int> promiseToken;
            std::atomic<bool> canceled = false;
        };


        /*-----------------------------*/
        /*   CO-TASK IMPLEMENTATIONS   */
        /*-----------------------------*/
        template <typename PromiseImplT>
        class CoTask : public CoTaskBase {
            CLASS_FULLNAME_LOGGING_INLINE_IMPLEMENTATION(CoTask);
        public:
            using Ret_t = typename PromiseImplT::ObjectRet_t;
            using CoHandle_t = std::coroutine_handle<PromiseImplT>;
            using promise_type = PromiseImplT;

            CoTask() {
                LOG_FUNCTION_ENTER_VERBOSE("CoTask()");
            }
            CoTask(CoHandle_t promiseCoroHandle, std::weak_ptr<int> promiseToken, std::wstring instanceName)
                : CoTaskBase(promiseCoroHandle, promiseToken, instanceName)
                , promiseCoroHandleTyped{ promiseCoroHandle }
            {
            }

            CoTask(CoTask&& other)
                : promiseCoroHandle{ other.promiseCoroHandle }
                , promiseToken{ other.promiseToken }
            {
                other.promiseCoroHandle = nullptr;
            }
            CoTask& operator=(CoTask&& other) {
                if (this != &other) {
                    if (promiseCoroHandle) {
                        promiseCoroHandle.destroy();
                    }
                    promiseCoroHandle = other.promiseCoroHandle;
                    promiseToken = other.promiseToken;
                    other.promiseCoroHandle = nullptr;
                }
                return *this;
            }

            CoTask(CoTask const&) = delete;
            CoTask& operator=(CoTask const&) = delete;

            // TODO: quilify operator with &&
            auto operator co_await() {
                return AwaiterBase<promise_type> { promiseCoroHandleTyped };
            }

        private:
            CoHandle_t promiseCoroHandleTyped;
        };
    } // namespace Async
} // namespace HELPERS_NS


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