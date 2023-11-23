#pragma once
#include <condition_variable>

namespace H {
	// NOTE: use "Snake Case" code style to be combatible with std::..._lock
	class MultithreadMutex {
	public:
		MultithreadMutex() = default;
		~MultithreadMutex();

		void lock();
		void unlock();

		bool is_locked();

	private:
		std::mutex mx;
		std::atomic<bool> locked = false;
		std::condition_variable cvLocker;
	};
}