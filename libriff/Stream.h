
/** $VER: Stream.h (2025.03.15) P. Stuer **/

#pragma once

#include <SDKDDKVer.h>
#include <windows.h>

#include <string>

#include "Exception.h"

#pragma warning(disable: 4820)

namespace riff
{

/// <summary>
/// Implements a stream.
/// </summary>
class stream_t
{
public:
    virtual ~stream_t()
    {
    }

    virtual void Close() noexcept = 0;

    /// <summary>
    /// Reads a number of bytes into a buffer.
    /// </summary>
    virtual void Read(void * data, uint64_t size) = 0;

    /// <summary>
    /// Skips the specified number of bytes.
    /// </summary>
    virtual void Skip(uint64_t size) = 0;
};

/// <summary>
/// Implements a stream.
/// </summary>
class file_stream_t : public stream_t
{
public:
    file_stream_t() : _hFile(INVALID_HANDLE_VALUE)
    {
    }

    virtual ~file_stream_t()
    {
        Close();
    }

    bool Open(const std::wstring & filePath);

    virtual void Close() noexcept;
    virtual void Read(void * data, uint64_t size);
    virtual void Skip(uint64_t size);

protected:
    HANDLE _hFile;
};

/// <summary>
/// Implements a stream.
/// </summary>
class memory_stream_t : public stream_t
{
public:
    memory_stream_t() : _hFile(INVALID_HANDLE_VALUE), _hMap(), _Data(), _Curr(), _Tail()
    {
    }

    bool Open(const std::wstring & filePath, uint64_t offset, uint64_t size);

    bool Open(const uint8_t * data, uint64_t size);

    virtual void Close() noexcept;

    virtual void Read(void * data, uint64_t size)
    {
        if (_Curr + size > _Tail)
            throw riff::exception("Insufficient data");

        ::memcpy(data, _Curr, (size_t) size);
        _Curr += size;
    }

    /// <summary>
    /// Skips the specified number of bytes.
    /// </summary>
    virtual void Skip(uint64_t size)
    {
        if (_Curr + size > _Tail)
            throw riff::exception("Insufficient data");

        _Curr += size;
    }

protected:
    HANDLE _hFile;
    HANDLE _hMap;

    const uint8_t * _Data;
    const uint8_t * _Curr;
    const uint8_t * _Tail;
};

}
