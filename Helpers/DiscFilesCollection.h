#pragma once
#include "FilesObserver.h"

namespace H {
    namespace FS {
        struct DiscFileItem {
            std::filesystem::path localPath;
            std::filesystem::path discPath;
        };

        class DiscFilesCollection : public IFilesCollection {
        public:
            enum class Format {
                Default,
                RelativeDiscPath,
            };

            DiscFilesCollection(wchar_t discLetter, Format format = Format::Default);
            DiscFilesCollection(DiscFilesCollection&) = default;

            // After copying, internally calls Complete()
            DiscFilesCollection& operator=(const DiscFilesCollection& other);

            const std::vector<DiscFileItem>& GetDirs() const;
            const std::vector<DiscFileItem>& GetFiles() const;
            const uint64_t GetSize() const;

        private:
            void Initialize() override;
            void HandlePathItem(const PathItem& pathItem) override;
            void Complete() override;

        private:
            uint64_t totalSize;
            const Format format;
            std::filesystem::path discRootPath;
            std::vector<DiscFileItem> dirs;
            std::vector<DiscFileItem> files;
        };
    }
}