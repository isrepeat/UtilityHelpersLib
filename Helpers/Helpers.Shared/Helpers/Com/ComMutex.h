#pragma once
#include <Helpers/common.h>

namespace HELPERS_NS {
    namespace Com {
		// NOTE: mutex internal implementation may vary for different dlls 
		//       so use 'virtual' to safe call lock / unlock in the dll where the mutex was created.
		template <typename MutexT>
		struct Mutex {
			virtual void lock() {
				this->mx.lock();
			}
			virtual void unlock() {
				this->mx.unlock();
			}

		private:
			MutexT mx;
		};
    }
}