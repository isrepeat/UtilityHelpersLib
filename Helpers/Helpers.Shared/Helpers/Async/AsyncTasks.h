#pragma once
#include <Helpers/common.h>
#include <Helpers/Container.hpp>
#include <Helpers/Thread.h>
//#include <Helpers/Signal.h>
#include <Helpers/Logger.h>
#include "Awaitables.h"
#include "CoTask.h"
#include <variant>

// Don't forget return original definitions for these macros at the end of file
#define LOG_FUNCTION_ENTER_VERBOSE(fmt, ...)
#define LOG_FUNCTION_SCOPE_VERBOSE(fmt, ...)
#define LOG_FUNCTION_ENTER_VERBOSE_C(fmt, ...)
#define LOG_FUNCTION_SCOPE_VERBOSE_C(fmt, ...)

namespace HELPERS_NS {
    namespace Async {
        // TODO: move this helpers to another file
        // Helper to get shared pointer type of variant or nullptr if variant has bad access
        template <typename T, typename... Types>
        std::shared_ptr<T>& GetVariantItem(std::variant<std::monostate, std::shared_ptr<Types>...>& variant) {
            static const std::shared_ptr<T> emptyPoiner = nullptr;

            try {
                return std::get<std::shared_ptr<T>>(variant);
            }
            catch (const std::bad_variant_access&) {
                return const_cast<std::shared_ptr<T>&>(emptyPoiner);
            }
        }

        template<class... Ts>
        struct overloaded : Ts... { using Ts::operator()...; };


        // TODO: move implementation to .cpp
        class AsyncTasks {
            CLASS_FULLNAME_LOGGING_INLINE_IMPLEMENTATION(AsyncTasks);
            using RootTask = CoTask<PromiseRoot>;
        public:
            using Task = CoTask<PromiseDefault>;
            using TaskSignal = CoTask<PromiseSignal>;

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

            // TODO: Rewrite wtih concepts
            template <typename PromiseImplT>
            using AddedTaskResult_t = std::enable_if_t<
                std::is_same_v<PromiseImplT, PromiseDefault> ||
                std::is_same_v<PromiseImplT, PromiseSignal>, std::weak_ptr<CoTask<PromiseImplT>>>;

            template <typename PromiseImplT>
            AddedTaskResult_t<PromiseImplT> Add(std::shared_ptr<CoTask<PromiseImplT>> task, std::chrono::milliseconds startAfter) {
                LOG_FUNCTION_SCOPE_VERBOSE_C("Add(task, startAfter)");
                std::unique_lock lk{ mx };
                tasks.push({ task, startAfter });
                return task;
            }

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
                for (auto& [taskVariant, startAfter] : tasks) {
                    std::visit(overloaded{
                        [](std::monostate) {},
                        [](auto&& task) {
                            if (task)
                                task->cancel();
                            }
                        }, taskVariant);
                }
            }

        private:
            struct TaskWrapper {
                std::variant<std::monostate, std::shared_ptr<Task>, std::shared_ptr<TaskSignal>> taskVariant;
                std::chrono::milliseconds startAfter;
            };
            // TODO: Add multiple calls guard
            // TODO: Implement special CoTask for StartExecuting() to avoid resume this task from another thread
            RootTask::Ret_t StartExecutingCoroutine(std::wstring coroFrameName, std::function<void(std::weak_ptr<CoTaskBase>)> resumeCallback) {
                LOG_FUNCTION_SCOPE_VERBOSE_C("StartExecutingCoroutine()");
                static thread_local std::size_t functionEnterThreadId = HELPERS_NS::GetThreadId();
                
                while (true) {
                    try {
                        auto [taskVariant, startAfter] = GetNextTask();
                        if (std::holds_alternative<std::monostate>(taskVariant)) {
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
                        // TODO: maybe refactor with custom task visitor or overload co_await for std::variant co-tasks?
                        if (auto task = GetVariantItem<Task>(taskVariant)) {
                            co_await *task;
                        }
                        else if (auto taskSignal = GetVariantItem<TaskSignal>(taskVariant)) {
                            co_await *taskSignal;
                        }
                        LOG_DEBUG_D("co-task finished");
                    }
                    catch (const std::bad_variant_access&) {
                        break;
                    }
                }
                executingStarted = false; // TODO: move to promise final_suspend logic
                co_return;
            }

            TaskWrapper GetNextTask() {
                LOG_FUNCTION_SCOPE_VERBOSE_C("GetNextTask()");
                std::unique_lock lk{ mx };
                if (tasks.empty()) {
                    return { std::monostate{}, 0ms };
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