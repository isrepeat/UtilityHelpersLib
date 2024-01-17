#pragma once
#include "common.h"
#include "FilesObserver.h"

namespace HELPERS_NS {
    namespace FS {
        struct MappedFileItem {
            std::filesystem::path localPath;
            std::filesystem::path mappedPath;
        };


        class MappedFilesCollection : public IFilesCollection {
        public:
            enum class Format {
                Default,
                RelativeMappedPath,
            };

            MappedFilesCollection(std::filesystem::path mappedRootPath, Format format = Format::Default);
            MappedFilesCollection(MappedFilesCollection&) = default;

            // After copying, internally calls Complete()
            MappedFilesCollection& operator=(const MappedFilesCollection& other);

            void SetPreserveDirectoryStructure(bool preserve);
            bool IsPreserveDirectoryStructure();

            const uint64_t GetSize() const;
            const std::vector<MappedFileItem>& GetDirs() const;
            const std::vector<MappedFileItem>& GetFiles() const;

            const MappedFileItem& operator[](int i) const;

        private:
            void Initialize() override;
            void HandlePathItem(const PathItem& pathItem) override;
            void Complete() override;

        private:
            uint64_t totalSize;
            const Format format;
            std::filesystem::path mappedRootPath;
            std::vector<MappedFileItem> dirs;
            std::vector<MappedFileItem> files;
            bool preserveDirStructure;
        };
    }
}