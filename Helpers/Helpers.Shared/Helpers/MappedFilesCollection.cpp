#pragma once
#include "MappedFilesCollection.h"

namespace HELPERS_NS {
    namespace FS {
        MappedFilesCollection::MappedFilesCollection(std::filesystem::path mappedRootPath, HELPERS_NS::Flags<Format> formatFlags)
            : totalSize{ 0 }
            , formatFlags{formatFlags}
            , mappedRootPath{ mappedRootPath }
            , preserveDirStructure{ false }
        {
        }

        MappedFilesCollection& MappedFilesCollection::operator=(const MappedFilesCollection& other) {
            if (this != &other) {
                dirs = other.dirs;
                files = other.files;
                totalSize = other.totalSize;
                mappedRootPath = other.mappedRootPath;
                preserveDirStructure = other.preserveDirStructure;
                Complete();
            }
            return *this;
        }

        void MappedFilesCollection::SetPreserveDirectoryStructure(bool preserve) {
            preserveDirStructure = preserve;
        }

        bool MappedFilesCollection::IsPreserveDirectoryStructure() {
            return preserveDirStructure;
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
            auto basePath = mappedRootPath;

            if (preserveDirStructure) {
                basePath = mappedRootPath / pathItem.mainItem.relative_path().remove_filename();

                // Add missing dirs to list
                auto newDirs = pathItem.mainItem.relative_path().remove_filename();
                std::filesystem::path currentSubdir;
                for (auto& subdir : newDirs) {
                    if (subdir.empty()) {
                        continue;
                    }

                    currentSubdir /= subdir;
                    auto existingDir = std::find_if(dirs.begin(), dirs.end(),
                        [&](MappedFileItem& item) {
                            return item.mappedPath == currentSubdir;
                        });

                    if (existingDir == dirs.end()) {
                        dirs.push_back({pathItem.mainItem.root_path() / currentSubdir, currentSubdir});
                    }
                }
            }

            switch (pathItem.type) {
            case PathItem::Type::File:
            case PathItem::Type::Directory:
                mappedFileItem = { pathItem.mainItem, basePath / pathItem.mainItem.filename() };
                break;
            case PathItem::Type::RecursiveEntry: {
                // Cut mainItem path part from recursiveItem (recursiveItem is a child item of mainItem)
                auto relativePathToMainItem = pathItem.recursiveItem.wstring().substr(pathItem.mainItem.wstring().size() + 1); // +1 - skip slash
                mappedFileItem = { pathItem.recursiveItem, basePath / pathItem.mainItem.filename() / relativePathToMainItem };
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
            if (formatFlags.Has(Format::RenameDuplicates)) {
                for (auto* collection : {&dirs, &files}) {
                    for (auto itemIt = collection->rbegin(); itemIt != collection->rend(); ++itemIt) {
                        auto pathTmp = std::move(itemIt->mappedPath); // Temporarily "remove" from list
                        HELPERS_NS::FS::RenameDuplicate(pathTmp, *collection, [&](const MappedFileItem& item) {
                            return item.mappedPath == pathTmp;
                            });
                        itemIt->mappedPath = pathTmp;
                    }
                }
            }

            if (formatFlags.Has(Format::RelativeMappedPath)) { // remove root path
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

std::ostream& operator<< (std::ostream& out, const HELPERS_NS::FS::MappedFilesCollection& mappedFilesCollection) {
    for (auto& file : mappedFilesCollection.GetFiles()) {
        out << "[" << file.localPath << ";  " << file.mappedPath << "] \n";
    }
    return out;
}