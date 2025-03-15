
/** $VER: IFFReader.h (2023.07.26) P. Stuer **/

#pragma once

#include <SDKDDKVer.h>
#include <windows.h>

#include <stdio.h>
#include <stdint.h>

#include "FOURCC.h"
#include "Exceptions.h"

struct ChunkHeader
{
    uint32_t Id;
    uint32_t Size;
};

#pragma warning(disable: 4820)
/// <summary>
/// Implements a reader for a RIFF file.
/// </summary>
class IFFReader
{
public:
    IFFReader() : _hFile(INVALID_HANDLE_VALUE), _Options(), _Header()
    {
    }

    virtual ~IFFReader()
    {
    }

    enum class Option
    {
        None = 0,
        OnlyMandatory = 1
    };

    bool Open(const WCHAR * filePath, Option options);

    virtual void Close() noexcept;

    bool ReadHeader(uint32_t & formType);

    template <typename T> bool ReadChunks(T&& processChunk);

    void Read(void * data, uint32_t size)
    {
        DWORD BytesRead;

        if (::ReadFile(_hFile, data, size, &BytesRead, nullptr) == 0)
            throw Win32Exception();

        if (BytesRead != size)
            throw RIFFException(L"Insufficient data");
    }

    void SkipChunk(const ChunkHeader & ch)
    {
        Skip(ch.Size);
    }

    void Skip(uint32_t size)
    {
        if (::SetFilePointer(_hFile, (LONG) size, nullptr, FILE_CURRENT) == INVALID_SET_FILE_POINTER)
            throw RIFFException(L"Insufficient data");
    }

protected:
    virtual bool HasMandatory()
    {
        return false;
    }

private:
    template <typename T> bool ReadChunks(uint32_t parentID, uint32_t parentSize, T&& processChunk);

private:
    uint64_t GetFilePointer()
    {
        LARGE_INTEGER FilePointer{};

        if (::SetFilePointerEx(_hFile, FilePointer, (LARGE_INTEGER *) &FilePointer, FILE_CURRENT) == 0)
            throw Win32Exception();

        return (uint64_t) FilePointer.QuadPart;
    }

protected:
    HANDLE _hFile;
    Option _Options;
    ChunkHeader _Header;
};

template <typename T> bool IFFReader::ReadChunks(T&& processChunk)
{
    return ReadChunks(_Header.Id, _Header.Size - 4, processChunk);
}

template <typename T> bool IFFReader::ReadChunks(uint32_t chunkId, uint32_t chunkSize, T&& processChunk)
{
    UNREFERENCED_PARAMETER(chunkId);

    while (chunkSize != 0)
    {
        ChunkHeader ch;

        Read(&ch, sizeof(ch));
        chunkSize -= sizeof(ch);

        ch.Size = ((ch.Size >> 24) & 0xFF) | ((ch.Size >> 8) & 0xFF00) | ((ch.Size << 8) & 0xFF0000) | ((ch.Size << 24) & 0xFF000000);

        switch (ch.Id)
        {
            case FOURCC_LIST:
            {
                uint32_t ListType;

                if (ch.Size < sizeof(ListType))
                    throw RIFFException(L"Invalid list chunk");

                Read(&ListType, sizeof(ListType));

                ReadChunks(ch.Id, ch.Size - sizeof(ListType), processChunk);
                break;
            }

            case FOURCC_CAT:
            {
                uint32_t CatType;

                if (ch.Size < sizeof(CatType))
                    throw RIFFException(L"Invalid cat chunk");

                Read(&CatType, sizeof(CatType));

                ReadChunks(ch.Id, ch.Size - sizeof(CatType), processChunk);
                break;
            }

            default:
            {
                processChunk(ch);
            }
        }

        chunkSize -= ch.Size; // This is different in RIFF.

        if (((ch.Size & 1) == 1) && ::SetFilePointer(_hFile, (LONG) 1, nullptr, FILE_CURRENT) == INVALID_SET_FILE_POINTER)
            return false;
    }

    return true;
}
