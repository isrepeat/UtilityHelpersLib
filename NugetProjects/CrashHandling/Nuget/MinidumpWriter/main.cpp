#include "../CrashHandling.Shared/Internal/MiniDumpMessages.h"
#include <Helpers/RegistryManager.h>
#include <Helpers/FileSystem.h>
#include <Helpers/CrashInfo.h>
#include <Helpers/Channel.h>
#include <Helpers/Helpers.h>
#include <Helpers/Time.h>

#include "AppCenter.h"

#include <QtCore/QCoreApplication>
#include <Windows.h>
#include <filesystem>
#include <inttypes.h>
#include <dbghelp.h>
#include <iostream>
#include <string>
#include <memory>

#pragma comment (lib, "dbghelp.lib" )

#define VERSION L"1.2.1"
// TODO: rewrite all paths via std::filesystem::path

// NOTE: use uint8_t (instead whar_t) to be able send serialized structs
H::Channel<MiniDumpMessages> channelMinidump;
std::wstring crashReportFolder;
std::wstring packageFolder;
QString exceptionMessage;
QString appCenterId;
QString appVersion;
QString backtrace;
QString appUuid;

bool ChannelListenerHandler(H::Channel<MiniDumpMessages>::Msg_t message, H::Channel<MiniDumpMessages>::WriteFunc Write, HANDLE hProcess);
void GenerateMiniDump(std::shared_ptr<CrashInfo> crashInfo, HANDLE hProcess, int processId, std::wstring path);

template<typename Ret>
Ret Convert(const std::wstring& str) {
	if constexpr (std::is_same_v<Ret, std::wstring>) {
		return str;
	}
	else if constexpr (std::is_same_v<Ret, std::string>) {
		return H::WStrToStr(str);
	}
	else if constexpr (std::is_same_v<Ret, int>) {
		return std::stoi(str);
	}
	else if constexpr (std::is_same_v<Ret, bool>) {
		return str == L"true" ? true : false;
	}
	else {
		static_assert(false, "Type conversion not supported");
	}
}

template<typename T>
void SetValue(const std::map<std::wstring, std::wstring>& mapParams, const std::wstring& searchKey, T& value, bool isRequired = false) {
	if (mapParams.count(searchKey)) {
		value = Convert<T>(mapParams.at(searchKey));
	}
	else {
		if (isRequired) {
			LOG_ERROR(L"requeied parameter not found = '{}'", searchKey);
			throw;
		}
		// not modified, use default value
	}
}


#define PAR_DEBUG					L"-debug"
#define PAR_IS_UWP					L"-isUWP"
#define PAR_PROCESS_ID				L"-processId"
#define PAR_SLEEP_AT_START			L"-sleepAtStart"
#define PAR_PROCESS_ACCESS_FLAGS	L"-processAccessFlags"
#define PAR_MINIDUMP_CHANNEL_NAME	L"-minidumpChannelName"

// default values:
auto debug = false;
auto isUWP = false;
auto processId = 0;
auto sleepAtStart = 0;
auto minidumpChannelName = std::wstring{ L"\\\\.\\pipe\\Local\\channelDumpWriter" };
auto processAccessFlags = PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_DUP_HANDLE;


int _tmain(int argc, wchar_t* argv[]) {
	int _argc = 1;
	char _arg0[1] = "";
	char* _argv[2] = { _arg0, nullptr };
	QCoreApplication app(_argc, _argv);

	H::Flags<lg::InitFlags> loggerInitFlags =
		lg::InitFlags::DefaultFlags |
		lg::InitFlags::EnableLogToStdout |
		lg::InitFlags::CreateInPackageFolder;

	lg::DefaultLoggers::Init(L".\\MinidumpWriter.log", loggerInitFlags);

	LOG_DEBUG_D(L"Version = {}", VERSION);

	for (int i = 0; i < argc; i++) {
		LOG_DEBUG_D(L"arg [{}] = {} \n", i, argv[i]);
	}

	auto parseSuccessful = false;
	if (argc > 1) { // Parse for raw call where the parsed string is the second argument
		try {
			auto params = H::ParseArgsFromStringToMap(std::wstring{ argv[1] });

			LOG_DEBUG_D("\nlist incomming params:");
			for (auto& param : params) {
				LOG_DEBUG_D(L"param .. [{}, {}]", param.first, param.second);
			}

			debug = params.count(PAR_DEBUG);
			isUWP = params.count(PAR_IS_UWP);
			SetValue(params, PAR_PROCESS_ID, processId, true);
			SetValue(params, PAR_SLEEP_AT_START, sleepAtStart);
			SetValue(params, PAR_PROCESS_ACCESS_FLAGS, processAccessFlags);
			SetValue(params, PAR_MINIDUMP_CHANNEL_NAME, minidumpChannelName);

			parseSuccessful = true;
		}
		catch (std::out_of_range& ex) {
			LOG_ERROR("Catch std::out_of_range expection = {}", ex.what());
			parseSuccessful = false;
		}
		catch (...) {
			LOG_ERROR("Catch unrecognized exception");
			parseSuccessful = false;
		}
	}

	LOG_DEBUG_D("\nlist parsed params:");
	LOG_DEBUG_D("-debug = {}", debug);
	LOG_DEBUG_D("-isUWP = {}", isUWP);
	LOG_DEBUG_D("-processId = {}", processId);
	LOG_DEBUG_D("-sleepAtStart = {}", sleepAtStart);
	LOG_DEBUG_D("-processAccessFlags = 0b{:016b}", processAccessFlags);
	LOG_DEBUG_D(L"-minidumpChannelName = {}", minidumpChannelName);

	if (!debug) {
		::ShowWindow(::GetConsoleWindow(), SW_HIDE);
	}

	if (!parseSuccessful) {
		LOG_ERROR("wrong parsed");
		if (debug) {
			LOG_DEBUG_D("Sleep 8s and exit ...");
			Sleep(8000);
		}
		return 1;
	}


	Sleep(sleepAtStart);

	QObject::connect(&AppCenter::GetInstance(), &AppCenter::ReportSendingStatus, [](bool success) {
		if (success) {
			LOG_DEBUG_D("Report sent successful !!!");
			MessageBoxA(NULL, (LPCSTR)"Report sent successful", (LPCSTR)"Crash report", MB_ICONINFORMATION | MB_DEFBUTTON2);
		}
		else {
			LOG_ERROR("Report not sent !!!");
			MessageBoxA(NULL, (LPCSTR)"Report not sent", (LPCSTR)"Crash report", MB_ICONWARNING | MB_DEFBUTTON2);
		}

		Sleep(5'000); // wait some time that user can read message

		if (debug) {
			Sleep(10'000); // wait some time that developer can read console output
		}
		qApp->exit();
		});

	try {
		if (HANDLE hProcess = OpenProcess(processAccessFlags, FALSE, processId)) {
			if (isUWP) {
				auto shortChannelName = std::filesystem::path(minidumpChannelName).filename();
				channelMinidump.CreateForUWP(shortChannelName, std::bind(ChannelListenerHandler, std::placeholders::_1, std::placeholders::_2, hProcess), hProcess, 20'000);
			}
			else {
				channelMinidump.Create(minidumpChannelName, std::bind(ChannelListenerHandler, std::placeholders::_1, std::placeholders::_2, hProcess), 20'000);
			}
		}
	}
	catch (H::PipeError err) {
		LOG_ERROR("Catch PipeError = {}", MagicEnum::ToString(err));
	}
	catch (...) {
		LOG_ERROR("Catch unrecognized exception");
		LogLastError;
	}


	AppCenter::GetInstance(); // Create singleton in qt main thread

	LOG_DEBUG_D("qt event loop ...");
	return app.exec();
}


bool ChannelListenerHandler(H::Channel<MiniDumpMessages>::Msg_t message, H::Channel<MiniDumpMessages>::WriteFunc Write, HANDLE hProcess) {
	switch (message->type) {
	case MiniDumpMessages::Connect: {
		LOG_DEBUG_D("[PIPE] processId = {}", processId);
		break;
	}
	case MiniDumpMessages::PackageFolder: {
		auto strData = std::string{ message->payload.begin(), message->payload.end() };
		LOG_DEBUG_D("[PIPE] packageFolder = {}", strData);
		packageFolder = H::StrToWStr(strData);
		crashReportFolder = packageFolder + L"\\CrashReport";
		break;
	}
	case MiniDumpMessages::AppCenterId: {
		auto strData = std::string{ message->payload.begin(), message->payload.end() };
		LOG_DEBUG_D("[PIPE] appCenterId = {}", strData);
		appCenterId = QUuid::fromString(H::StrToWStr(strData)).toString(QUuid::StringFormat::WithoutBraces);
		break;
	}
	case MiniDumpMessages::AppVersion: {
		auto strData = std::string{ message->payload.begin(), message->payload.end() };
		LOG_DEBUG_D("[PIPE] appVersion = {}", strData);
		appVersion = QString::fromStdWString(H::StrToWStr(strData));
		break;
	}

	case MiniDumpMessages::AppUuid: {
		auto strData = std::string{ message->payload.begin(), message->payload.end() };
		LOG_DEBUG_D("[PIPE] appUuid = {}", strData);
		appUuid = QUuid::fromString(H::StrToWStr(strData)).toString(QUuid::StringFormat::WithoutBraces);
		break;
	}

	case MiniDumpMessages::Backtrace: {
		auto strData = std::string{ message->payload.begin(), message->payload.end() };
		LOG_DEBUG_D("[PIPE] backtrace =\n{}\n", strData);
		backtrace = QString::fromStdWString(H::StrToWStr(strData));
		break;
	}

	case MiniDumpMessages::ExceptionMessage: {
		auto strData = std::string{ message->payload.begin(), message->payload.end() };
		LOG_DEBUG_D("[PIPE] exception message = {}", strData);
		exceptionMessage = QString::fromStdWString(H::StrToWStr(strData));
		break;
	}

	case MiniDumpMessages::ExceptionInfo: {
		LOG_DEBUG_D("[PIPE] exception info:");
		auto crashInfo = DeserializeCrashInfo(message->payload);
		FixExceptionPointersInCrashInfo(crashInfo);

		LOG_DEBUG_D("[PIPE] crashInfo: size serialized bytes = {}", message->payload.size());
		LOG_DEBUG_D("[PIPE] crashInfo: number of exceptions = {}", crashInfo->numberOfExceptionRecords);
		LOG_DEBUG_D("[PIPE] crashInfo: hProcess = 0x{:08x}", (DWORD)hProcess);
		LOG_DEBUG_D("[PIPE] crashInfo: ProcessId = {} \n", processId);
		LOG_DEBUG_D("[PIPE] creating dump ...");
		GenerateMiniDump(crashInfo, hProcess, processId, crashReportFolder);

		LOG_DEBUG_D("[PIPE] dump created!");
		Write(MiniDumpMessages::DumpCreated, {}); // now crashing process may be closed

		auto productName = H::StrToWStr(H::RegistryManager::GetRegValue(H::HKey::LocalMachine, "HARDWARE\\DESCRIPTION\\System\\BIOS", "SystemProductName"));
		H::FS::WriteFile(crashReportFolder + L"\\" + appUuid.toStdWString() + L"_" + productName, {});

		QList attachmentDirs{
			QDir{QString::fromStdWString(crashReportFolder)},
		};

		LOG_DEBUG_D("[PIPE] send report to App Center ...");
		AppCenter::GetInstance().SetApplicationData(appCenterId, appUuid, appVersion);
		AppCenter::GetInstance().SendCrashReport(exceptionMessage, {}, backtrace, attachmentDirs);
		break;
	}
	}
	return true;
}


void GenerateMiniDump(std::shared_ptr<CrashInfo> crashInfo, HANDLE hProcess, int processId, std::wstring path) {
	if (!std::filesystem::exists(path))
		std::filesystem::create_directories(path);

	HANDLE hFile = CreateFileW((path + L"\\MiniDump.dmp").c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if ((hFile != NULL) && (hFile != INVALID_HANDLE_VALUE)) {
		MINIDUMP_EXCEPTION_INFORMATION mdei;
		mdei.ThreadId = crashInfo->threadId;
		mdei.ExceptionPointers = &crashInfo->exceptionPointers;
		mdei.ClientPointers = FALSE;

		if (mdei.ExceptionPointers == NULL) {
			LOG_WARNING(L"ExceptionPointers == 0");
		}

		MINIDUMP_TYPE mdt = (MINIDUMP_TYPE)(MiniDumpWithIndirectlyReferencedMemory | MiniDumpScanMemory);

		BOOL rv = MiniDumpWriteDump(hProcess, processId, hFile, mdt, (mdei.ExceptionPointers != NULL) ? &mdei : NULL, NULL, NULL);

		if (!rv) {
			LOG_ERROR(L"MiniDumpWriteDump failed");
			LogLastError;
		}
		else {
			//WriteDebug(L"Minidump created");
		}

		CloseHandle(hFile);
	}
	else {
		LOG_ERROR(L"CreateFile failed");
		LogLastError;
	}
}