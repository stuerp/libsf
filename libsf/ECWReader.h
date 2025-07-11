
/** $VER: ECWReader.h (2025.05.01) P. Stuer - Implements a reader for a ECW-compliant wave set. **/

#pragma once

#include "pch.h"

#include "Waveset.h"

namespace ecw
{

/// <summary>
/// Implements a reader for an ECW file.
/// </summary>
class reader_t
{
public:
    reader_t() noexcept : _Stream()
    {
    }

    virtual ~reader_t()
    {
    }

    bool Open(riff::stream_t * stream)
    {
        _Stream = stream;

        return true;
    }

    virtual void Close() noexcept
    {
        if (_Stream != nullptr)
        {
            _Stream->Close();
            _Stream = nullptr;
        }
    }

    /// <summary>
    /// Reads a value.
    /// </summary>
    template <typename T> void Read(T & data)
    {
        _Stream->Read(&data, sizeof(data));
    }

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
    /// Move to the specified offset.
    /// </summary>
    virtual void Offset(uint32_t size)
    {
        _Stream->Offset(size);
    }

    void Process(waveset_t & ecw);

private:
    std::string ReadString(const char * data, size_t length)
    {
        return std::string(data, ::strnlen(data, length));
    }

protected:
    riff::stream_t * _Stream;

private:
};

}
