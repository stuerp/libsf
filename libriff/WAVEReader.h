
/** $VER: WAVEReader.h (2023.07.20) P. Stuer **/

#pragma once

#include "RIFFReader.h"

#include <mmreg.h>

#include <type_traits>

struct ChunkFMT
{
    uint16_t Tag;
    uint16_t ChannelCount;
    uint32_t SampleRate;
    uint32_t AvgBytesPerSec;
    uint16_t BlockAlign;
    uint16_t BitsPerSample;
};

struct ChunkSMPL
{
    uint32_t Manufacturer;
    uint32_t Product;
    uint32_t SamplePeriod;
    uint32_t MIDIUnityNote;
    uint32_t MIDIPitchFraction;
    uint32_t SMPTEFormat;
    uint32_t SampleLoops;
    uint32_t SamplerData;
};

enum class MandatoryChunks : uint8_t
{
    None    = 0,
    Format  = 1,
    Data    = 2
};

inline MandatoryChunks operator |(MandatoryChunks lhs, MandatoryChunks rhs)
{
    return (MandatoryChunks) ((std::underlying_type_t<MandatoryChunks>) lhs | (std::underlying_type_t<MandatoryChunks>) rhs);
}

inline MandatoryChunks operator &(MandatoryChunks lhs, MandatoryChunks rhs)
{
    return (MandatoryChunks) ((std::underlying_type_t<MandatoryChunks>) lhs & (std::underlying_type_t<MandatoryChunks>) rhs);
}

inline MandatoryChunks & operator |=(MandatoryChunks & lhs, MandatoryChunks rhs)
{
    return lhs = lhs | rhs;
}

#pragma warning(disable: 4820)
/// <summary>
/// Implements a reader for a RIFF WAVE file.
/// </summary>
class WAVEReader : public RIFFReader
{
public:
    WAVEReader() : RIFFReader(), _Format(), _SampleLength(), _Data(), _Size(), _HandledChunks()
    {
    }

    virtual ~WAVEReader()
    {
        Reset();
    }

    virtual void Reset()
    {
        delete[] _Data;
        _Data = nullptr;
        _Size = 0;
        _HandledChunks = MandatoryChunks::None;
    }

    uint16_t Format() const noexcept { return _Format.Tag; }
    uint16_t ChannelCount() const noexcept { return _Format.ChannelCount; }
    uint32_t SampleRate() const noexcept { return _Format.SampleRate; }
    uint32_t AvgBytesPerSec() const noexcept { return _Format.AvgBytesPerSec; }
    uint16_t BlockAlign() const noexcept { return _Format.BlockAlign; }
    uint16_t BitsPerSample() const noexcept { return _Format.BitsPerSample; }

    uint32_t SampleLength() const noexcept { return _SampleLength; }

    const uint8_t * Data() const noexcept { return _Data; }
    uint32_t Size() const noexcept { return _Size; }

    virtual bool HandleChunk(uint32_t chunkId, uint32_t chunkSize)
    {
        switch (chunkId)
        {
            case FOURCC_FMT:
                return HandleFMT(chunkSize);

            case FOURCC_FACT:
                return HandleFACT(chunkSize);
                break;

            case FOURCC_DATA:
                return HandleDATA(chunkSize);

            case FOURCC_SMPL:
                return HandleSMPL(chunkSize);
                break;

            default:
                return RIFFReader::HandleChunk(chunkId, chunkSize);
        }
    }

    virtual bool HasMandatory()
    {
        return _HandledChunks == (MandatoryChunks::Format | MandatoryChunks::Data);
    }

    bool HandleFMT(uint32_t chunkSize)
    {
        _HandledChunks |= MandatoryChunks::Format;

        {
            if (chunkSize < sizeof(_Format))
                return false;

            Read(&_Format, sizeof(_Format));

            chunkSize -= sizeof(_Format);
        }

        uint16_t ExtraSize;

        {
            if (chunkSize < sizeof(ExtraSize))
                return false;

            Read(&ExtraSize, sizeof(ExtraSize));

            chunkSize -= sizeof(ExtraSize);
        }

        switch (_Format.Tag)
        {
            case WAVE_FORMAT_PCM:
                break;
        
            case WAVE_FORMAT_ADPCM:
                break;
        }

        return (::SetFilePointer(_hFile, (LONG) chunkSize, nullptr, FILE_CURRENT) != INVALID_SET_FILE_POINTER);
    }

    bool HandleFACT(uint32_t chunkSize)
    {
        if (chunkSize != sizeof(_SampleLength))
            throw RIFFException(L"Insufficient data");

        Read(&_SampleLength, sizeof(_SampleLength));

        return true;
    }

    bool HandleDATA(uint32_t chunkSize)
    {
        _HandledChunks |= MandatoryChunks::Data;

        _Data = new uint8_t[chunkSize];
        _Size = chunkSize;

        Read(_Data, _Size);

        return true;
    }

    bool HandleSMPL(uint32_t chunkSize)
    {
        if (chunkSize != sizeof(_SampleLength))
            throw RIFFException(L"Insufficient data");

        ChunkSMPL ck;

        Read(&ck, sizeof(ck));

        return true;
    }

private:
    ChunkFMT _Format;
    uint32_t _SampleLength;
    uint8_t * _Data;
    uint32_t _Size;
    MandatoryChunks _HandledChunks;
};
