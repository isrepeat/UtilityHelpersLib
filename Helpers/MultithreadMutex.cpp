#include "MultithreadMutex.h"

namespace H {
	void MultithreadMutex::lock() {
		std::unique_lock<std::mutex> lk(mx);
		if (locked) {
			cvLocker.wait(lk, [this] {
				return static_cast<bool>(!locked);
				});
		}
		locked = true;
	}

	void MultithreadMutex::unlock() {
		std::unique_lock<std::mutex> lk(mx);
		locked = false;
		cvLocker.notify_all();
	}

	bool MultithreadMutex::is_locked() {
		std::unique_lock<std::mutex> lk(mx);
		return locked;
	}

	MultithreadMutex::~MultithreadMutex() {
		cvLocker.notify_all();
	}
}