#pragma once
#include <Helpers/common.h>
#include <Helpers/Logger.h>
#include <Helpers/Thread.h>
#include <Helpers/Signal.h>
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
#ifdef __INTELLISENSE__ // https://stackoverflow.com/questions/67209981/weird-error-from-visual-c-no-default-constructor-for-promise-type
    #define INTELLISENSE_DEFAULT_CTOR(className) className()
#else
    #define INTELLISENSE_DEFAULT_CTOR(className)
#endif

        struct suspend {
            static constexpr bool always = false; // call await_suspend()
            static constexpr bool never = true; // resume immediately (not call await_suspend())
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
                    return suspend::always;
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
            Promise(std::wstring instanceName) {
                this->SetFullClassName(instanceName);
                LOG_FUNCTION_ENTER_VERBOSE_C(L"Promise(instanceName)");
            }

            // Used for Caller class methods
            template <typename Caller>
            Promise(Caller&, std::wstring instanceName) {
                this->SetFullClassName(instanceName);
                LOG_FUNCTION_ENTER_VERBOSE_C(L"Promise(Caller, instanceName)");
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

            template <typename PromiseT>
            void set_continuation(std::coroutine_handle<PromiseT> otherPromise) {
                this->resumeCallback = otherPromise.promise().get_resume_callback();
                this->continuation = otherPromise;
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

        protected:
            std::function<void(std::weak_ptr<CoTaskBase>)> resumeCallback; // can be initialized in derived classes

        private:
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
            INTELLISENSE_DEFAULT_CTOR(PromiseDefault);

            PromiseDefault(std::wstring instanceName)
                : _MyBase(instanceName)
            {
                LOG_FUNCTION_ENTER_VERBOSE(L"PromiseDefault(instanceName = {})", instanceName);
            }
            template <typename Caller>
            PromiseDefault(Caller& caller, std::wstring instanceName)
                : _MyBase(caller, instanceName)
            {
                LOG_FUNCTION_ENTER_VERBOSE(L"PromiseDefault(Caller, instanceName = {})", instanceName);
            }
        };

        class PromiseRoot : public Promise<PromiseRoot> {
        public:
            using _MyBase = Promise<PromiseRoot>;
            INTELLISENSE_DEFAULT_CTOR(PromiseRoot);

            PromiseRoot(std::wstring instanceName, std::function<void(std::weak_ptr<CoTaskBase>)> resumeCallback)
                : _MyBase(instanceName)
            {
                LOG_FUNCTION_ENTER_VERBOSE(L"PromiseRoot(instanceName = {}, resumeCallback)", instanceName);
                this->_MyBase::resumeCallback = resumeCallback;
            }
            template <typename Caller>
            PromiseRoot(Caller& caller, std::wstring instanceName, std::function<void(std::weak_ptr<CoTaskBase>)> resumeCallback)
                : _MyBase(caller, instanceName)
            {
                LOG_FUNCTION_ENTER_VERBOSE(L"Promise(Caller, instanceName = {}, resumeCallback)", instanceName);
                this->_MyBase::resumeCallback = resumeCallback;
            }
        };

        class PromiseSignal : public Promise<PromiseSignal> {
        public:
            using _MyBase = Promise<PromiseSignal>;
            INTELLISENSE_DEFAULT_CTOR(PromiseSignal);

            PromiseSignal(std::wstring instanceName, std::shared_ptr<HELPERS_NS::Signal> resumeSignal)
                : _MyBase(instanceName)
                , resumeSignal{ resumeSignal }
            {
                LOG_FUNCTION_ENTER_VERBOSE(L"PromiseSignal(instanceName = {}, resumeSignal)", instanceName);
            }
            template <typename Caller>
            PromiseSignal(Caller& caller, std::wstring instanceName, std::shared_ptr<HELPERS_NS::Signal> resumeSignal)
                : _MyBase(caller, instanceName)
                , resumeSignal{ resumeSignal }
            {
                LOG_FUNCTION_ENTER_VERBOSE(L"PromiseSignal(Caller, instanceName = {}, resumeSignal)", instanceName);
            }

            std::weak_ptr<HELPERS_NS::Signal> get_resume_signal() {
                return resumeSignal;
            }

        private:
            std::shared_ptr<HELPERS_NS::Signal> resumeSignal;
        };

        /*------------------*/
        /*   AWAITER BASE   */
        /*------------------*/
        template <typename PromiseImplT>
        class AwaiterBase { // Base Awaiter class for CoTask. You need overload await_suspend for all type of promises that can be used. 
        public:
            AwaiterBase(std::coroutine_handle<PromiseImplT> promiseCoroHandle)
                : promiseCoroHandle{ promiseCoroHandle }
            {
            }
            bool await_ready() noexcept {
                return suspend::always;
            }
            std::coroutine_handle<PromiseImplT> await_suspend(std::coroutine_handle<PromiseDefault> callerPromiseCoroHandle) noexcept {
                return await_suspend_internal<PromiseDefault>(callerPromiseCoroHandle);
            }
            std::coroutine_handle<PromiseImplT> await_suspend(std::coroutine_handle<PromiseRoot> callerPromiseCoroHandle) noexcept {
                return await_suspend_internal<PromiseRoot>(callerPromiseCoroHandle);
            }
            std::coroutine_handle<PromiseImplT> await_suspend(std::coroutine_handle<PromiseSignal> callerPromiseCoroHandle) noexcept {
                return await_suspend_internal<PromiseSignal>(callerPromiseCoroHandle);
            }
            void await_resume() {
                LOG_FUNCTION_ENTER_VERBOSE("await_resume()");
            }

        private:
            // NOTE: callerPromiseCoroHandle associated with outter co-function context where called operator co_await CoTask{}
            template <typename PromiseT>
            std::coroutine_handle<PromiseImplT> await_suspend_internal(std::coroutine_handle<PromiseT> callerPromiseCoroHandle) noexcept {
                LOG_FUNCTION_SCOPE_VERBOSE("await_suspend_internal(callerPromiseCoroHandle)");
                // 1. Remember callerPromiseCoroHandle (it is resumed when promiseCoroHandle finished in Promise::FinalAwaiter).
                // 2. Resume promiseCoroHandle.
                promiseCoroHandle.promise().set_continuation(callerPromiseCoroHandle);
                return promiseCoroHandle;
                // 3. Control returned to callerPromiseCoroHandle only when promiseCoroHandle finished;
                //    if promiseCoroHandle has its own suspend points control returned to base resumer.
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
            CoTaskBase() { // In most cases default Ctor must not be called
                LOG_FUNCTION_ENTER_VERBOSE("CoTaskBase()");
            }
            CoTaskBase(std::coroutine_handle<> promiseCoroHandle, std::weak_ptr<int> promiseToken, std::wstring instanceName)
                : promiseCoroHandle{ promiseCoroHandle }
                , promiseToken{ promiseToken }
            {
                this->SetFullClassName(instanceName);
                LOG_FUNCTION_ENTER_VERBOSE_C("CoTaskBase(promiseCoroHandle, promiseToken, instanceName)");
            }
            ~CoTaskBase() {
                LOG_FUNCTION_ENTER_VERBOSE_C("~CoTaskBase()");
            }

            CoTaskBase(const CoTaskBase&) = delete;
            CoTaskBase& operator=(const CoTaskBase&) = delete;

            CoTaskBase(CoTaskBase&&) = delete;
            CoTaskBase& operator=(CoTaskBase&&) = delete;

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

            CoTask() { // In most cases default Ctor must not be called
                LOG_FUNCTION_ENTER_VERBOSE("CoTask()");
            }
            CoTask(CoHandle_t promiseCoroHandle, std::weak_ptr<int> promiseToken, std::wstring instanceName)
                : CoTaskBase(promiseCoroHandle, promiseToken, instanceName)
                , promiseCoroHandleTyped{ promiseCoroHandle }
            {
                this->SetFullClassName(instanceName);
                LOG_FUNCTION_ENTER_VERBOSE_C("CoTask(promiseCoroHandle, promiseToken, instanceName)");
            }
            ~CoTask() {
                LOG_FUNCTION_ENTER_VERBOSE_C("~CoTask()");
                if (promiseCoroHandleTyped) {
                    // promise_type will be destroyed; no need destroy promiseCoroHandle because it is the same.
                    promiseCoroHandleTyped.destroy(); 
                }
            }
            
            CoTask(const CoTask&) = delete;
            CoTask& operator=(const CoTask&) = delete;

            CoTask(CoTask&&) = delete;
            CoTask& operator=(CoTask&&) = delete;

            //CoTask(CoTask&& other)
            //    : promiseCoroHandle{ other.promiseCoroHandle }
            //    , promiseToken{ other.promiseToken }
            //    , canceled{ other.canceled }
            //    , promiseCoroHandleTyped{ other.promiseCoroHandleTyped }
            //{
            //    other.promiseCoroHandle = nullptr;
            //    other.promiseCoroHandleTyped = nullptr;
            //}
            //CoTask& operator=(CoTask&& other)
            //    if (this != &other) {
            //        if (promiseCoroHandleTyped) {
            //            promiseCoroHandleTyped.destroy();
            //        }
            //        promiseCoroHandle = other.promiseCoroHandle;
            //        promiseToken = other.promiseToken;
            //        canceled = other.canceled;
            //        other.promiseCoroHandle = nullptr;
            //        other.promiseCoroHandleTyped = nullptr;
            //    }
            //    return *this;
            //}

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