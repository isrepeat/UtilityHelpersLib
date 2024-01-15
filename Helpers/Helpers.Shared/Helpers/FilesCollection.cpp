#pragma once
#include "FilesCollection.h"

namespace HELPERS_NS {
    namespace FS {
        void FilesCollection::ReplaceRootPath(std::wstring newRootPath) {
            for (auto& dir : dirs) {
                dir = newRootPath / dir.relative_path();
            }
            for (auto& file : files) {
                file = newRootPath / file.relative_path();
            }
        }

        const std::vector<std::filesystem::path>& FilesCollection::GetDirs() const {
            return dirs;
        }

        const std::vector<std::filesystem::path>& FilesCollection::GetFiles() const {
            return files;
        }

        const uint64_t FilesCollection::GetSize() const {
            return totalSize;
        }

        void FilesCollection::Initialize() {
            dirs.clear();
            files.clear();
            totalSize = 0;
        }

        void FilesCollection::HandlePathItem(const PathItem& pathItem) {
            switch (pathItem.ExpandType()) {
            case PathItem::Type::File:
                files.push_back(pathItem.mainItem);
                totalSize += std::filesystem::file_size(pathItem.mainItem);
                break;
            case PathItem::Type::Directory:
                dirs.push_back(pathItem.mainItem);
                break;
            }
        }

        void FilesCollection::Complete() {
        }
    }
}