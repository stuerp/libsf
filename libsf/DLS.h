
/** $VER: DLS.h (2025.03.22) P. Stuer - DLS data types **/

#pragma once

#include "pch.h"

#include <vector>
#include <unordered_map>

#include "SoundFontBase.h"

namespace sf::dls
{

#pragma warning(disable: 4820) // x bytes padding

/// <summary>
/// Represents a connection block.
/// </summary>
class connection_block_t
{
public:
    uint16_t Source;
    uint16_t Control;
    uint16_t Destination;
    uint16_t Transform;
    int32_t Scale;
};

/// <summary>
/// Represents an articulator.
/// </summary>
class articulator_t
{
public:
    std::vector<connection_block_t> ConnectionBlocks;
};

/// <summary>
/// Represents a wave link.
/// </summary>
class wave_link_t
{
public:
    uint16_t Options;
    uint16_t PhaseGroup;
    uint32_t Channel;
    uint32_t TableIndex;
};

/// <summary>
/// Represents the parameters of a sample loop.
/// </summary>
class wave_sample_loop_t
{
public:
    wave_sample_loop_t(uint32_t type, uint32_t start, uint32_t length)
    {
        Type   = type;
        Start  = start;
        Length = length;
    }

    static const uint32_t WLOOP_TYPE_FORWARD = 0;

    uint32_t Type;      // Loop type
    uint32_t Start;     // Start point of the loop in samples as an absolute offset from the beginning of the data in the "data" subchunk of the "wave-list" wave file chunk.
    uint32_t Length;    // Length of the loop in samples.
};

/// <summary>
/// Represents a wave sample.
/// </summary>
class wave_sample_t
{
public:
    static const uint32_t F_WSMP_NO_TRUNCATION  = 0x0011;
    static const uint32_t F_WSMP_NO_COMPRESSION = 0x0021;

    uint16_t UnityNote;     // MIDI note which will replay the sample at original pitch. (0..127)
    int16_t FineTune;       // Tuning offset from the UnityNote in 16 bit relative pitch
    int32_t Attenuation;    // Attenuation to be applied to this sample in 32 bit relative gain.
    uint32_t SampleOptions;

    std::vector<wave_sample_loop_t> Loops;
};

/// <summary>
/// Represents an instrument region.
/// </summary>
class region_t
{
public:
    region_t() { }

    region_t(uint16_t lowKey, uint16_t highKey, uint16_t lowVelocity, uint16_t highVelocity, uint16_t options, uint16_t keyGroup, uint16_t layer)
    {
        LowKey = lowKey;
        HighKey = highKey;
        LowVelocity = lowVelocity;
        HighVelocity = highVelocity;
        Options = options;
        KeyGroup = keyGroup;
        Layer = layer;
    }

    uint16_t LowKey;        // Key range
    uint16_t HighKey;
    uint16_t LowVelocity;   // Velocity range
    uint16_t HighVelocity;
    uint16_t Options;       // Synthesis options
    uint16_t KeyGroup;      // Key group for a drum instrument
    uint16_t Layer;         // Editing layer

    wave_sample_t WaveSample;
    wave_link_t WaveLink;

    std::vector<articulator_t> Articulators;
};

/// <summary>
/// Represents an instrument.
/// </summary>
class instrument_t
{
public:
    instrument_t() { }

    instrument_t(uint32_t regionCount, uint8_t bankMSB, uint8_t bankLSB, uint8_t program, bool isPercussion)
    {
        Regions.reserve(regionCount);

        BankMSB = bankMSB;
        BankLSB = bankLSB;
        Program = program;
        IsPercussion = isPercussion;
    }

    uint8_t BankMSB; // CC0
    uint8_t BankLSB; // CC32
    uint8_t Program;
    bool IsPercussion;

    std::vector<region_t> Regions;
    std::vector<articulator_t> Articulators;

    std::unordered_map<std::string, std::string> Infos;
};

class wave_t
{
public:
    wave_t() { }

    // Wave Format
    uint16_t FormatTag;
    uint16_t Channels;
    uint32_t SamplesPerSec;
    uint32_t AvgBytesPerSec;
    uint16_t BlockAlign;

    uint16_t BitsPerSample;

    wave_sample_t WaveSample;

    std::unordered_map<std::string, std::string> Infos;
};

/// <summary>
/// Represents a DLS-compliant sound font.
/// </summary>
class soundfont_t : public soundfont_base_t
{
public:
    soundfont_t() { }

    // Represents the version of the contents of the sound font.
    uint16_t Major;
    uint16_t Minor;
    uint16_t Revision;
    uint16_t Build;

    std::vector<instrument_t> Instruments;
    std::vector<wave_t> Waves;
    std::vector<uint32_t> Cues;

    std::unordered_map<std::string, std::string> Infos;

private:
};

#pragma warning(default: 4820) // x bytes padding

}
