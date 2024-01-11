#include "MultithreadMutex.h"

namespace HELPERS_NS {
	void MultithreadMutex::lock() {
		std::unique_lock<std::mutex> lk(mx);
		cvLocker.wait(lk, [this] {
			return !locked;
			});
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