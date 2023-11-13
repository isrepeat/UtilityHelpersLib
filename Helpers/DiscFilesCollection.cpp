#pragma once
#include "DiscFilesCollection.h"

namespace H {
    namespace FS {
        DiscFilesCollection::DiscFilesCollection(wchar_t discLetter, Format format)
            : totalSize{ 0 }
            , format{ format }
            , discRootPath{ discLetter + std::wstring(L":\\") }
        {
        }

        DiscFilesCollection& DiscFilesCollection::operator=(const DiscFilesCollection& other) {
            if (this != &other) {
                dirs = other.dirs;
                files = other.files;
                totalSize = other.totalSize;
                discRootPath = other.discRootPath;
                Complete();
            }
            return *this;
        }


        const std::vector<DiscFileItem>& DiscFilesCollection::GetDirs() const {
            return dirs;
        }

        const std::vector<DiscFileItem>& DiscFilesCollection::GetFiles() const {
            return files;
        }

        const uint64_t DiscFilesCollection::GetSize() const {
            return totalSize;
        }


        void DiscFilesCollection::Initialize() {
            dirs.clear();
            files.clear();
            totalSize = 0;
        }

        void DiscFilesCollection::HandlePathItem(const PathItem& pathItem) {
            DiscFileItem discFileItem;

            switch (pathItem.type) {
            case PathItem::Type::File:
            case PathItem::Type::Directory:
                discFileItem = { pathItem.mainItem, discRootPath / pathItem.mainItem.filename() };
                break;
            case PathItem::Type::RecursiveEntry: {
                // Cut mainItem path part from recursiveItem (recursiveItem is a child item of mainItem)
                auto relativePathToMainItem = pathItem.recursiveItem.wstring().substr(pathItem.mainItem.wstring().size() + 1); // +1 - skip slash
                discFileItem = { pathItem.recursiveItem, discRootPath / pathItem.mainItem.filename() / relativePathToMainItem };
                break;
            }
            }

            switch (pathItem.ExpandType()) {
            case PathItem::Type::File:
                files.push_back(std::move(discFileItem));
                totalSize += std::filesystem::file_size(pathItem.mainItem);
                break;
            case PathItem::Type::Directory:
                dirs.push_back(std::move(discFileItem));
                break;
            }
        }

        void DiscFilesCollection::Complete() {
            if (format == Format::RelativeDiscPath) { // remove root path
                for (auto& dir : dirs) {
                    dir.discPath = dir.discPath.relative_path();
                }
                for (auto& file : files) {
                    file.discPath = file.discPath.relative_path();
                }
            }
        }
    }
}