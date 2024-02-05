#pragma once
#include <Helpers/common.h>
#include <Helpers/FunctionTraits.hpp>
#include <Helpers/Container.hpp>
#include <Helpers/Thread.h>
#include <Helpers/Logger.h>
#include "Awaitables.h"
#include "CoTask.h"
#include <variant>
#include <list>

// Don't forget return original definitions for these macros at the end of file
#define LOG_FUNCTION_ENTER_VERBOSE(fmt, ...)
#define LOG_FUNCTION_SCOPE_VERBOSE(fmt, ...)
#define LOG_FUNCTION_ENTER_VERBOSE_C(fmt, ...)
#define LOG_FUNCTION_SCOPE_VERBOSE_C(fmt, ...)

using namespace std::chrono_literals;

namespace HELPERS_NS {
    namespace Async {
        // TODO: move implementation to .cpp
        class AsyncTasks {
            CLASS_FULLNAME_LOGGING_INLINE_IMPLEMENTATION(AsyncTasks);            
        public:
            using Task = CoTask<PromiseDefault>;

            AsyncTasks(std::wstring instanceName = L"Unknown") {
                this->SetFullClassName(instanceName);
                LOG_FUNCTION_ENTER_VERBOSE_C("AsyncTasks()");
            }
            ~AsyncTasks() {
                LOG_FUNCTION_SCOPE_VERBOSE_C("~AsyncTasks()");
                Cancel();
            }

        private:
            using RootTask = CoTask<PromiseRoot>;

            // TODO: Rewrite wtih concepts
            template <bool Requirements, typename PromiseImplT>
            using AddedResult = std::enable_if_t<Requirements, std::weak_ptr<CoTask<PromiseImplT>>>;

            template <typename PromiseImplT>
            static constexpr bool IsValidPromise = std::is_same_v<PromiseImplT, PromiseDefault>;
            
            class TaskWrapper {
            public:
                TaskWrapper(std::shared_ptr<Task> task, std::chrono::milliseconds startAfter, std::function<void()> releaseCallback = nullptr)
                    : task{ task }
                    , startAfter{ startAfter }
                    , releaseCallback{ releaseCallback }
                {}
                ~TaskWrapper() {
                    if (releaseCallback) {
                        releaseCallback();
                    }
                }
                std::shared_ptr<Task>& GetTask() {
                    return task;
                }
                std::chrono::milliseconds GetStartAfterTime() {
                    return startAfter;
                }
            private:
                std::shared_ptr<Task> task;
                std::chrono::milliseconds startAfter;
                std::function<void()> releaseCallback;
            };

        public:
            void SetResumeCallback(std::function<void(std::weak_ptr<CoTaskBase>)> resumeCallback) {
                LOG_FUNCTION_ENTER_VERBOSE_C("SetResumeCallback(resumeCallback)");
                this->resumeCallback = resumeCallback;
            }

            template <typename PromiseImplT, typename... FnArgs, typename... RealArgs>
            AddedResult<IsValidPromise<PromiseImplT>, PromiseImplT> AddTaskFn(
                std::chrono::milliseconds startAfter,
                std::shared_ptr<CoTask<PromiseImplT>> (*taskFn)(FnArgs...), RealArgs&&... args)
            {
                LOG_FUNCTION_SCOPE_VERBOSE_C("AddTaskFn(startAfter, taskFn, ...args)");
                std::unique_lock lk{ mx };
                tasks.push(TaskWrapper{ taskFn(std::forward<RealArgs&&>(args)...), startAfter });
                return tasks.front().GetTask();
            }

            template <typename PromiseImplT, typename C, typename... FnArgs, typename... RealArgs>
            AddedResult<IsValidPromise<PromiseImplT>, PromiseImplT> AddTaskFn(
                std::chrono::milliseconds startAfter,
                C* classPtr, std::shared_ptr<CoTask<PromiseImplT>>(C::*taskFn)(FnArgs...), RealArgs&&... args)
            {
                LOG_FUNCTION_SCOPE_VERBOSE_C("AddTaskFn(startAfter, classPtr, taskFn, ...args)");
                std::unique_lock lk{ mx };
                tasks.push(TaskWrapper{ std::invoke(taskFn, classPtr, std::forward<RealArgs&&>(args)...), startAfter });
                return tasks.front().GetTask();
            }

            template <typename Fn, typename... Args, typename PromiseImplT = PromiseExtractor<typename HELPERS_NS::FunctionTraits<Fn>::Ret>::promiseImpl_t>
            AddedResult<IsValidPromise<PromiseImplT>, PromiseImplT> AddTaskLambda(
                std::chrono::milliseconds startAfter,
                Fn lambda, Args&&... args)
            {
                LOG_FUNCTION_SCOPE_VERBOSE_C("AddTaskLambda(startAfter, taskFn, ...args)");
                struct LambdaBindCoro {
                    static typename HELPERS_NS::FunctionTraits<Fn>::Ret Bind(Fn lambda, Args&&... args) {
                        co_await *lambda(std::forward<Args&&>(args)...);
                        co_return;
                    }
                };

                std::unique_lock lk{ mx };

                tasks.push(TaskWrapper{ LambdaBindCoro::Bind(lambda, std::forward<Args&&>(args)...), startAfter });

                return tasks.front().GetTask();
            }

            void StartExecuting() {
                LOG_FUNCTION_SCOPE_VERBOSE_C("StartExecuting()");
                if (LOG_ASSERT(!executingStarted.exchange(true), "Executing of root coroutine already started!")) {
                    return;
                }
                rootTask = StartExecutingCoroutine(L"rootTask", resumeCallback);
                HELPERS_NS::Async::SafeResume(rootTask);
                return;
            }

            void Cancel() {
                LOG_FUNCTION_SCOPE_VERBOSE_C("Cancel()");
                if (rootTask)
                    rootTask->cancel(); // no need mutex synchronization

                std::unique_lock lk{ mx };
                for (auto& taskWrapper : tasks) {
                    if (auto& task = taskWrapper.GetTask()) {
                        task->cancel();
                    }
                }
            }

        private:
            // TODO: Add multiple calls guard
            // TODO: Implement special CoTask for StartExecuting() to avoid resume this task from another thread
            RootTask::Ret_t StartExecutingCoroutine(std::wstring coroFrameName, std::function<void(std::weak_ptr<CoTaskBase>)> resumeCallback) {
                LOG_FUNCTION_SCOPE_VERBOSE_C("StartExecutingCoroutine()");
                static thread_local std::size_t functionEnterThreadId = HELPERS_NS::GetThreadId();

                while (!tasks.empty()) {
                    auto taskWrapper = GetNextTask();
                    auto startAfter = taskWrapper.GetStartAfterTime();
                    auto& task = taskWrapper.GetTask();

                    // NOTE: if you want resume such tasks (with 0ms timeout) asynchronously (in next workQueue Pop event) - comment this condition
                    if (startAfter.count() > 0) {
                        co_await ResumeAfter<RootTask::promise_type>(startAfter); // [suspend point] here control is returned to caller
                    }

                    if (HELPERS_NS::GetThreadId() != functionEnterThreadId) {
                        LOG_ERROR_D("You resume this task from thread that differs from initial!");
                    }

                    LOG_DEBUG_D("await co-task ...");
                    co_await *task;
                    LOG_DEBUG_D("co-task finished");
                }
                executingStarted = false; // TODO: move to promise final_suspend logic
                co_return;
            }

            TaskWrapper GetNextTask() {
                LOG_FUNCTION_SCOPE_VERBOSE_C("GetNextTask()");
                std::unique_lock lk{ mx };
                if (tasks.empty()) {
                    return TaskWrapper{ nullptr, 0ms };
                }
                auto taskWrapper = std::move(tasks.front());
                tasks.pop();
                return taskWrapper;
            }

        private:
            std::mutex mx;
            RootTask::Ret_t rootTask;
            std::atomic<bool> executingStarted = false;
            HELPERS_NS::iterable_queue<TaskWrapper> tasks;
            std::function<void(std::weak_ptr<CoTaskBase>)> resumeCallback;
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