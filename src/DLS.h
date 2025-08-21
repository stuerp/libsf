
/** $VER: DLS.h (2025.08.20) P. Stuer - DLS data types **/

#pragma once

#include "pch.h"

#include "BaseTypes.h"
#include "Definitions.h"

namespace sf::dls
{

#pragma warning(disable: 4820) // x bytes padding

/// <summary>
/// Represents a connection block.
/// </summary>
class connection_block_t
{
public:
    connection_block_t() noexcept : Source(), Control(), Destination(), Transform(), Scale() { }

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
    wave_link_t() noexcept : Options(), PhaseGroup(), Channel(), CueIndex() { }

public:
    uint16_t Options;       // Specifies flag options for this wave link. (fusOptions)
    uint16_t PhaseGroup;    // Specifies a group number for samples which are phase locked. (usPhaseGroup)
    uint32_t Channel;       // Specifies the channel placement of the sample. This is used to place mono sounds within a stereo pair or for multi-track placement. (ulChannel)
    uint32_t CueIndex;      // Specifies the 0 based index of the cue entry in the wave pool table. (ulTableIndex)

    static const uint32_t WAVELINK_CHANNEL_LEFT   = 0x0001;
    static const uint32_t WAVELINK_CHANNEL_RIGHT  = 0x0002;

    static const uint16_t F_WAVELINK_PHASE_MASTER = 0x0001; // This link is the master in a group of phase locked wave links.
    static const uint16_t F_WAVELINK_MULTICHANNEL = 0x0002; // The Channel field provides the channel steering information and all the channel steering data in the articulation chunk should be ignored.
};

/// <summary>
/// Represents the parameters of a sample loop.
/// </summary>
class wave_sample_loop_t
{
public:
    wave_sample_loop_t(uint32_t type, uint32_t start, uint32_t length) noexcept : Type(type), Start(start), Length(length) { }

public:
    uint32_t Type;      // Loop type
    uint32_t Start;     // Start point of the loop in samples as an absolute offset from the beginning of the data in the "data" subchunk of the "wave-list" wave file chunk.
    uint32_t Length;    // Length of the loop in samples.

    static const uint32_t WLOOP_TYPE_FORWARD = 0; // Forward loop
    static const uint32_t WLOOP_TYPE_RELEASE = 1; // Loop and release (DLS 2.2)
};

/// <summary>
/// Represents a wave sample. (1.14.10 Wave Sample)
/// </summary>
class wave_sample_t
{
public:
    wave_sample_t() noexcept : UnityNote(60), FineTune(), Gain(), Options() { }

public:
    uint16_t UnityNote; // Specifies the MIDI note which will replay the sample at original pitch. This value ranges from 0 to 127 (a value of 60 represents Middle C, as defined by the MIDI specification).
    int16_t FineTune;   // Specifies the tuning offset from the UnityNote in 16 bit relative pitch.
    int32_t Gain;       // Specifies the gain to be applied to this sample in 32 bit relative gain units. This is used primarily to balance multi-sample splits.
    uint32_t Options;   // Specifies flag options for the digital audio sample. Flags are defined for disabling 16 bit to 8-bit truncation of samples and compression of samples by the driver.

    std::vector<wave_sample_loop_t> Loops;

    static const uint32_t F_WSMP_NO_TRUNCATION  = 0x0001; // The synthesis engine is not allowed to truncate the bit depth of the sample if it cannot synthesize at the bit depth of the digital audio.
    static const uint32_t F_WSMP_NO_COMPRESSION = 0x0002; // The synthesis engine is not allowed to use compression for the digital audio sample.
};

/// <summary>
/// Represents an instrument region.
/// </summary>
class region_t
{
public:
    region_t() noexcept : LowKey(), HighKey(), LowVelocity(), HighVelocity(), Options(), KeyGroup(), Layer() { }

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

public:
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

    static const uint16_t F_RGN_OPTION_SELFNONEXCLUSIVE = 0x0001; // If a second Note-On of the same note is received by the synthesis engine, then the second note will be played as well as the first.
};

/// <summary>
/// Represents an instrument.
/// </summary>
class instrument_t
{
public:
    instrument_t() noexcept : BankMSB(), BankLSB(), Program(), IsPercussion(false) { }

    instrument_t(uint32_t regionCount, uint8_t bankMSB, uint8_t bankLSB, uint8_t program, bool isPercussion)
    {
        Regions.reserve(regionCount);

        BankMSB = bankMSB;
        BankLSB = bankLSB;
        Program = program;
        IsPercussion = isPercussion;
    }

public:
    std::string Name;

    uint8_t BankMSB; // CC0
    uint8_t BankLSB; // CC32
    uint8_t Program;
    bool IsPercussion;

    std::vector<region_t> Regions;
    std::vector<articulator_t> Articulators;

    properties_t Properties;
};

/// <summary>
/// Represents a wave in the wave pool.
/// </summary>
class wave_t
{
public:
    wave_t() noexcept : FormatTag(WAVE_FORMAT_PCM), Channels(1), SamplesPerSec(), AvgBytesPerSec(), BlockAlign(), BitsPerSample(16) { }

public:
    std::string Name;

    // Wave Format
    uint16_t FormatTag;
    uint16_t Channels;
    uint32_t SamplesPerSec;
    uint32_t AvgBytesPerSec;
    uint16_t BlockAlign;

    uint16_t BitsPerSample;

    wave_sample_t WaveSample;
    std::vector<uint8_t> Data;

    properties_t Properties;
};

/// <summary>
/// Represents a DLS-compliant collection.
/// </summary>
class collection_t
{
public:
    collection_t() noexcept { }

public:
    properties_t Properties;

    // Represents the version of the contents of the sound font.
    uint16_t Major;
    uint16_t Minor;
    uint16_t Revision;
    uint16_t Build;

    std::vector<instrument_t> Instruments;
    std::vector<wave_t> Waves;
    std::vector<uint32_t> Cues;
};

#pragma warning(default: 4820) // x bytes padding

}
