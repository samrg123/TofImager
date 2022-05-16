#pragma once

#include "util.h"

class Timer {
	private:

		uint64 timestampUs;

	protected:

		template<typename ResultT>
		static inline constexpr ResultT DivideTime(uint64 time, auto n) {

			if constexpr(std::is_floating_point<ResultT>::value) {

				return time / ResultT(n); 

			} else if constexpr(std::is_integral<ResultT>::value) {

				return time / n;

			} else if constexpr(IsFixedPoint<ResultT>()) {

				return ResultT(time, n);

			} else {
				static_assert(Defer<ResultT>(false), "Unsupported ResultT");
			}
		}

	public:

		inline constexpr Timer(uint64 timestampUs = 0): timestampUs(timestampUs) {}

		inline void Lap() {
			timestampUs = micros64();
		}

		inline uint64 LapUs() {
			uint64 currentUs = micros64();
			uint64 deltaUs = currentUs - timestampUs;

			timestampUs = currentUs;

			return deltaUs;
		}

		template<typename ResultT = float>
		inline ResultT LapMs() {
			uint64 deltaUs = LapUs();
			return DivideTime<ResultT>(deltaUs, 1000);
		}

		template<typename ResultT = float>
		inline ResultT LapS() {
			uint64 deltaUs = LapUs();
			return DivideTime<ResultT>(deltaUs, 1000000);
		}

};
