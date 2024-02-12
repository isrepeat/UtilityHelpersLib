#pragma once
#include "MappedFilesCollection.h"
#include <Helpers/TypeSwitch.h>

namespace HELPERS_NS {
    namespace FS {
        MappedFilesCollection::MappedFilesCollection(std::filesystem::path mappedRootPath, HELPERS_NS::Flags<Format> formatFlags)
            : totalSize{ 0 }
            , formatFlags{ formatFlags }
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

        bool MappedFilesCollection::IsPreserveDirectoryStructure() {
            return formatFlags.Has(Format::PreserveDirStructure);
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

            // TODO: Rewrite without paths comparison, write tests
            if (formatFlags.Has(Format::PreserveDirStructure)) {
                basePath = mappedRootPath / pathItem.mainItem->path.relative_path().remove_filename();

                // Add missing dirs to list
                auto newDirs = pathItem.mainItem->path.relative_path().remove_filename();
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
                        dirs.push_back({ pathItem.mainItem->path.root_path() / currentSubdir, currentSubdir });
                    }
                }
            }

            H::TypeSwitch(pathItem.mainItem.get(),
                [&basePath](FileItemWithMappedPath& item) {
                    basePath /= item.mappedPath;
                    return;
                }
                );

            switch (pathItem.type) {
            case PathItem::Type::File:
            case PathItem::Type::Directory:
                mappedFileItem = { pathItem.mainItem->path, basePath / pathItem.mainItem->path.filename() };
                break;
            case PathItem::Type::RecursiveEntry: {
                // Cut mainItem path part from recursiveItem (recursiveItem is a child item of mainItem)
                auto relativePathToMainItem = std::filesystem::relative(pathItem.recursiveItem->path, pathItem.mainItem->path);
                mappedFileItem = { pathItem.recursiveItem->path, basePath / pathItem.mainItem->path.filename() / relativePathToMainItem };
                break;
            }
            }

            switch (pathItem.ExpandType()) {
            case PathItem::Type::File:
                files.push_back(std::move(mappedFileItem));
                totalSize += std::filesystem::file_size(pathItem.mainItem->path);
                break;
            case PathItem::Type::Directory:
                dirs.push_back(std::move(mappedFileItem));
                break;
            }
        }

        void MappedFilesCollection::Complete() {
            if (formatFlags.Has(Format::RenameDuplicates)) {
                for (auto* collection : { &dirs, &files }) {
                    for (auto itemIt = collection->rbegin(); itemIt != collection->rend(); ++itemIt) {
                        auto pathTmp = std::move(itemIt->mappedPath); // Temporarily "remove" from list
                        HELPERS_NS::FS::RenameDuplicate(pathTmp, *collection, [&](const MappedFileItem& item) {
                            return item.mappedPath == pathTmp;
                            });
                        itemIt->mappedPath = pathTmp;
                    }
                }
            }

            if (formatFlags.Has(Format::KeepRelativeMappedPath)) { // remove root path
                for (auto& dir : dirs) {
                    dir.mappedPath = dir.mappedPath.relative_path();
                }
                for (auto& file : files) {
                    file.mappedPath = file.mappedPath.relative_path();
                }
            }
        }

        void GetFilesCollection(std::vector<FileItemWithMappedPath> fileItems, MappedFilesCollection& filesCollection) {
            std::vector<std::unique_ptr<FileItemBase>> fileItemsUniq;

            std::transform(fileItems.begin(), fileItems.end(), std::back_inserter(fileItemsUniq),
                [](const FileItemWithMappedPath& item) {
                    return std::make_unique<H::FS::FileItemWithMappedPath>(item);
                });

            GetFilesCollection<MappedFilesCollection>(std::move(fileItemsUniq), filesCollection);
        }
    }
}

std::ostream& operator<< (std::ostream& out, const std::vector<HELPERS_NS::FS::MappedFileItem>& mappedPathItems) {
    for (auto& file : mappedPathItems) {
        out << file.localPath << "\n";
        out << file.mappedPath << "\n\n";
    }
    return out;
}

std::ostream& operator<< (std::ostream& out, const HELPERS_NS::FS::MappedFilesCollection& mappedFilesCollection) {
    for (auto& file : mappedFilesCollection.GetFiles()) {
        out << file.localPath << "\n";
        out << file.mappedPath << "\n\n";
    }
    return out;
}