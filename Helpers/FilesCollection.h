#pragma once
#include "common.h"
#include "FilesObserver.h"

namespace HELPERS_NS {
    namespace FS {
        class FilesCollection : public IFilesCollection {
        public:
            FilesCollection() = default;

            void ReplaceRootPath(std::wstring newRootPath);

            const std::vector<std::filesystem::path>& GetDirs() const;
            const std::vector<std::filesystem::path>& GetFiles() const;
            const uint64_t GetSize() const;

        private:
            void Initialize() override;
            void HandlePathItem(const PathItem& pathItem) override;
            void Complete() override;

        private:
            uint64_t totalSize = 0;
            std::vector<std::filesystem::path> dirs;
            std::vector<std::filesystem::path> files;
        };
    }
}