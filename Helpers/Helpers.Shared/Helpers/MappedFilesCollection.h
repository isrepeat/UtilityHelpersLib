#pragma once
#include "common.h"
#include "FilesObserver.h"
#include <Helpers/Flags.h>

namespace HELPERS_NS {
    namespace FS {
        struct MappedFileItem {
            std::filesystem::path localPath;
            std::filesystem::path mappedPath;
        };


        class MappedFilesCollection : public IFilesCollection {
        public:
            enum Format {
                None = 0x00,
                RelativeMappedPath = 0x01,
                RenameDuplicates = 0x02,
                Default = None
            };

            MappedFilesCollection(std::filesystem::path mappedRootPath,
                HELPERS_NS::Flags<Format> formatFlags = Format::Default);

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
            const HELPERS_NS::Flags<Format> formatFlags;
            std::filesystem::path mappedRootPath;
            std::vector<MappedFileItem> dirs;
            std::vector<MappedFileItem> files;
            bool preserveDirStructure;
        };
    }
}