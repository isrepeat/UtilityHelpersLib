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

		using Hns = DurationBase<long long, std::ratio<1, 10'000'000>>; // Do not use unsigned bacause may be sideeffects when cast to float
	}

	inline namespace Literals {
		inline namespace ChronoLiterals {
			constexpr Chrono::Hns operator"" _hns(unsigned long long _Val) noexcept {
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
		MeasureTimeScoped(std::function<void(std::chrono::duration<double, std::milli> dt)> completedCallback);
		~MeasureTimeScoped();

	private:
		std::function<void(std::chrono::duration<double, std::milli>)> completedCallback;
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



#if _DEBUG
// https://stackoverflow.com/questions/1597007/creating-c-macro-with-and-line-token-concatenation-with-positioning-macr
// use it with __LINE__ to fix "hides declaration of the same name in outer scope"
//#define MEASURE_TIME HELPERS_NS::MeasureTime H_CONCAT(_measureTimeScoped, __LINE__);
//#define MEASURE_TIME_WITH_CALLBACK(callback) HELPERS_NS::MeasureTimeScoped H_CONCAT(_measureTimeScoped, __LINE__)(callback);
#else
//#define MEASURE_TIME
//#define MEASURE_TIME_WITH_CALLBACK(callback)
#endif


#if _DEBUG && SPDLOG_SUPPORT
#define MEASURE_TIME_SCOPED(name) HELPERS_NS::MeasureTimeScoped H_CONCAT(_measureTimeScoped, __LINE__)([] (std::chrono::duration<double, std::milli> dt) {	\
		LOG_DEBUG_D("[" ##name "] dt = {}", dt.count());																									\
		})

#define LOG_DELTA_TIME_POINTS(tpNameB, tpNameA) LOG_DEBUG_D("" #tpNameB " - " #tpNameA " = {}", std::chrono::duration<double, std::milli>(tpNameB - tpNameA).count());
#else
#define MEASURE_TIME_SCOPED(name)
#define LOG_DELTA_TIME_POINTS(tpNameB, tpNameA)
#endif