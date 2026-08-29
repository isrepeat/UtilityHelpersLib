#pragma once
#include "RootHeader.h"

namespace NUGET_HELPERS_NS {
	template<typename EnumMsg, typename T>
	class Channel;

	template<typename R, typename... Ts>
	class Callback;
}

namespace CppFeatures {
	namespace details {
		enum ChannelBaseMessages {
			None,
			Connect,
		};
	}

	class CPPFEATURES_API Channel {
	public:
		Channel();
		~Channel();

		void Create(const std::wstring& pipeName, NUGET_HELPERS_NS::Callback<bool, int, std::vector<uint8_t>> listenHandler, int timeout = 0);
		void Open(const std::wstring& pipeName, NUGET_HELPERS_NS::Callback<bool, int, std::vector<uint8_t>> listenHandler, int timeout = 10'000);
		
		void Write(int msgType, std::vector<uint8_t> payload);

	private:
		std::unique_ptr<NUGET_HELPERS_NS::Channel<details::ChannelBaseMessages, uint8_t>> channel;
	};
}