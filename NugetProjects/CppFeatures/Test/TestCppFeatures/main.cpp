#include <CppFeatures/ChannelNative.h>
#include "Helpers/Channel.h"

enum class ClientMessages {
	None,
	Connect,
	ActivatedByFile,
	ActivatedByProtocol,
	CloseRequest
};



void TestConnectWithUWP() {
	std::atomic<bool> working = true;
	std::wstring testChannelName = L"\\\\.\\pipe\\Local\\testChannel";
	auto processAccessFlags = PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_DUP_HANDLE;

	H::Channel<ClientMessages> channelClient;
	try {
		channelClient.SetFullClassName(L"channelClient");
		channelClient.SetInterruptHandler([&] {
			working = false;
			});

		auto processId = H::GetProcessID(L"TestCppFeatures.UWP.exe");	
		if (HANDLE hProcessUWP = OpenProcess(processAccessFlags, FALSE, processId)) {
			auto shortChannelName = std::filesystem::path(testChannelName).filename();

			channelClient.CreateForUWP(shortChannelName,
				[&](H::Channel<ClientMessages>::Msg_t message, H::Channel<ClientMessages>::WriteFunc Write) {
					switch (message->type) {
					case ClientMessages::Connect:
						break;

					case ClientMessages::ActivatedByFile: {
						LOG_DEBUG_S(&channelClient, L"[ActivatedByFile]");
						std::string text{ message->payload.begin(), message->payload.end() };
						LOG_DEBUG_D("message->payload: \"{}\"", text);
						break;
					}

					case ClientMessages::CloseRequest: {
						std::string text{ message->payload.begin(), message->payload.end() };
						LOG_DEBUG_D("message->payload: \"{}\"", text);
						Write(ClientMessages::ActivatedByProtocol, {});
						Sleep(200);
						working = false;
						return false;
					}
					}

					return true;
				},
				hProcessUWP
			);
		}

	}
	catch (H::PipeError err) {
		LOG_ERROR_S(&channelClient, "Catch PipeError = {}", magic_enum::enum_name(err));
	}

	while (working) {
		Sleep(100);
	}
}

void TestConnectWithWinUI() {
	std::atomic<bool> working = true;
	std::wstring testChannelName = L"\\\\.\\pipe\\testChannel";

	H::Channel<ClientMessages> channelClient;
	try {
		channelClient.SetFullClassName(L"channelClient");
		channelClient.SetInterruptHandler([&] {
			working = false;
			});

		channelClient.Create(testChannelName,
			[&](H::Channel<ClientMessages>::Msg_t message, H::Channel<ClientMessages>::WriteFunc Write) {
				switch (message->type) {
				case ClientMessages::Connect:
					break;

				case ClientMessages::ActivatedByFile: {
					LOG_DEBUG_S(&channelClient, L"[ActivatedByFile]");
					std::string text{ message->payload.begin(), message->payload.end() };
					LOG_DEBUG_D("message->payload: \"{}\"", text);
					break;
				}

				case ClientMessages::CloseRequest: {
					std::string text{ message->payload.begin(), message->payload.end() };
					LOG_DEBUG_D("message->payload: \"{}\"", text);
					Write(ClientMessages::ActivatedByProtocol, {});
					Sleep(200);
					working = false;
					return false;
				}
				}

				return true;
			}
		);
	}
	catch (H::PipeError err) {
		LOG_ERROR_S(&channelClient, "Catch PipeError = {}", magic_enum::enum_name(err));
	}

	while (working) {
		Sleep(100);
	}
}

int main() {
	H::Flags<lg::InitFlags> loggerInitFlags =
		lg::InitFlags::DefaultFlags |
		lg::InitFlags::EnableLogToStdout;

	lg::DefaultLoggers::Init(H::ExePath() / L"TestCppFeature.log", loggerInitFlags);

	//TestConnectWithUWP();
	TestConnectWithWinUI();

	return 0;
}