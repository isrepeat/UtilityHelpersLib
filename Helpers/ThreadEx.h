#pragma once
#include <functional>
#include <thread>

namespace H {
	// Wrapper over std::thread that notify you if was exception
	class ThreadEx : public std::thread {
	public:
		ThreadEx() = default;
		~ThreadEx() = default;

		// TODO: Add Ctor and pass "thread message queue" to which signal will be sent

		explicit ThreadEx(std::function<void()> exceptionCallback, std::function<void()> lambda);

		ThreadEx(ThreadEx&&) = default;
		ThreadEx& operator=(ThreadEx&&) = default;

		ThreadEx(const ThreadEx&) = delete;
		ThreadEx& operator=(const ThreadEx&) = delete;

		std::exception_ptr GetExceptionPtr();

	private:
		void WorkingRoutine(std::function<void()> exceptionCallback, std::function<void()> lambda);

	private:
		std::exception_ptr exceptionPtr;
		std::thread workThread;
	};
}