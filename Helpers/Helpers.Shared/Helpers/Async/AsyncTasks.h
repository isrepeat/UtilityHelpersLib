#pragma once
#include <Helpers/common.h>
#include <Helpers/Container.hpp>
#include <Helpers/Thread.h>
//#include <Helpers/Signal.h>
#include <Helpers/Logger.h>
#include "Awaitables.h"
#include "CoTask.h"

// Don't forget return original definitions for these macros at the end of file
#define LOG_FUNCTION_ENTER_VERBOSE(fmt, ...)
#define LOG_FUNCTION_SCOPE_VERBOSE(fmt, ...)
#define LOG_FUNCTION_ENTER_VERBOSE_C(fmt, ...)
#define LOG_FUNCTION_SCOPE_VERBOSE_C(fmt, ...)

namespace HELPERS_NS {
    namespace Async {
        // TODO: move implementation to .cpp
        class AsyncTasks {
            CLASS_FULLNAME_LOGGING_INLINE_IMPLEMENTATION(AsyncTasks);
        public:
            using Task = typename CoTask<PromiseDefault>;
            using RootTask = typename CoTask<PromiseRoot>;

            AsyncTasks(std::wstring instanceName = L"Unknown") {
                this->SetFullClassName(instanceName);
                LOG_FUNCTION_ENTER_VERBOSE_C("AsyncTasks()");
            }
            ~AsyncTasks() {
                LOG_FUNCTION_SCOPE_VERBOSE_C("~AsyncTasks()");
                Cancel();
            }
            void SetResumeCallback(std::function<void(std::weak_ptr<CoTaskBase>)> resumeCallback) {
                LOG_FUNCTION_ENTER_VERBOSE_C("SetResumeCallback(resumeCallback)");
                this->resumeCallback = resumeCallback;
            }
            std::weak_ptr<Task> Add(Task::Ret_t task, std::chrono::milliseconds startAfter) {
                LOG_FUNCTION_SCOPE_VERBOSE_C("Add(task, startAfter)");
                std::unique_lock lk{ mx };
                tasks.push({ task, startAfter });
                return task;
            }
            //std::weak_ptr<Task> Add(Task::Ret_t task, std::shared_ptr<HELPERS_NS::Signal> completedSignal) {
            //    LOG_FUNCTION_SCOPE_VERBOSE_C("Add(task, startAfter)");
            //    std::unique_lock lk{ mx };
            //    tasks.push({ task, 0ms, completedSignal });
            //    return task;
            //}
            void StartExecuting() {
                LOG_FUNCTION_SCOPE_VERBOSE_C("StartExecuting()");
                if (LOG_ASSERT(!executingStarted.exchange(true), "Executing of root coroutine already started!")) {
                    return;
                } 
                rootTask = StartExecutingCoroutine(L"rootTask", resumeCallback);
                rootTask->resume();
                return;
            }
            void Cancel() {
                LOG_FUNCTION_SCOPE_VERBOSE_C("Cancel()");
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
                //std::shared_ptr<HELPERS_NS::Signal> completedSignal;
            };
            // TODO: Add multiple calls guard
            // TODO: Implement special CoTask for StartExecuting() to avoid resume this task from another thread
            RootTask::Ret_t StartExecutingCoroutine(std::wstring coroFrameName, std::function<void(std::weak_ptr<CoTaskBase>)> resumeCallback) {
                LOG_FUNCTION_SCOPE_VERBOSE_C("StartExecutingCoroutine()");
                static thread_local std::size_t functionEnterThreadId = HELPERS_NS::GetThreadId();

                while (true) {
                    auto [task, startAfter] = GetNextTask();
                    if (!task) {
                        break;
                    }
                    
                    // NOTE: if you want resume such tasks (with 0ms timeout) in next workQueue Pop event - comment this condition
                    if (startAfter.count() > 0) {
                        co_await ResumeAfter<RootTask::promise_type>(startAfter); // [suspend point] here control is returned to caller
                    }

                    if (HELPERS_NS::GetThreadId() != functionEnterThreadId) {
                        LOG_ERROR_D("It seems you resume this task from thread that differs from initial. Force return.");
                        executingStarted = false;
                        co_return; // mb need clear tasks queue also?
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
                    return { nullptr, 0ms };
                }
                auto task = std::move(tasks.front());
                tasks.pop();
                return task;
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