#pragma once
#include "common.h"
#include <condition_variable>
#include <functional>
#include <chrono>
#include <thread>
#include <future>

namespace HELPERS_NS {
	namespace Chrono {
		template <class _To, class _Rep, class _Period>
		constexpr const std::chrono::duration<float, typename _To::period> duration_cast_float(const std::chrono::duration<_Rep, _Period>& _Dur) noexcept {
			return std::chrono::duration_cast<std::chrono::duration<float, typename _To::period>>(_Dur);
		}

		template <typename _Rep, typename _Period>
		struct DurationBase : std::chrono::duration<_Rep, _Period> {
			using _MyBase = std::chrono::duration<_Rep, _Period>;
			using _MyBase::duration;

			operator uint64_t() const {
				return this->count();
			}
			operator int64_t() const {
				return this->count();
			}
			operator uint32_t() const {
				return this->count();
			}
			operator int32_t() const {
				return this->count();
			}
			explicit operator float() const {
				return duration_cast_float<_MyBase>(*this).count();
			}
			explicit operator double() const {
				// TODO: add duration_cast_double
				return duration_cast_float<_MyBase>(*this).count();
			}
		};

		template <typename _Rep, typename _Period>
		constexpr DurationBase<_Rep, _Period> operator+(const DurationBase<_Rep, _Period>& _Left, const DurationBase<_Rep, _Period>& _Right) noexcept {
			return std::chrono::operator+(_Left, _Right);
		}

		template <typename _Rep, typename _Period>
		constexpr DurationBase<_Rep, _Period> operator-(const DurationBase<_Rep, _Period>& _Left, const DurationBase<_Rep, _Period>& _Right) noexcept {
			return std::chrono::operator-(_Left, _Right);
		}


		using Hns = DurationBase<unsigned long long, std::ratio<1, 10'000'000>>;
	}

	inline namespace Literals {
		inline namespace ChronoLiterals {
			constexpr Chrono::Hns operator"" hns(unsigned long long _Val) noexcept {
				return Chrono::Hns(_Val);
			}
		}
	}
	
	constexpr float HnsToSeconds(Chrono::Hns countHns) {
		return Chrono::duration_cast_float<std::chrono::seconds>(countHns).count();
	}
	constexpr float HnsToMilliseconds(Chrono::Hns countHns) {
		return Chrono::duration_cast_float<std::chrono::milliseconds>(countHns).count();
	}


	class Timer	{
	public:
		template <typename Duration>
		static void Once(Duration timeout, std::function<void()> callback) { // Not thread safe callback call
			std::thread([=] {
				std::this_thread::sleep_for(timeout);
				callback();
				}).detach();
		}

		template <typename Duration>
		static bool Once(std::unique_ptr<std::future<void>>& futureToken, Duration timeout, std::function<void()> callback) { // Thread safe callback call
			if (futureToken)
				return false; // Guard from double set. Don't init new future until previous not finished.

			futureToken = std::make_unique<std::future<void>>(std::async(std::launch::async, [&futureToken, timeout, callback] {
				std::this_thread::sleep_for(timeout);
				callback();
				futureToken = nullptr;
				}));

			return true;
		}


		Timer() = default;
		Timer(std::chrono::milliseconds timeout, std::function<void()> callback, bool autoRestart = false) {
			Start(timeout, callback, autoRestart);
		}
		~Timer() {
			StopThread();
		}

		void Start(std::chrono::milliseconds timeout, std::function<void()> callback, bool autoRestart = false) {
			StopThread();
			{
				std::lock_guard lk1{ mx };
				this->timeout = timeout;
				this->callback = callback;
				this->autoRestart = autoRestart;
			}
			StartThread();
		}
		void Stop() {
			StopThread();
		}

		void Reset(std::chrono::milliseconds timeout) {
			StopThread();
			StartThread();
		}

	private:
		void StopThread() {
			stop = true;
			cv.notify_all();
			if (threadTimer.joinable())
				threadTimer.join();
		}

		void StartThread() {
			stop = false;
			threadTimer = std::thread([this] {
				std::lock_guard lk1{ mx }; // guard from changing timeout and callback
				while (!stop) {
					std::mutex tmpMx;
					std::unique_lock lk2{ tmpMx };
					if (!cv.wait_for(lk2, timeout, [this] { return static_cast<bool>(stop); })) {
						if (callback) {
							callback();
						}
					}
					if (!autoRestart) {
						stop = true;
						break;
					}
				}
				});
		}
	
	private:
		std::mutex mx;
		std::thread threadTimer;
		std::condition_variable cv;

		std::function<void()> callback;
		std::chrono::milliseconds timeout;

		std::atomic<bool> stop = false;
		std::atomic<bool> autoRestart = false;
	};


	class MeasureTime {
	public:
		MeasureTime();
		~MeasureTime();

	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> start;
	};

	class MeasureTimeScoped {
	public:
		MeasureTimeScoped(std::function<void(uint64_t dtMs)> completedCallback);
		~MeasureTimeScoped();

	private:
		std::function<void(uint64_t dtMs)> completedCallback;
		std::chrono::time_point<std::chrono::high_resolution_clock> start;
	};

	enum class TimeFormat {
		None,
		Ymdhms_with_separators,
	};
	std::string GetTimeNow(TimeFormat format = TimeFormat::None);
	std::string GetTimezone();
};

using namespace std::chrono_literals;
using namespace HELPERS_NS::ChronoLiterals;


#ifdef _DEBUG
// https://stackoverflow.com/questions/1597007/creating-c-macro-with-and-line-token-concatenation-with-positioning-macr
#define MEASURE_TIME_TOKENPASTE(x, y) x ## y
// use it with __LINE__ to fix "hides declaration of the same name in outer scope"
#define MEASURE_TIME_TOKENPASTE2(x, y) MEASURE_TIME_TOKENPASTE(x, y)
#define MEASURE_TIME HELPERS_NS::MeasureTime MEASURE_TIME_TOKENPASTE2(_measureTimeScoped, __LINE__);
#define MEASURE_TIME_WITH_CALLBACK(callback) HELPERS_NS::MeasureTimeScoped MEASURE_TIME_TOKENPASTE2(_measureTimeScoped, __LINE__)(callback);
#else
#define MEASURE_TIME
#define MEASURE_TIME_WITH_CALLBACK(callback)
#endif