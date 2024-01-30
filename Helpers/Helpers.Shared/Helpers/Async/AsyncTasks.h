#pragma once
#include <Helpers/common.h>
#include <Helpers/Container.hpp>
#include <Helpers/Thread.h>
#include <Helpers/Logger.h>
#include "Awaitables.h"
#include "CoTask.h"

// Don't forget return original definitions for these macros at the end of file
//#define LOG_FUNCTION_ENTER_VERBOSE(fmt, ...)
//#define LOG_FUNCTION_SCOPE_VERBOSE(fmt, ...)

namespace HELPERS_NS {
    namespace Async {
        // TODO: move implementation to .cpp
        class AsyncTasks {
        public:
            using Task = typename CoTask<initial_suspend_always>;
            using RootTask = typename CoTask<initial_suspend_never>;

            AsyncTasks() {
                LOG_FUNCTION_ENTER_VERBOSE("AsyncTasks()");
            }
            ~AsyncTasks() {
                LOG_FUNCTION_SCOPE_VERBOSE("~AsyncTasks()");
                Cancel();
            }
            void SetResumeCallback(std::function<void(std::weak_ptr<RootTask>)> resumeCallback) {
                LOG_FUNCTION_ENTER_VERBOSE("SetResumeCallback(resumeCallback)");
                this->resumeCallback = resumeCallback;
            }
            void Add(Task::Ret_t task, std::chrono::milliseconds startAfter) {
                LOG_FUNCTION_SCOPE_VERBOSE("Add(task, startAfter)");
                std::unique_lock lk{ mx };
                tasks.push({ std::move(task), startAfter });
            }
            // CHECK: Should not be called untill previous StartExecutingCoroutine() not finished
            void StartExecuting() {
                LOG_FUNCTION_SCOPE_VERBOSE("StartExecuting()");
                //StartExecutingCoroutine();
                rootTask = StartExecutingCoroutine();
                //rootTask->cancel();
                return;
            }
            void Cancel() {
                LOG_FUNCTION_SCOPE_VERBOSE("Cancel()");
                if (rootTask)
                    rootTask->cancel(); // no need mutex synchronization

                std::unique_lock lk{ mx };
                for (auto& [task, startAfter] : tasks) {
                    task->cancel();
                }
            }

        private:
            struct TaskWrapper {
                Task::Ret_t task;
                std::chrono::milliseconds startAfter;
            };
            // TODO: Add multiple calls guard
            // TODO: Implement special CoTask for StartExecuting() to avoid resume this task from another thread
            RootTask::Ret_t StartExecutingCoroutine() {
                LOG_FUNCTION_SCOPE_VERBOSE("StartExecutingCoroutine()");
                static thread_local std::size_t functionEnterThreadId = HELPERS_NS::GetThreadId();

                while (true) {
                    auto [task, startAfter] = GetNextTask();
                    if (!task) {
                        break;
                    }

                    co_await InvokeCallbackAfter<RootTask>(startAfter, resumeCallback); // [suspend point] here control is returned to caller

                    if (HELPERS_NS::GetThreadId() != functionEnterThreadId) {
                        LOG_ERROR_D("It seems you resume this task from thread that differs from initial. Force return.");
                        co_return;
                    }
                    LOG_DEBUG_D("resume co-task ...");
                    task->resume(); // here context is changed on co-task ...
                    LOG_DEBUG_D("co-task finished");
                }
                co_return;
            }

            TaskWrapper GetNextTask() {
                LOG_FUNCTION_SCOPE_VERBOSE("GetNextTask()");
                std::unique_lock lk{ mx };
                if (tasks.empty()) {
                    return { nullptr, 0ms };
                }
                auto task = std::move(tasks.front());
                tasks.pop();
                return task;
            }

        private:
            std::mutex mx;
            RootTask::Ret_t rootTask;
            HELPERS_NS::iterable_queue<TaskWrapper> tasks;
            std::function<void(std::weak_ptr<RootTask>)> resumeCallback;
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