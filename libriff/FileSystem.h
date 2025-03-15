
/** $VER: FileSystem.h (2023.07.26) P. Stuer **/

#pragma once

#include <SDKDDKVer.h>
#include <windows.h>
#include <pathcch.h>
#include <shlwapi.h>

#pragma comment(lib, "pathcch")
#pragma comment(lib, "shlwapi")

/// <summary>
/// Encapsulates a WIN32_FIND_DATA structure:
/// </summary>
class FindData : public WIN32_FIND_DATA
{
public:
    FindData() : WIN32_FIND_DATA()
    {
    }

    bool IsDirectory() const noexcept { return (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY); }
};

/// <summary>
/// Implements an interface to the file system.
/// </summary>
class FileSystem
{
public:
    FileSystem() : _RootPath{} { }

    template <typename T> bool Enumerate(const WCHAR * pathName, const WCHAR * searchPattern, T&& processItem) noexcept;

private:
    template <typename T> bool ProcessItems(const WCHAR * pathName, T&& processItem) const noexcept;

private:
    WCHAR _RootPath[MAX_PATH];
};

/// <summary>
/// Enumerates the file system items starting at the specified path.
/// </summary>
template <typename T>
bool FileSystem::Enumerate(const WCHAR * pathName, const WCHAR * searchPattern, T&& processItem) noexcept
{
    ::wcscpy_s(_RootPath, _countof(_RootPath), pathName);

    WCHAR PathName[MAX_PATH];

    if (::PathIsDirectoryW(pathName))
    {
        ::PathCchCombine(PathName, _countof(PathName), pathName, searchPattern);
    }
    else
    {
        ::PathCchRemoveFileSpec(_RootPath, _countof(_RootPath));
        ::wcscpy_s(PathName, _countof(PathName), pathName); 
    }

    return ProcessItems(PathName, processItem);
}

template <typename T>
bool FileSystem::ProcessItems(const WCHAR * pathName, T&& processItem) const noexcept
{
    FindData fd;

    HANDLE hFile = ::FindFirstFileW(pathName, &fd);

    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    do
    {
        WCHAR PathName[MAX_PATH];

        ::PathCchCombineEx(PathName, _countof(PathName), _RootPath, fd.cFileName, PATHCCH_ALLOW_LONG_PATHS);

        processItem(PathName, fd);
    }
    while (::FindNextFileW(hFile, &fd));

    ::FindClose(hFile);

    return true;
}
