#include "CrashHandling.h"
#include <Helpers/PackageProvider.h>
#include <Helpers/Singleton.hpp>
#include "Helpers/Callback.hpp"
#include <Helpers/CrashInfo.h>
#include <Helpers/Helpers.h>
#include <Helpers/Channel.h>
#include <Helpers/Logger.h>
#include <ComAPI/ComAPI.h>
#include "MiniDumpMessages.h"
#include <dbghelp.h>
#include <format>

#pragma comment (lib, "dbghelp.lib" )

namespace CrashHandling {
	std::function<void(EXCEPTION_POINTERS*, ExceptionType)> gCrashCallback = nullptr;
	H::Channel<MiniDumpMessages> channelMinidump; // TODO: maybe encapsulate to CrashHandlerBaseSingleton
	const uint32_t ClrException = 0xE0434352;
	const uint32_t CppException = 0xE06D7363;
	bool handledCrashException = false;
	bool wasCrashException = false;
	int waitBeforeExit = 0; // [ms]


	using CrashHandlerBaseSingleton = H::SingletonUnscoped<class CrashHandlerBase>;

	class CrashHandlerBase {
	public:
		CrashHandlerBase(std::wstring runProtocol, std::wstring appCenterId, std::wstring appUuid)
			: runProtocol{ runProtocol }
		{
			H::Flags<lg::InitFlags> loggerInitFlags = lg::InitFlags::DefaultFlags | lg::InitFlags::CreateInPackageFolder;
			lg::DefaultLoggers::Init(L".\\CrashHandling.log", loggerInitFlags);

			additionalInfo.appCenterId = appCenterId;
			additionalInfo.appVersion = L""; // detect automatically
			additionalInfo.appUuid = appUuid;

			CrashHandling::RegisterDefaultCrashHandler([this](EXCEPTION_POINTERS* pExceptionPtrs, CrashHandling::ExceptionType exType) {
				LOG_FUNCTION_ENTER("DefaultCrashHandler(...)");
				std::wstring exceptionMsg;

				switch (exType) {
				case CrashHandling::ExceptionType::StructuredException: {
					switch (pExceptionPtrs->ExceptionRecord->ExceptionCode) {
					case EXCEPTION_ACCESS_VIOLATION:
						exceptionMsg = L"EXCEPTION_ACCESS_VIOLATION";
						break;
					case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
						exceptionMsg = L"EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
						break;
					case EXCEPTION_DATATYPE_MISALIGNMENT:
						exceptionMsg = L"EXCEPTION_DATATYPE_MISALIGNMENT";
						break;
					case EXCEPTION_FLT_DENORMAL_OPERAND:
						exceptionMsg = L"EXCEPTION_FLT_DENORMAL_OPERAND";
						break;
					case EXCEPTION_FLT_DIVIDE_BY_ZERO:
						exceptionMsg = L"EXCEPTION_FLT_DIVIDE_BY_ZERO";
						break;
					case EXCEPTION_FLT_INEXACT_RESULT:
						exceptionMsg = L"EXCEPTION_FLT_INEXACT_RESULT";
						break;
					case EXCEPTION_FLT_INVALID_OPERATION:
						exceptionMsg = L"EXCEPTION_FLT_INVALID_OPERATION";
						break;
					case EXCEPTION_FLT_OVERFLOW:
						exceptionMsg = L"EXCEPTION_FLT_OVERFLOW";
						break;
					case EXCEPTION_FLT_STACK_CHECK:
						exceptionMsg = L"EXCEPTION_FLT_STACK_CHECK";
						break;
					case EXCEPTION_ILLEGAL_INSTRUCTION:
						exceptionMsg = L"EXCEPTION_ILLEGAL_INSTRUCTION";
						break;
					case EXCEPTION_IN_PAGE_ERROR:
						exceptionMsg = L"EXCEPTION_IN_PAGE_ERROR";
						break;
					case EXCEPTION_INT_DIVIDE_BY_ZERO:
						exceptionMsg = L"EXCEPTION_INT_DIVIDE_BY_ZERO";
						break;
					case EXCEPTION_INT_OVERFLOW:
						exceptionMsg = L"EXCEPTION_INT_OVERFLOW";
						break;
					}
					break;
				}
				case CrashHandling::ExceptionType::UnhandledException: {
					exceptionMsg = L"UNHANDLED_EXCEPTION";
					break;
				}
				}

				auto backtrace = CrashHandling::GetBacktrace(0);
				auto backtraceStr = CrashHandling::BacktraceToString(backtrace);

				LOG_ERROR(L"{} [{}]", exceptionMsg, pExceptionPtrs->ExceptionRecord->ExceptionCode);
				LOG_ERROR(L"\n\n Backtrace:\n{}", backtraceStr);

				if (this->crashCallback) {
					LOG_FUNCTION_SCOPE("crashCallback()");
					this->crashCallback();
				}

				this->additionalInfo.backtrace = backtraceStr;
				this->additionalInfo.exceptionMsg = exceptionMsg;

				if (this->runProtocolWithParamsCallback) {
					// WinRT require launch uri protocol in UI thread
					CrashHandling::GenerateCrashReport(pExceptionPtrs, this->additionalInfo, this->runProtocol, this->protocolCommandArgs, this->runProtocolWithParamsCallback);
				}
				else {
					// Suitable for Desktop or CLR
					CrashHandling::GenerateCrashReport(pExceptionPtrs, this->additionalInfo, this->runProtocol, this->protocolCommandArgs);
				}

				if (this->finishCallback) {
					LOG_FUNCTION_SCOPE("finishCallback()");
					// Here you can delete CrashHandlerSingleton manually 
					// to ensure that Dtor CrashHandler is called before exit/crash (if this code executing under CLR).
					this->finishCallback();
				}

				channelMinidump.StopChannel();
				std::this_thread::sleep_for(std::chrono::milliseconds(waitBeforeExit));
				CrashHandlerBaseSingleton::DeleteInstance(); // Delete manually because Dtor may be not called under CLR, CHECK!

				LOG_DEBUG("-- EXIT(0) --");
				exit(0);
				});
		}

		~CrashHandlerBase() = default;


		void SetCrashCallback(H::Callback<void> crashCallback) {
			this->crashCallback = crashCallback;
		}
		void SetFinishCallback(H::Callback<void> finishCallback) {
			this->finishCallback = finishCallback;
		}
		void SetRunProtocolWithParamsCallback(H::Callback<void, const std::wstring&>runProtocolWithParamsCallback) {
			this->runProtocolWithParamsCallback = runProtocolWithParamsCallback;
		}
		void SetProtocolCommandArgs(std::vector<std::pair<std::wstring, std::wstring>> protocolCommandArgs) {
			this->protocolCommandArgs = protocolCommandArgs;
		}

	private:
		std::wstring runProtocol;
		AdditionalInfo additionalInfo;
		H::Callback<void> crashCallback;
		H::Callback<void> finishCallback;
		H::Callback<void, const std::wstring&> runProtocolWithParamsCallback;
		std::vector<std::pair<std::wstring, std::wstring>> protocolCommandArgs;
	};



	CRASH_HANDLING_API Backtrace_t GetBacktrace(int SkipFrames) {
		constexpr int TRACE_MAX_STACK_FRAMES = 99;
		void* stack[TRACE_MAX_STACK_FRAMES];
		ULONG hash;

		int attempts = 3;
		int numFrames = 0;
		while ((numFrames = CaptureStackBackTrace(SkipFrames + 1, TRACE_MAX_STACK_FRAMES, stack, &hash)) == 0) {
			if (--attempts == 0)
				break;
			//printf("numFrames == 0, try again [attempts left %d]", attempts);
		}

		if (numFrames == 0) {
			LOG_ERROR("Can't capture backtrace");
			return {};
		}

		Backtrace_t backtrace;
		std::wstring prevModuleFilename = L"";

		for (int i = 0; i < numFrames; ++i) {
			void* moduleBaseVoid = nullptr;
			RtlPcToFileHeader(stack[i], &moduleBaseVoid);
			auto moduleBase = (const unsigned char*)moduleBaseVoid;
			constexpr auto MODULE_BUF_SIZE = 4096U;
			wchar_t modulePath[MODULE_BUF_SIZE];
			if (moduleBase != nullptr) {
				GetModuleFileNameW((HMODULE)moduleBase, modulePath, MODULE_BUF_SIZE);
				std::wstring moduleFilepath{ modulePath };

				int n = moduleFilepath.rfind(L"\\");
				if (n != std::wstring::npos) {
					auto moduleFilename = moduleFilepath.substr(n + 1);
					if (moduleFilename != prevModuleFilename) {
						prevModuleFilename = moduleFilename;
						backtrace.push_back({ moduleFilename, {} });
					}
					backtrace.back().second.push_back({ moduleFilename, std::uint32_t((unsigned char*)stack[i] - moduleBase) });
				}
			}
		}
		return backtrace;
	}

	CRASH_HANDLING_API std::wstring BacktraceToString(const Backtrace_t& backtrace) {
		std::wstring backtraceStr;

		for (auto& [modulename, backtraceFrames] : backtrace) {
			for (auto& backtraceFrame : backtraceFrames) {
				backtraceStr += std::format(L"{} 0x{:08x} \n", backtraceFrame.moduleName, backtraceFrame.RVA);
			}
		}
		return backtraceStr;
	}

	Backtrace::Backtrace(int skipFrames)
		: backtrace{ CrashHandling::GetBacktrace(skipFrames) }
	{}
	Backtrace_t Backtrace::GetBacktrace() const {
		return backtrace;
	}
	std::wstring Backtrace::GetBacktraceStr() const {
		return CrashHandling::BacktraceToString(backtrace);
	}


	LONG WINAPI DefaultVectoredExceptionHandlerFirst(EXCEPTION_POINTERS* pep);
	LONG WINAPI DefaultVectoredExceptionHandlerLast(EXCEPTION_POINTERS* pep);
	LONG WINAPI DefaultUnhandledExceptionFilter(EXCEPTION_POINTERS* pep);

	CRASH_HANDLING_API void RegisterVectorHandler(PVECTORED_EXCEPTION_HANDLER handler) {
		LOG_FUNCTION_ENTER("RegisterVectorHandler(handler)");
		AddVectoredExceptionHandler(0, handler);
	}

	// SetUnhandledExceptionFilter need set for each thread,
	// so by default call RegisterDefaultCrashHandler from main (UI) thread.
	CRASH_HANDLING_API void RegisterDefaultCrashHandler(std::function<void(EXCEPTION_POINTERS*, ExceptionType)> crashCallback, int waitBeforeExitMs) {
		LOG_FUNCTION_ENTER("RegisterDefaultCrashHandler(...)");
		gCrashCallback = crashCallback;
		waitBeforeExit = waitBeforeExitMs;
		AddVectoredExceptionHandler(1, DefaultVectoredExceptionHandlerFirst); // additional guard for cathing c++ exception within clr environment
		AddVectoredExceptionHandler(0, DefaultVectoredExceptionHandlerLast);
		SetUnhandledExceptionFilter(&DefaultUnhandledExceptionFilter);
	}

	CRASH_HANDLING_API void GenerateCrashReport(EXCEPTION_POINTERS* pExceptionPtrs, const AdditionalInfo& additionalInfo, const std::wstring& runProtocolMinidumpWriter,
		const std::vector<std::pair<std::wstring, std::wstring>>& commandArgs, std::function<void(const std::wstring&)> callbackToRunProtocol)
	{
		LOG_FUNCTION_SCOPE("GenerateCrashReport(...)");
		auto processId = GetCurrentProcessId();
		auto threadId = GetCurrentThreadId();

		std::vector<std::pair<std::wstring, std::wstring>> params = {
			{L"-processId", std::to_wstring(processId)},
		};
		params.insert(params.end(), commandArgs.begin(), commandArgs.end());

		if (callbackToRunProtocol) {
			std::wstring protcolWithParams = runProtocolMinidumpWriter + L":\"" + H::CreateStringParams(params) + L"\"";
			LOG_FUNCTION_SCOPE("callbackToRunProtocol()");
			callbackToRunProtocol(protcolWithParams);
		}
		else {
			std::wstring protcolWithParams = L"/c start " + runProtocolMinidumpWriter + L":\"" + H::CreateStringParams(params) + L"\"";
			H::ExecuteCommandLineW(protcolWithParams, false, SW_HIDE);
		}

		auto packageFolder = H::PackageProvider::IsRunningUnderPackage() ? ComApi::GetPackageFolder() : H::ExePath();

		if (additionalInfo.appVersion.empty()) {
			const_cast<AdditionalInfo&>(additionalInfo).appVersion = H::PackageProvider::GetPackageVersion();
		}

		LOG_DEBUG("Send exception info to MinidumpWriter.exe ...");
		try {
			channelMinidump.Open(additionalInfo.channelName,
				[&](H::Channel<MiniDumpMessages>::Msg_t message, H::Channel<MiniDumpMessages>::WriteFunc Write) {
					switch (message->type) {
					case MiniDumpMessages::Connect: {
						std::string strData = H::WStrToStr(packageFolder);
						Write(MiniDumpMessages::PackageFolder, { strData.begin(), strData.end() });

						strData = H::WStrToStr(additionalInfo.appCenterId);
						Write(MiniDumpMessages::AppCenterId, { strData.begin(), strData.end() });

						strData = H::WStrToStr(additionalInfo.appVersion);
						Write(MiniDumpMessages::AppVersion, { strData.begin(), strData.end() });

						strData = H::WStrToStr(additionalInfo.appUuid);
						Write(MiniDumpMessages::AppUuid, { strData.begin(), strData.end() });

						strData = H::WStrToStr(additionalInfo.backtrace);
						Write(MiniDumpMessages::Backtrace, { strData.begin(), strData.end() });

						strData = H::WStrToStr(additionalInfo.exceptionMsg);
						Write(MiniDumpMessages::ExceptionMessage, { strData.begin(), strData.end() });

						auto crashInfo = std::make_shared<CrashInfo>();
						crashInfo->threadId = threadId;
						FillCrashInfoWithExceptionPointers(crashInfo, pExceptionPtrs);
						auto serializedData = SerializeCrashInfo(crashInfo);
						Write(MiniDumpMessages::ExceptionInfo, std::move(serializedData));
						break;
					}
					}

					return true;
				}, 7'000); // block this thread for 7s

			//channelMinidump.WaitFinishSendingMessage(MiniDumpMessages::ExceptionInfo);
			std::this_thread::sleep_for(std::chrono::milliseconds(7'000));
		}
		catch (H::PipeError err) {
			LOG_ERROR("Catch PipeError = {}", MagicEnum::ToString(err));
		}
		catch (...) {
			LOG_ERROR("Catch unrecognized exception");
			LogLastError;
			Dbreak;
		}
	}


	LONG WINAPI DefaultVectoredExceptionHandlerFirst(EXCEPTION_POINTERS* pExceptionPtrs) {
		//LOG_FUNCTION_ENTER("DefaultVectoredExceptionHandlerFirst(...)");

		switch (pExceptionPtrs->ExceptionRecord->ExceptionCode) {
		case EXCEPTION_ACCESS_VIOLATION:
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
		case EXCEPTION_DATATYPE_MISALIGNMENT:
		case EXCEPTION_FLT_DENORMAL_OPERAND:
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:
		case EXCEPTION_FLT_INEXACT_RESULT:
		case EXCEPTION_FLT_INVALID_OPERATION:
		case EXCEPTION_FLT_OVERFLOW:
		case EXCEPTION_FLT_STACK_CHECK:
		case EXCEPTION_ILLEGAL_INSTRUCTION:
		case EXCEPTION_IN_PAGE_ERROR:
		case EXCEPTION_INT_OVERFLOW:
			//LOG_DEBUG("[Crash exception]");
			wasCrashException = true;
			break;

		case ClrException: {
			//LOG_DEBUG("[ClrException]");
			break;
		}
		case CppException: {
			//LOG_DEBUG("[CppException]");
			break;
		}
		}

		return EXCEPTION_CONTINUE_SEARCH;
	}


	LONG WINAPI DefaultVectoredExceptionHandlerLast(EXCEPTION_POINTERS* pExceptionPtrs) {
		if (handledCrashException) {
			//LOG_WARNING("Crash exception already handled, ignore.");
			return EXCEPTION_CONTINUE_SEARCH; // ignore next exception
		}
		LOG_FUNCTION_ENTER("DefaultVectoredExceptionHandlerLast()");

		switch (pExceptionPtrs->ExceptionRecord->ExceptionCode) {
		case EXCEPTION_ACCESS_VIOLATION:
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
		case EXCEPTION_DATATYPE_MISALIGNMENT:
		case EXCEPTION_FLT_DENORMAL_OPERAND:
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:
		case EXCEPTION_FLT_INEXACT_RESULT:
		case EXCEPTION_FLT_INVALID_OPERATION:
		case EXCEPTION_FLT_OVERFLOW:
		case EXCEPTION_FLT_STACK_CHECK:
		case EXCEPTION_ILLEGAL_INSTRUCTION:
		case EXCEPTION_IN_PAGE_ERROR:
		case EXCEPTION_INT_DIVIDE_BY_ZERO:
		case EXCEPTION_INT_OVERFLOW:
			break;

		case ClrException: {
			LOG_DEBUG("[ClrException]");
			if (wasCrashException) {
				break;
			}
			else {
				LOG_DEBUG("[ClrException] continue search");
				return EXCEPTION_CONTINUE_SEARCH;
			}
		}

		case CppException: {
			LOG_DEBUG("[CppException]");
			if (wasCrashException) {
				break;
			}
			else {
				LOG_DEBUG("[CppException] continue search");
				return EXCEPTION_CONTINUE_SEARCH;
			}
		}

		default:
			LOG_DEBUG("[default] continue search");
			return EXCEPTION_CONTINUE_SEARCH;
		}

		handledCrashException = true;
		if (gCrashCallback) {
			gCrashCallback(pExceptionPtrs, ExceptionType::StructuredException);
		}
	}


	LONG WINAPI DefaultUnhandledExceptionFilter(EXCEPTION_POINTERS* pExceptionPtrs) {
		if (handledCrashException) {
			//LOG_WARNING("Unhandled exception already handled, ignore.");
			return EXCEPTION_CONTINUE_SEARCH; // ignore next exception
		}
		LOG_FUNCTION_ENTER("DefaultUnhandledExceptionFilter()");

		handledCrashException = true;
		if (gCrashCallback) {
			gCrashCallback(pExceptionPtrs, ExceptionType::UnhandledException);
		}
	}


	// Should init CrashHandler from main (UI) thread
	void CrashHandler::Init(std::wstring runProtocol, std::wstring appCenterId, std::wstring appUuid) {
		CrashHandlerBaseSingleton::CreateInstance(runProtocol, appCenterId, appUuid);
	}
	void CrashHandler::SetCrashCallback(H::Callback<void> crashCallback) {
		CrashHandlerBaseSingleton::GetInstance()->SetCrashCallback(crashCallback);
	}
	void CrashHandler::SetFinishCallback(H::Callback<void> finishCallback) {
		CrashHandlerBaseSingleton::GetInstance()->SetFinishCallback(finishCallback);
	}
	void CrashHandler::SetProtocolCommandArgs(std::vector<std::pair<std::wstring, std::wstring>> protocolCommandArgs) {
		CrashHandlerBaseSingleton::GetInstance()->SetProtocolCommandArgs(protocolCommandArgs);
	}
	void CrashHandler::SetRunProtocolWithParamsCallback(H::Callback<void, const std::wstring&> runProtocolWithParamsCallback) {
		CrashHandlerBaseSingleton::GetInstance()->SetRunProtocolWithParamsCallback(runProtocolWithParamsCallback);
	}
}