#include "CrashHandler.h"
#include "../CrashHandling.Shared/CrashHandling.h"
// TODO: move this logic to .pch file.
// Include here all headers with necessary XXX_NUGET_YYY_NS 
// (that is used in public headers of .Desktop project or depends on it).
#pragma push_macro("HELPERS_NS");
#define HELPERS_NS CRASH_HANDLING_NUGET_HELPERS_NS
#include "Helpers/CallbackWinRT.hpp"
#pragma pop_macro("HELPERS_NS");

namespace CrashHandling {
	namespace Cx {
		CrashHandlerWinRT^ CrashHandlerWinRT::instance = nullptr;

		CrashHandlerWinRT::CrashHandlerWinRT(Platform::String^ minidumpWriterProtocolName, Platform::String^ appCenterId, Platform::String^ appUuid)
			: initializedThreadId{ static_cast<size_t>(::GetCurrentThreadId()) }
		{
			CrashHandling::CrashHandler::Init(minidumpWriterProtocolName->Data(), appCenterId->Data(), appUuid->Data());
			CrashHandling::CrashHandler::SetProtocolCommandArgs({
				{L"-isUWP", L""},
				{L"-debug", L""}
				});
			CrashHandling::CrashHandler::SetCrashCallback(CrashHandlingHelpers::MakeWinRTCallback(this, CrashHandlerWinRT::CrashCallback));
			CrashHandling::CrashHandler::SetRunProtocolWithParamsCallback(CrashHandlingHelpers::MakeWinRTCallback(this, CrashHandlerWinRT::RunProtocolWithParamsCallback));
		}

		CrashHandlerWinRT^ CrashHandlerWinRT::CreateInstance(Platform::String^ minidumpWriterProtocolName, Platform::String^ appCenterId, Platform::String^ appUuid) {
			if (!instance) {
				instance = ref new CrashHandlerWinRT(minidumpWriterProtocolName, appCenterId, appUuid);
			}
			return instance;
		}
		CrashHandlerWinRT^ CrashHandlerWinRT::GetInstance() {
			return instance;
		}


		void CrashHandlerWinRT::CrashCallback(CrashHandlerWinRT^ _this) {
			_this->CrashEvent();
		}

		void CrashHandlerWinRT::RunProtocolWithParamsCallback(CrashHandlerWinRT^ _this, const std::wstring& protocolWithParams) {
			std::size_t callerThreadId = static_cast<size_t>(::GetCurrentThreadId());

			if (callerThreadId == _this->initializedThreadId) {
				auto uri = ref new Windows::Foundation::Uri(ref new Platform::String(protocolWithParams.c_str()));
				Windows::System::Launcher::LaunchUriAsync(uri);
			}
			else {
				// Need run from UI thread
				Windows::ApplicationModel::Core::CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::High,
					ref new Windows::UI::Core::DispatchedHandler([protocolWithParams] {
						auto uri = ref new Windows::Foundation::Uri(ref new Platform::String(protocolWithParams.c_str()));
						Windows::System::Launcher::LaunchUriAsync(uri);
						}));
			}
		}
	}
}