#include "Channel.h"
#include "Helpers.h"
//#include "Helpers/Helpers.h"
#pragma push_macro("HELPERS_NS")
#define HELPERS_NS NUGET_HELPERS_NS
// Include here all headers (that is used in public headers of .Desktop project) with NUGET_HELPERS_NS
// TODO: move this logic to .pch file.
#include "Helpers/CallbackWinRT.hpp"
#pragma pop_macro("HELPERS_NS")


namespace CppFeatures {
	namespace Cx {
		Channel::Channel()
			: channelNative{}
		{}

		Channel::~Channel()
		{}

		void Channel::Create(Platform::String^ pipeName, ListenHandler^ listenHandler) {
			this->listenHandler = listenHandler;
			this->channelNative.Create(pipeName->Data(), NUGET_HELPERS_NS::MakeWinRTCallback(this, Channel::ListenCallback));
		}

		void Channel::Open(Platform::String^ pipeName, ListenHandler^ listenHandler) {
			this->listenHandler = listenHandler;
			this->channelNative.Open(pipeName->Data(), NUGET_HELPERS_NS::MakeWinRTCallback(this, Channel::ListenCallback));
		}

		void Channel::Write(int msgType, Platform::String^ payload) {
			std::string str = details::WStrToStr({ payload->Data(), payload->Length() });
			this->channelNative.Write(msgType, std::vector<uint8_t>(str.begin(), str.end()));
		}

		bool Channel::ListenCallback(Channel^ _this, int msgType, std::vector<uint8_t> payload) {
			if (_this->listenHandler) {
				std::wstring wstr = details::StrToWStr({ payload.begin(), payload.end() }, CP_UTF8);
				return _this->listenHandler->Invoke(msgType, ref new Platform::String(wstr.data(), wstr.size()));
			}
			return false;
		}
	}
}