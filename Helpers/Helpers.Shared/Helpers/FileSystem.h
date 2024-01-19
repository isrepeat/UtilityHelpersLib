#pragma once
#include "common.h"
#if COMPILE_FOR_DESKTOP
#include "HWindows.h"
#include "FileSystem_inline.h"
#include <functional>
#include <vector>
#include <string>

namespace HELPERS_NS {
    namespace FS {
        struct FileHeader {
            std::filesystem::path fileItem;
            FILE_BASIC_INFO basicInfo;
            uint64_t fileSize;
        };

        /* ------------------- */
        /*     Read / Write    */
        /* ------------------- */
        bool RemoveFile(const std::wstring& filename);
        void RenameFile(const std::wstring& oldFilename, const std::wstring& newFilename);

        std::vector<uint8_t> ReadFile(const std::filesystem::path& filename);
        void WriteFile(const std::filesystem::path& filename, const std::vector<uint8_t>& fileData);

        FileHeader ReadFileHeader(const std::filesystem::path& filename);
        std::vector<std::vector<uint8_t>> ReadFileChunks(const std::filesystem::path&, int chunkSize);

        void WriteFileWithHeader(FileHeader fileHeader, const std::vector<uint8_t>& fileData);

        /* ------------------- */
        /*     Path helpers    */
        /* ------------------- */
        void ConvertPathsWithPreferredSlashes(std::vector<std::filesystem::path>& paths);

        /* ------------------- */
        /*    Copy / Replace   */
        /* ------------------- */
        void CopyFirstItem(const std::filesystem::path& fromDir, const std::filesystem::path& toDir, const std::wstring& prefix = L"");
        void CopyDirContentTo(const std::filesystem::path& fromDir, const std::filesystem::path& toDir);

        /* ------------------- */
        /*   Filesystem info   */
        /* ------------------- */
        std::vector<std::filesystem::path> GetAllLogicalDrives();
    }
}
#endif