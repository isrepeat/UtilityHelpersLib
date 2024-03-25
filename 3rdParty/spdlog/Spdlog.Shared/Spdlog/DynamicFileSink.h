#pragma once
#include <Helpers/FileSystem_inline.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/details/file_helper.h>
#include <spdlog/common.h>
#include <spdlog/details/os.h>

#include <Helpers/EventObject.h>

#include <mutex>
#include <string>
#include <string_view>
#include <filesystem>
#include <memory>

namespace LOGGER_NS {
    template<class Mutex>
    class DynamicFileSink : public spdlog::sinks::base_sink<Mutex> {
    public:
        explicit DynamicFileSink(
            const spdlog::filename_t& filename,
            bool truncate,
            const spdlog::file_event_handlers& event_handlers = {},
            const std::wstring& pauseLoggingEventName = L"")
            : fileHelper{event_handlers}
        {
            fileHelper.open(filename, truncate);
            if (!pauseLoggingEventName.empty()) {
                pauseLoggingEvent = std::make_unique<HELPERS_NS::EventObject>(pauseLoggingEventName);
            }
        }

        explicit DynamicFileSink(
            const spdlog::filename_t& filename,
            bool truncate,
            const std::wstring& pauseLoggingEventName)
            : DynamicFileSink(filename, truncate, {}, pauseLoggingEventName)
        {
        }

        void SwitchFile() {
            std::scoped_lock lk(this->mutex_);
            std::filesystem::path filePath(fileHelper.filename());

            if (IsAltFile(filePath)) {
                SetFilenameInternal(GetInitialPath(filePath));
            } else {
                SetFilenameInternal(GetAltPath(filePath));
            }
        }

        spdlog::filename_t GetFilename() const {
            return fileHelper.filename();
        }

        void SetFilename(const spdlog::filename_t& filename, bool truncate = false) {
            std::scoped_lock lk(this->mutex_);
            SetFilenameInternal(filename, truncate);
        }

        // Pick which file to write to, depending on modification time
        static std::filesystem::path PickLogFile(const std::filesystem::path& path) {
            auto altPath = GetAltPath(path);
            auto initialPath = GetInitialPath(path);

            bool initialExists = std::filesystem::exists(initialPath);
            bool altExists = std::filesystem::exists(altPath);

            // One of the files or both of them are not present
            if (!initialExists && !altExists) {
                return initialPath;
            } else if (!initialExists && altExists) {
                return altPath;
            } else if (initialExists && !altExists) {
                return initialPath;
            }

            // Both files are present, choose which one to write to by modification time
			auto initialTime = std::filesystem::last_write_time(initialPath);
			auto altTime = std::filesystem::last_write_time(altPath);

			// Alt file is newer, so use it and delete regular file
			if (altTime.time_since_epoch().count() > initialTime.time_since_epoch().count()) {
				std::filesystem::remove(initialPath);
				return altPath;
			} else {
				std::filesystem::remove(altPath);
			}

            return initialPath;
        }

    protected:
        virtual void sink_it_(const spdlog::details::log_msg& msg) override {
            if (pauseLoggingEvent) {
                pauseLoggingEvent->Wait();
            }

            if (!std::filesystem::exists(fileHelper.filename())) {
                SwitchFile();
            }

            spdlog::memory_buf_t formatted;
            spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
            fileHelper.write(formatted);
        }

        virtual void flush_() override {
            fileHelper.flush();
        }

    private:
        void SetFilenameInternal(const spdlog::filename_t& filename, bool truncate = false) {
            fileHelper.close();
            fileHelper.open(filename, truncate);
        }

        static bool IsAltFile(const std::filesystem::path& filePath) {
            return filePath.filename().wstring()._Starts_with(altFilePrefix);
        }

        static std::filesystem::path GetInitialPath(const std::filesystem::path& filePath) {
            if (!IsAltFile(filePath)) {
                return filePath;
            }

            std::filesystem::path initialFileName(filePath.filename().wstring().erase(0, altFilePrefix.size()));
            std::filesystem::path initialFilePath = std::filesystem::path(filePath).remove_filename() / initialFileName;
            return initialFilePath;
        }

        static std::filesystem::path GetAltPath(const std::filesystem::path& filePath) {
            if (IsAltFile(filePath)) {
                return filePath;
            }

            std::filesystem::path altFileName(std::wstring(altFilePrefix) + filePath.filename().wstring());
            auto altFilePath = std::filesystem::path(filePath).remove_filename() / altFileName;
            return altFilePath;
        }

        static constexpr const std::wstring_view altFilePrefix{L"_"};
        spdlog::details::file_helper fileHelper;
        std::unique_ptr<HELPERS_NS::EventObject> pauseLoggingEvent;
    };

    using DynamicFileSinkMt = DynamicFileSink<std::mutex>;
}