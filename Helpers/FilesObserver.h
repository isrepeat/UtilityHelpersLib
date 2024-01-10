#pragma once
#include "common.h"
#include <filesystem>
#include <cassert>
#include <vector>
#include <string>

namespace HELPERS_NS {
    namespace FS {
        struct PathItem {
            enum class Type {
                File,
                Directory,
                RecursiveEntry,
            };

            Type ExpandType() const {
                if (type == PathItem::Type::RecursiveEntry) {
                    assert(!recursiveItem.empty() && "--> recursiveItem is empty!");

                    if (std::filesystem::is_regular_file(recursiveItem)) {
                        return Type::File;
                    }
                    else if (std::filesystem::is_directory(recursiveItem)) {
                        return Type::Directory;
                    }
                }
                return type;
            }

            Type type;
            std::filesystem::path mainItem;
            std::filesystem::path recursiveItem;
        };


        class IFilesCollection {
        public:
            virtual ~IFilesCollection() = default;

        protected:
            friend class FilesObserver;
            virtual void Initialize() = 0;
            virtual void HandlePathItem(const PathItem& pathItem) = 0;
            virtual void Complete() = 0;
        };


        class FilesObserver {
        private:
            FilesObserver() = delete;
            ~FilesObserver() = delete;

        public:
            template <typename TCollection>
            static void GetFilesCollection(const std::vector<std::filesystem::path>& filePaths, TCollection& filesCollection) {
                IFilesCollection& filesCollectionInterface = filesCollection;
                filesCollectionInterface.Initialize();

                for (auto& item : filePaths) {
                    if (std::filesystem::is_regular_file(item)) {
                        filesCollectionInterface.HandlePathItem(PathItem{ PathItem::Type::File, std::move(item) });
                    }
                    else if (std::filesystem::is_directory(item)) {
                        PathItem pathItem{ PathItem::Type::Directory, std::move(item) };
                        filesCollectionInterface.HandlePathItem(pathItem);

                        for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(item)) {
                            pathItem.type = PathItem::Type::RecursiveEntry;
                            pathItem.recursiveItem = dirEntry.path();
                            filesCollectionInterface.HandlePathItem(pathItem);
                        }
                    }
                }
                filesCollectionInterface.Complete();
            };
        };
    }
}