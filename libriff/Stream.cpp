
/** $VER: Stream.cpp (2025.03.20) P. Stuer **/

#include "pch.h"

#include "Stream.h"
#include "Encoding.h"
#include "Win32Exception.h"

namespace riff
{

/// <summary>
/// Opens the stream.
/// </summary>
bool file_stream_t::Open(const std::wstring & filePath)
{
    _hFile = ::CreateFileW(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);

    if (_hFile == INVALID_HANDLE_VALUE)
        throw win32_exception(::FormatText("Failed to open file \"%s\" for reading", filePath.c_str()));

    return true;
}

/// <summary>
/// Closes the stream.
/// </summary>
void file_stream_t::Close() noexcept
{
    if (_hFile != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(_hFile);
        _hFile = INVALID_HANDLE_VALUE;
    }
}

/// <summary>
/// Reads a number of bytes into a buffer.
/// </summary>
void file_stream_t::Read(void * data, uint64_t size)
{
    DWORD BytesRead;

    if (::ReadFile(_hFile, data, (DWORD) size, &BytesRead, nullptr) == 0)
        throw win32_exception(::FormatText("Failed to read %llu bytes", size));

    if (BytesRead != size)
        throw win32_exception(::FormatText("Failed to read %llu bytes, got %u bytes", size, BytesRead));
}

/// <summary>
/// Skips the specified number of bytes.
/// </summary>
void file_stream_t::Skip(uint64_t size)
{
    if (size == 0)
        return;

    LARGE_INTEGER FilePointer = {};

    FilePointer.QuadPart = (LONGLONG) size;

    if (::SetFilePointerEx(_hFile, FilePointer, (LARGE_INTEGER *) &FilePointer, FILE_CURRENT) == 0)
        throw win32_exception(::FormatText("Failed to skip %llu bytes", size));
}

/// <summary>
/// Opens the stream.
/// </summary>
bool memory_stream_t::Open(const std::wstring & filePath, uint64_t offset, uint64_t size)
{
    _hFile = ::CreateFileW(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);

    if (_hFile == INVALID_HANDLE_VALUE)
        throw win32_exception(::FormatText("Failed to open file \"%s\" for reading", filePath.c_str()));

    _hMap = ::CreateFileMappingW(_hFile, NULL, PAGE_READONLY, 0, 0, NULL);

    if (_hMap == NULL)
        throw win32_exception("Failed to create file mapping");

    {
        ULARGE_INTEGER li = { };

        li.QuadPart = offset;

        _Data = (uint8_t *) ::MapViewOfFile(_hMap, FILE_MAP_READ, li.HighPart, li.LowPart, (SIZE_T) size);

        if (_Data == nullptr)
            throw win32_exception("Failed to map view of file");
    }

    if (size == 0)
    {
        LARGE_INTEGER li = { };

        if (!::GetFileSizeEx(_hFile, &li))
            throw win32_exception("Failed to get file size");

        size = (uint64_t) li.QuadPart;
    }

    _Curr = _Data;
    _Tail = _Data + size;

    return true;
}

/// <summary>
/// Opens the stream.
/// </summary>
bool memory_stream_t::Open(const uint8_t * data, uint64_t size)
{
    _Data = data;

    _Curr = _Data;
    _Tail = _Data + size;

    return true;
}

/// <summary>
/// Closes the stream.
/// </summary>
void memory_stream_t::Close() noexcept
{
    if (_hMap)
    {
        if (_Data)
        {
            ::UnmapViewOfFile(_Data);
            _Data = nullptr;
        }

        ::CloseHandle(_hMap);
        _hMap = NULL;
    }

    if (_hFile != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(_hFile);
        _hFile = INVALID_HANDLE_VALUE;
    }
}

}
