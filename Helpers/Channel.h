#pragma once
#include "HWindows.h"
#include <MagicEnum/MagicEnum.h>

#include "ConcurrentQueue.h"
#include "LocalPtr.hpp"
#include "Helpers.h"
#include "Logger.h"
#include "Thread.h"
#include "File.h"

#include <condition_variable>
#include <functional>
#include <string>
#include <vector>
#include <thread>
#include <sddl.h>
#include <queue>


#define BUFFER_PIPE 512

enum class PipeConnectionStatus {
    Error,
    Stopped,
    TimeoutConnection,
    Connected,
};

enum class PipeError {
    InvalidHandle,
    WriteError,
    ReadError,
};

PipeConnectionStatus WaitConnectPipe(IN HANDLE hPipe, const std::atomic<bool>& stop, int timeout = 0);
PipeConnectionStatus WaitOpenPipe(OUT HANDLE& hPipe, const std::wstring& pipeName, const std::atomic<bool>& stop, int timeout = 0);


template <typename T = uint8_t>
void ReadFromPipeAsync(HANDLE hNamedPipe, const std::atomic<bool>& stop, std::vector<T>& outBuffer) {
    try {
        return ReadFileAsync<T>(hNamedPipe, stop, outBuffer, BUFFER_PIPE);
    }
    catch (const std::exception& ex) {
        LOG_ERROR_D("Catch ReadFileAsync exception = {}", ex.what());
        throw PipeError::ReadError;
    }
}

template <typename T = uint8_t>
void WriteToPipeAsync(HANDLE hNamedPipe, const std::atomic<bool>& stop, std::span<T> writeData) {
    if (writeData.empty())
        return;

    try {
        return WriteFileAsync<T>(hNamedPipe, stop, writeData);
    }
    catch (const std::exception& ex) {
        LOG_ERROR_D("Catch WriteFileAsync exception = {}", ex.what());
        throw PipeError::WriteError;
    }
}


template<typename T>
H::LocalPtr<T> GetTokenInfo(HANDLE hToken, TOKEN_INFORMATION_CLASS typeInfo) {
    DWORD dwSize = 0;
    if (!GetTokenInformation(hToken, typeInfo, NULL, 0, &dwSize) && GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        return nullptr;
    }

    if (auto pTokenData = H::LocalPtr<T>(LocalAlloc(LPTR, dwSize))) {
        if (GetTokenInformation(hToken, typeInfo, pTokenData.get(), dwSize, &dwSize)) {
            return pTokenData;
        }
    }

    return nullptr;
}



// NOTE: EnumMsg must contain "Connect" msg and first "None" msg (fix in future)
// TODO: Add guard for multiple calls pulbic methods and for usage in them in multithreading
// TODO: fix when interrupt connection -> not process "None" msg (??? CHECK)
template<typename EnumMsg, typename T = uint8_t>
class Channel : public H::IThread {
    CLASS_FULLNAME_LOGGING_INLINE_IMPLEMENTATION(Channel);

public:
#pragma pack(push, 1)
    struct MessageDescriptor {
        uint32_t size = 0;
        uint32_t type = 0;
    };
#pragma pack(pop)

    struct Message {
        EnumMsg type;
        std::vector<T> payload;
    };

    struct MessageInternal {
        MessageDescriptor descriptor;
        Message message;
        uint32_t processedBytes = 0;
    };


    using Msg_t = std::shared_ptr<Message>;
    using WriteFunc = std::function<void(std::vector<T>&&, EnumMsg)>;

    Channel()
        : messagesQueue{ std::make_shared<H::ConcurrentQueue<Msg_t>>() }
    {
        LOG_FUNCTION_ENTER("Channel()");
        readStreamBuffer.reserve(BUFFER_PIPE * 2); // reserve double size
    }

    ~Channel() {
        LOG_FUNCTION_ENTER_C("~Channel()");
        StopChannel();
    }

    // Make security attributes to connect admin & user pipe
    void CreateForAdmin(const std::wstring& pipeName, std::function<bool(Msg_t, WriteFunc)> listenHandler, int timeout = 0) {
        LOG_FUNCTION_ENTER(L"CreateForAdmin(pipeName = {}, ...)", pipeName);

        auto pSecurityDescriptor = std::make_unique<SECURITY_DESCRIPTOR>();
        InitializeSecurityDescriptor(pSecurityDescriptor.get(), SECURITY_DESCRIPTOR_REVISION);
        SetSecurityDescriptorDacl(pSecurityDescriptor.get(), TRUE, (PACL)NULL, FALSE);
        pSecurityAttributes = std::make_unique<SECURITY_ATTRIBUTES>(SECURITY_ATTRIBUTES{ sizeof(SECURITY_ATTRIBUTES), pSecurityDescriptor.get(), FALSE });

        Create(pipeName, listenHandler, timeout);
    }

    // Make security attributes to connect UWP & desktop apps
    void CreateForUWP(const std::wstring& pipeName, std::function<bool(Msg_t, WriteFunc)> listenHandler, HANDLE hProcessUWP, int timeout = 0) {
        LOG_FUNCTION_ENTER(L"CreateForUWP(pipeName = {}, ...)", pipeName);

        pSecurityAttributes = std::make_unique<SECURITY_ATTRIBUTES>(SECURITY_ATTRIBUTES{ sizeof(SECURITY_ATTRIBUTES), NULL, FALSE });

        // SDDL_EVERYONE + SDDL_ALL_APP_PACKAGES + SDDL_ML_LOW
        if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(L"D:(A;;GA;;;WD)(A;;GA;;;AC)S:(ML;;;;;LW)", SDDL_REVISION_1, &pSecurityAttributes->lpSecurityDescriptor, 0)) {
            LogLastError;
            throw PipeError::InvalidHandle;
        }

        std::wstring formattedPipeName;

        HANDLE hToken = nullptr;
        if (OpenProcessToken(hProcessUWP, TOKEN_QUERY, &hToken)) {
            if (auto pTokenAppContainerInfo = GetTokenInfo<TOKEN_APPCONTAINER_INFORMATION>(hToken, ::TokenAppContainerSid)) {
                if (auto pTokenSessingId = GetTokenInfo<ULONG>(hToken, ::TokenSessionId)) {
                    H::LocalPtr<WCHAR> pStr;
                    if (ConvertSidToStringSidW(pTokenAppContainerInfo->TokenAppContainer, pStr.ReleaseAndGetAdressOf())) {
                        formattedPipeName = L"\\\\?\\pipe\\Sessions\\" + std::to_wstring(*pTokenSessingId) + L"\\AppContainerNamedObjects\\" + pStr.get() + L"\\" + pipeName;
                    }
                    else {
                        LogLastError;
                        throw PipeError::InvalidHandle;
                    }
                }
            }
        }

        Create(formattedPipeName, listenHandler, timeout);
    }

    void Create(const std::wstring& pipeName, std::function<bool(Msg_t, WriteFunc)> listenHandler, int timeout = 0) {
        LOG_FUNCTION_ENTER(L"Create(pipeName = {}, listenHandler, timeout = {})", pipeName, timeout);
        this->pipeName = pipeName;

        hNamedPipe = CreateNamedPipeW(
            pipeName.c_str(),
            PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            BUFFER_PIPE * sizeof(T), BUFFER_PIPE * sizeof(T), 5000, pSecurityAttributes.get());

        if (hNamedPipe == INVALID_HANDLE_VALUE) {
            throw PipeError::InvalidHandle;
        }

        closeChannel = false;
        threadChannel = std::thread([this, listenHandler, timeout, pipeName] {
            LOG_THREAD(this->pipeName + L" threadChannel Create");

            while (!closeChannel) {
                LOG_DEBUG("Waiting for connect...");
                auto status = WaitConnectPipe(hNamedPipe, closeChannel, timeout);
                LOG_DEBUG("WaitConnectPipe status = {}", magic_enum::enum_name(status));

                switch (status) {
                case PipeConnectionStatus::Connected: {
                    ListenRoutine(listenHandler);
                    break;
                }
                case PipeConnectionStatus::Error:
                case PipeConnectionStatus::Stopped:
                case PipeConnectionStatus::TimeoutConnection:
                    closeChannel = true;
                    stopSignal = true;
                    break;
                }

                StopSecondThreads();
                DisconnectNamedPipe(hNamedPipe);
            }
            });
    }



    void Open(const std::wstring& pipeName, std::function<bool(Msg_t, WriteFunc)> listenHandler, int timeout = 10'000) {
        LOG_FUNCTION_ENTER(L"Open(pipeName = {}, listenHandler, timeout = {})", pipeName, timeout);
        this->pipeName = pipeName;

        closeChannel = false;
        auto status = WaitOpenPipe(hNamedPipe, pipeName, closeChannel, timeout);
        LOG_DEBUG("WaitOpenPipe status = {}", magic_enum::enum_name(status));

        switch (status) {
        case PipeConnectionStatus::Connected: {
            break;
        }
        case PipeConnectionStatus::Error:
        case PipeConnectionStatus::Stopped:
        case PipeConnectionStatus::TimeoutConnection:
            throw PipeError::InvalidHandle;
            break;
        }

        threadChannel = std::thread([this, listenHandler] {
            LOG_THREAD(this->pipeName + L" threadChannel Open");
            ListenRoutine(listenHandler);
            });
    }


    /* ------------------------------------ */
    /*			 IThread methods			*/
    /* ------------------------------------ */
    void NotifyAboutStop() override {
        LOG_FUNCTION_ENTER_C("NotifyAboutStop()");

        StopListening();
        closeChannel = true;
    }

    void WaitingFinishThreads() override {
        LOG_FUNCTION_ENTER_C("WaitingFinishThreads()");

        if (threadChannel.joinable()) // wait finish channel thread where init interrupt thread 
            threadChannel.join();

        StopSecondThreads();

        if (hNamedPipe) {
            CloseHandle(hNamedPipe);
            hNamedPipe = nullptr;
        }
    }


    bool IsConnected() {
        return connected;
    }

    void SetInterruptHandler(std::function<void()> handler) {
        interruptHandler = handler;
    }

    void SetConnectHandler(std::function<void()> handler) {
        connectHandler = handler;
    }

    void StopChannel() {
        LOG_FUNCTION_ENTER_C("StopChannel()");
        NotifyAboutStop();
        WaitingFinishThreads();
    }

    void Write(std::vector<T>&& writeData, EnumMsg type) {
        std::unique_lock lk{ mxWrite };
        Message message{ type, std::move(writeData) };

        if (!connected) {
            pendingMessages.push_back(std::move(message));
            return;
        }

        WriteInternal(message);
    }


    void WritePendingMessages() {
        LOG_FUNCTION_ENTER_C("WritePendingMessages()");
        std::lock_guard lk{ mxWrite };
        for (auto& message : pendingMessages) {
            if (!WriteInternal(message))
                break;
        }
        pendingMessages.clear();
    }


    void ClearPendingMessages() {
        LOG_FUNCTION_ENTER_C("ClearPendingMessages()");
        std::lock_guard lk{ mxWrite };
        pendingMessages.clear();
    }

    bool WriteInternal(Message& message) {
        try {
            MessageDescriptor msgDecriptor{ message.payload.size(), static_cast<uint8_t>(message.type) };
            T messageSizeBuffer[sizeof(uint32_t)];
            T messageTypeBuffer[sizeof(uint32_t)];
            std::memcpy(&messageSizeBuffer, &msgDecriptor.size, sizeof(uint32_t));
            std::memcpy(&messageTypeBuffer, &msgDecriptor.type, sizeof(uint32_t));

            WriteToPipeAsync<T>(hNamedPipe, stopSignal, messageSizeBuffer);
            WriteToPipeAsync<T>(hNamedPipe, stopSignal, messageTypeBuffer);
            WriteToPipeAsync<T>(hNamedPipe, stopSignal, message.payload);

            if (waitedMessage != EnumMsg::None && waitedMessage == message.type) {
                cvFinishSendingMessage.notify_all();
            }

            return true;
        }
        catch (PipeError error) {
            LOG_ERROR_D("Catch PipeError = {}", magic_enum::enum_name(error));

            switch (error) {
            case PipeError::WriteError:
                LOG_ERROR_D("Write error! Stop channel.");
                break;
            };

            StopListening();
            return false;
        }
    }


    // TODO: add logic to wait for one of several messages
    void WaitFinishSendingMessage(EnumMsg type) {
        std::unique_lock lk{ mxWrite };

        if (hNamedPipe == INVALID_HANDLE_VALUE)
            return;

        waitedMessage = type;
        cvFinishSendingMessage.wait(lk); // now mutex unlocked and thread begin wait
        waitedMessage = EnumMsg::None;
    }

private:
    void ListenRoutine(std::function<bool(Msg_t, WriteFunc)> listenHandler) {
        LOG_FUNCTION_ENTER_C("ListenRoutine(listenHandler)");

        connected = true;
        stopSignal = false;
        messagesQueue->StartWork();

        threadConnect = std::thread([this] { // Not block current thread to avoid deadlock (TODO: replace on std::async)
            LOG_THREAD(this->pipeName + L" threadConnect");
            connectHandler();
            });

        Write({}, EnumMsg::Connect);

        threadRead = std::thread([this] {
            LOG_THREAD(this->pipeName + L" threadRead");
            ReadRoutine();
            });

        while (!stopSignal) {
            if (messagesQueue->IsWorking()) {
                if (auto msg = messagesQueue->Pop()) {
                    if (listenHandler(msg, bindedWriteFunc) == false) {
                        StopListening();
                        LOG_DEBUG_D("listenHandler returned 'false', stop listening.");
                    }
                }
            }
        }
        LOG_DEBUG_D("Finish ListenRoutine");

        threadInterrupt = std::thread([this] { // Not block current thread to avoid deadlock
            LOG_THREAD(this->pipeName + L" threadInterrupt");
            interruptHandler();
            });

        cvFinishSendingMessage.notify_all();
        connected = false; // if we here pipe not connected
    }

    void ReadRoutine() {
        LOG_FUNCTION_ENTER_C("ReadRoutine()");
        MessageInternal messageInternal;

        try {
            while (!stopSignal) {
                bool clearStreamBuffer = true;
                ReadFromPipeAsync<T>(hNamedPipe, stopSignal, readStreamBuffer);

                uint32_t processedBytes = 0;

                while (processedBytes < readStreamBuffer.size()) {
                    if (!ParseMessage(readStreamBuffer, processedBytes, messageInternal)) {
                        clearStreamBuffer = false;
                        break; // need more data to read descriptor (keep bytes in readStreamBuffer)
                    }
                    if (messageInternal.processedBytes == sizeof(MessageDescriptor) + messageInternal.descriptor.size) { // if message completed
                        messagesQueue->Push(std::make_shared<Message>(Message{
                            .type = static_cast<EnumMsg>(messageInternal.descriptor.type),
                            .payload = std::move(messageInternal.message.payload)
                            }));

                        messageInternal = {};
                    }
                }

                if (clearStreamBuffer) {
                    readStreamBuffer.clear(); // can clear buffer because payload was saved/moved in messagesQueue
                }
            }
        }
        catch (PipeError error) {
            LOG_ERROR_D("Catch PipeError = {}", magic_enum::enum_name(error));

            switch (error) {
            case PipeError::ReadError:
                LOG_ERROR_D("Read error! Stop channel.");
                break;
            };
            StopListening();
        }
    }


    bool ParseMessage(std::vector<T>& readStreamBuffer, uint32_t& totalProcessedBytes, MessageInternal& messageInternal) {
        bool descriptorWasRead = messageInternal.processedBytes >= sizeof(MessageDescriptor);
        bool payloadWasRead = descriptorWasRead && messageInternal.processedBytes == sizeof(MessageDescriptor) + messageInternal.descriptor.size;

        if (!descriptorWasRead) {
            auto restStreamBytes = readStreamBuffer.size() - totalProcessedBytes;
            if (restStreamBytes < sizeof(MessageDescriptor)) {
                readStreamBuffer.erase(readStreamBuffer.begin(), readStreamBuffer.begin() + totalProcessedBytes);
                totalProcessedBytes = readStreamBuffer.size();
                assert(restStreamBytes == totalProcessedBytes);
                return false;
            }

            messageInternal.descriptor = *reinterpret_cast<MessageDescriptor*>(readStreamBuffer.data() + totalProcessedBytes);
            messageInternal.processedBytes += sizeof(MessageDescriptor);
            totalProcessedBytes += sizeof(MessageDescriptor);
        }

        if (!payloadWasRead) {
            if (messageInternal.descriptor.size > 0) {
                auto remainingPayloadBytes = messageInternal.descriptor.size - messageInternal.message.payload.size();
                auto restStreamBytes = readStreamBuffer.size() - totalProcessedBytes;
                auto startIt = readStreamBuffer.begin() + totalProcessedBytes;

                auto readPayloadBytes = restStreamBytes < remainingPayloadBytes ? restStreamBytes : remainingPayloadBytes;

                messageInternal.message.payload.insert(messageInternal.message.payload.end(), startIt, startIt + readPayloadBytes);
                messageInternal.processedBytes += readPayloadBytes;
                totalProcessedBytes += readPayloadBytes;
            }
        }

        return true;
    }


    void StopListening() {
        LOG_FUNCTION_ENTER_C("StopListening()");
        stopSignal = true;
        messagesQueue->StopWork();
    }

    void StopSecondThreads() {
        if (threadConnect.joinable())
            threadConnect.join();

        if (threadRead.joinable())
            threadRead.join();

        if (threadInterrupt.joinable())
            threadInterrupt.join();
    }


private:
    std::mutex mxWrite;
    HANDLE hNamedPipe = nullptr;
    std::thread threadRead;
    std::thread threadChannel;
    std::thread threadConnect;
    std::thread threadInterrupt;
    std::wstring pipeName; // used just for logging
    std::atomic<bool> connected = false;
    std::atomic<bool> stopSignal = false;
    std::atomic<bool> closeChannel = false;
    std::function<void()> connectHandler = []() {};
    std::function<void()> interruptHandler = []() {};
    std::vector<Message> pendingMessages;
    std::vector<T> readStreamBuffer;

    std::atomic<EnumMsg> waitedMessage = EnumMsg::None;
    std::condition_variable cvFinishSendingMessage;
    std::unique_ptr<SECURITY_ATTRIBUTES> pSecurityAttributes;

    const WriteFunc bindedWriteFunc = std::bind(&Channel::Write, this, std::placeholders::_1, std::placeholders::_2);

    std::shared_ptr<H::ConcurrentQueue<Msg_t>> messagesQueue;
};