#include "gif.h"

namespace vgc
{
	std::vector<USHORT> TimestampsToGifDelays(const std::vector<Timestamp>& timestamps, Timestamp stopTime)
	{
		if (timestamps.empty())
		{
			return {};
		}

		const size_t n = timestamps.size();
		std::vector<USHORT> result(n);

		Timestamp gifTime = timestamps[0];

		for (size_t i = 0; i < n; i++)
		{
			auto finish = i + 1 == n ? stopTime : timestamps[i + 1];

			long long desiredLength = finish - gifTime;

			if (desiredLength < 10'000'000)
			{
				// the time is too short, skip this frame
				result[i] = 0;
			}
			else
			{
				result[i] = (USHORT)std::clamp((desiredLength + 5'000'000) / 10'000'000, 2ll, 65535ll);
				gifTime += result[i] * 10'000'000ll;
			}
		}

		return result;
	}
}