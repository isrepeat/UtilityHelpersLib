#pragma once
#include "MappedFilesCollection.h"

namespace H {
    namespace FS {
        MappedFilesCollection::MappedFilesCollection(std::filesystem::path mappedRootPath, Format format)
            : totalSize{ 0 }
            , format{ format }
            , mappedRootPath{ mappedRootPath }
        {
        }

        MappedFilesCollection& MappedFilesCollection::operator=(const MappedFilesCollection& other) {
            if (this != &other) {
                dirs = other.dirs;
                files = other.files;
                totalSize = other.totalSize;
                mappedRootPath = other.mappedRootPath;
                Complete();
            }
            return *this;
        }

        const uint64_t MappedFilesCollection::GetSize() const {
            return totalSize;
        }

        const std::vector<MappedFileItem>& MappedFilesCollection::GetDirs() const {
            return dirs;
        }

        const std::vector<MappedFileItem>& MappedFilesCollection::GetFiles() const {
            return files;
        }

        const MappedFileItem& MappedFilesCollection::operator[](int i) const {
            return files[i];
        }

        

        void MappedFilesCollection::Initialize() {
            dirs.clear();
            files.clear();
            totalSize = 0;
        }

        void MappedFilesCollection::HandlePathItem(const PathItem& pathItem) {
            MappedFileItem mappedFileItem;

            switch (pathItem.type) {
            case PathItem::Type::File:
            case PathItem::Type::Directory:
                mappedFileItem = { pathItem.mainItem, mappedRootPath / pathItem.mainItem.filename() };
                break;
            case PathItem::Type::RecursiveEntry: {
                // Cut mainItem path part from recursiveItem (recursiveItem is a child item of mainItem)
                auto relativePathToMainItem = pathItem.recursiveItem.wstring().substr(pathItem.mainItem.wstring().size() + 1); // +1 - skip slash
                mappedFileItem = { pathItem.recursiveItem, mappedRootPath / pathItem.mainItem.filename() / relativePathToMainItem };
                break;
            }
            }

            switch (pathItem.ExpandType()) {
            case PathItem::Type::File:
                files.push_back(std::move(mappedFileItem));
                totalSize += std::filesystem::file_size(pathItem.mainItem);
                break;
            case PathItem::Type::Directory:
                dirs.push_back(std::move(mappedFileItem));
                break;
            }
        }

        void MappedFilesCollection::Complete() {
            if (format == Format::RelativeMappedPath) { // remove root path
                for (auto& dir : dirs) {
                    dir.mappedPath = dir.mappedPath.relative_path();
                }
                for (auto& file : files) {
                    file.mappedPath = file.mappedPath.relative_path();
                }
            }
        }
    }
}