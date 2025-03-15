
/** $VER: RIFFReader.h (2025.03.14) P. Stuer **/

#pragma once

#include <SDKDDKVer.h>
#include <windows.h>

#include "FOURCC.h"

#include "Stream.h"
#include "Exception.h"

namespace riff
{

struct chunk_header_t
{
    uint32_t Id;
    uint32_t Size;
};

#pragma warning(disable: 4820)

/// <summary>
/// Implements a reader for a RIFF file.
/// </summary>
class reader_t
{
public:
    reader_t() : _Stream(), _Options(), _Header()
    {
    }

    virtual ~reader_t()
    {
    }

    enum class option_t
    {
        None = 0,
        OnlyMandatory = 1
    };

    bool Open(stream_t * stream, option_t options);

    virtual void Close() noexcept;

    virtual bool ReadHeader(uint32_t & formType);

    template <typename T> bool ReadChunks(T&& processChunk);
    template <typename T> bool ReadChunks(uint32_t parentID, uint32_t parentSize, T&& processChunk);

    /// <summary>
    /// Reads a number of bytes into a buffer.
    /// </summary>
    virtual void Read(void * data, uint32_t size)
    {
        _Stream->Read(data, size);
    }

    /// <summary>
    /// Skips the specified number of bytes.
    /// </summary>
    virtual void Skip(uint32_t size)
    {
        _Stream->Skip(size);
    }

    /// <summary>
    /// Skips the current chunk.
    /// </summary>
    virtual void SkipChunk(const chunk_header_t & ch)
    {
        Skip(ch.Size);
    }

protected:
    virtual bool HasMandatory()
    {
        return false;
    }

protected:
    stream_t * _Stream;
    option_t _Options;
    chunk_header_t _Header;
};

template <typename T> bool reader_t::ReadChunks(T&& processChunk)
{
    return ReadChunks(_Header.Id, _Header.Size - 4, processChunk);
}

template <typename T> bool reader_t::ReadChunks(uint32_t chunkId, uint32_t chunkSize, T&& processChunk)
{
    UNREFERENCED_PARAMETER(chunkId);

    while (chunkSize != 0)
    {
        chunk_header_t ch;

        Read(&ch, sizeof(ch));
        chunkSize -= sizeof(ch);

        processChunk(ch);
        chunkSize -= ch.Size + (ch.Size & 1);

        if ((ch.Size & 1) == 1)
            Skip(1);
    }

    return true;
}

}
