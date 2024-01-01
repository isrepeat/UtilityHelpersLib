#pragma once
#include "HWindows.h"
#include "Logger.h"
#include <vector>
#include <atomic>
#include <span>

namespace H {
    inline BOOL CloseHandleSafe(HANDLE h) {
        return h && h != INVALID_HANDLE_VALUE ? CloseHandle(h) : TRUE;
    }

    // TODO: rewrite this without blocking current thread
    template<typename T = uint8_t>
    void ReadFileAsync(HANDLE hFile, const std::atomic<bool>& stop, std::vector<T>& outBuffer, DWORD numberOfBytesToRead) {
        enum StatusIO {
            Error,
            Stopped,
            Completed,
        };
        StatusIO status;

        DWORD dwBytesRead = 0;
        OVERLAPPED stOverlapped = { 0 };

        auto outBufferOldSize = outBuffer.size();
        outBuffer.resize(outBufferOldSize + numberOfBytesToRead);

        // Read new data after outBuffer.data() + outBufferOldSize to save old data
        if (ReadFile(hFile, outBuffer.data() + outBufferOldSize, numberOfBytesToRead * sizeof(T), &dwBytesRead, &stOverlapped)) {
            // ReadFile completed synchronously ...
            // NOTE: if dwBytesRead == 0 it is mean that other side call WriteFile with zero data
            status = StatusIO::Completed;
        }
        else {
            auto lastErrorReadFile = GetLastError();
            switch (lastErrorReadFile) {
            case ERROR_IO_PENDING:
                while (true) {
                    // Check break conditions in while scope (if we chek it after while may be status conflicts)
                    if (stop) {
                        status = StatusIO::Stopped;
                        break;
                    }

                    if (GetOverlappedResult(hFile, &stOverlapped, &dwBytesRead, FALSE)) {
                        // ReadFile operation completed
                        status = StatusIO::Completed;
                        break;
                    }
                    else {
                        // pending ...
                        auto lastErrorGetOverlappedResult = GetLastError();
                        if (lastErrorGetOverlappedResult != ERROR_IO_INCOMPLETE) {
                            // remote side was closed or smth else
                            status = StatusIO::Error;
                            LogLastError;
                            break;
                        }
                    }
                }
                break;

            default:
                LogLastError;
                status = StatusIO::Error;
            }
        }

        switch (status) {
        case StatusIO::Error:
            throw std::exception("ReadFile error");
            break;

        case StatusIO::Stopped:
            throw std::exception("Was stopped signal");
            break;

        case StatusIO::Completed:
            if (outBufferOldSize + dwBytesRead == 0) {
                throw std::exception("Zero data was read: 'outBufferOldSize + dwBytesRead == 0'");
            }
        }

        outBuffer.resize((outBufferOldSize + dwBytesRead) / sizeof(T)); // truncate
    }


    template<typename T = uint8_t>
    void WriteFileAsync(HANDLE hFile, const std::atomic<bool>& stop, std::span<T> writeData) {
        enum StatusIO {
            Error,
            Stopped,
            Completed,
        };
        StatusIO status;

        DWORD cbWritten = 0;
        OVERLAPPED stOverlapped = { 0 };

        if (WriteFile(hFile, writeData.data(), writeData.size() * sizeof(T), &cbWritten, &stOverlapped)) {
            // WriteFile completed synchronously ...
            status = StatusIO::Completed;
        }
        else {
            auto lastErrorReadFile = GetLastError();
            switch (lastErrorReadFile) {
            case ERROR_IO_PENDING:
                while (true) {
                    // Check break conditions in while scope (if we chek it after while may be status conflicts)
                    if (stop) {
                        status = StatusIO::Stopped;
                        break;
                    }

                    if (GetOverlappedResult(hFile, &stOverlapped, &cbWritten, FALSE)) {
                        // ReadFile operation completed
                        status = StatusIO::Completed;
                        break;
                    }
                    else {
                        // pending ...
                        auto lastErrorGetOverlappedResult = GetLastError();
                        if (lastErrorGetOverlappedResult != ERROR_IO_INCOMPLETE) {
                            // remote side was closed or smth else
                            status = StatusIO::Error;
                            LogLastError;
                            break;
                        }
                    }
                }
                break;

            default:
                LogLastError;
                status = StatusIO::Error;
            }
        }

        switch (status) {
        case StatusIO::Error:
            throw std::exception("WriteFile error");
            break;

        case StatusIO::Stopped:
            throw std::exception("Was stopped signal");
            break;

        case StatusIO::Completed: // mb unreachable
            if (cbWritten == 0) {
                throw std::exception("Zero bytes written: 'cbWritten == 0'");
            }
        }
    }
}