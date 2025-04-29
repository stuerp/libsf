
/** $VER: SoundFont.h (2025.04.29) P. Stuer - SoundFont data types **/

#pragma once

#include "pch.h"

#include "BaseTypes.h"

namespace sf
{

#pragma warning(disable: 4820) // x bytes padding

// A keyboard full of sound. Typically the collection of samples and articulation data associated with a particular MIDI preset number.
class preset_t
{
public:
    std::string Name;
    uint16_t Program;           // MIDI preset
    uint16_t Bank;              // MIDI bank
    uint16_t ZoneIndex;
};

// A subset of a preset containing an instrument reference and associated articulation data defined to play over certain key numbers and velocities. (SF1 name: Layer)
class preset_zone_t
{
public:
    uint16_t GeneratorIndex;    // Index to the preset zone's list of generators in the PGEN chunk.
    uint16_t ModulatorIndex;    // Index to the preset zone's list of modulators in the PMOD chunk.
};

class preset_zone_modulator_t
{
public:
    uint16_t SrcOperator;       // Source of data for the modulator.
    uint16_t DstOperator;       // Destination of the modulator.
    int16_t Amount;             // The degree to which the source modulates the destination.
    uint16_t AmountSource;      // The degree to which the source modulates the destination is to be controlled by the specficied modulation source.
    uint16_t SourceTransform;   // Transform that will be applied to the modulation source before application to the modulator.
};

class preset_zone_generator_t : public generator_base_t
{
public:
};

// A collection of zones which represents the sound of a single musical instrument or sound effect set.
class instrument_t : public instrument_base_t
{
public:
    instrument_t(const std::string & name, uint16_t zoneIndex) : instrument_base_t(name), ZoneIndex(zoneIndex) { }

    uint16_t ZoneIndex;         // Index to the instrument's zone list in the IBAG chunk.
};

// A subset of an instrument containing a sample reference and associated articulation data defined to play over certain key numbers and velocities. (SF1 name: Split)
class instrument_zone_t
{
public:
    uint16_t GeneratorIndex;    // Index to the instrument zone's list of generators in the IGEN chunk.
    uint16_t ModulatorIndex;     // Index to the instrument zone's list of modulators in the IMOD chunk.
};

class instrument_zone_modulator_t
{
public:
    uint16_t SrcOperator;       // Source of data for the modulator.
    uint16_t DstOperator;       // Destination of the modulator.
    int16_t Amount;             // The degree to which the source modulates the destination.
    uint16_t AmountSource;      // The degree to which the source modulates the destination is to be controlled by the specficied modulation source.
    uint16_t SourceTransform;   // Transform that will be applied to the modulation source before application to the modulator.
};

class instrument_zone_generator_t : public generator_base_t
{
public:
};

enum SampleTypes
{
    MonoSample      = 0x0001,
    RightSample     = 0x0002,
    LeftSample      = 0x0004,
    LinkedSample    = 0x0008,

    RomMonoSample   = 0x8001,
    RomRightSample  = 0x8002,
    RomLeftSample   = 0x8004,
    RomLinkedSample = 0x8008
};

class sample_t : public sample_base_t
{
public:
    uint32_t Start;             // Index from the beginning of the sample data to the start of the sample (in sample data points).
    uint32_t End;               // Index from the beginning of the sample data to the end of the sample (in sample data points).
    uint32_t LoopStart;         // Index from the beginning of the sample data to the loop start of the sample (in sample data points).
    uint32_t LoopEnd;           // Index from the beginning of the sample data to the loop end of the sample (in sample data points).
    uint32_t SampleRate;        // Sample rate in Hz at which this sample was acquired.
    uint8_t Pitch;              // MIDI key number of the recorded pitch of the sample.
    int8_t PitchCorrection;     // Pitch correction in cents that should be applied to the sample on playback.
    uint16_t SampleLink;        // Index of the sample header of the associated right or left stereo sample for SampleTypes LeftSample or RightSample respectively.
    uint16_t SampleType;        // enum SampleTypes
};

/// <summary>
/// Represents an SF2/SF3-compliant sound font.
/// </summary>
class soundfont_t : public soundfont_base_t
{
public:
    soundfont_t() noexcept : Major(), Minor(), ROMMajor(), ROMMinor() { }

    uint16_t Major;             // SoundFont specification major version level to which the file complies.
    uint16_t Minor;             // SoundFont specification minor version level to which the file complies.
    std::string SoundEngine;    // Wavetable sound engine for which the file was optimized (Default “EMU8000”).
    std::string BankName;       // Name of the SoundFont compatible bank.
    std::string ROMName;        // Wavetable sound data ROM to which any ROM samples refer.
    uint16_t ROMMajor;          // Wavetable sound data ROM major revision to which any ROM samples refer.
    uint16_t ROMMinor;          // Wavetable sound data ROM minor revision to which any ROM samples refer.

    std::vector<uint8_t> SampleData;

    // Hydra

    std::vector<preset_t> Presets;
    std::vector<preset_zone_t> PresetZones;
    std::vector<preset_zone_modulator_t> PresetZoneModulators;
    std::vector<preset_zone_generator_t> PresetZoneGenerators;

    std::vector<instrument_t> Instruments;
    std::vector<instrument_zone_t> InstrumentZones;
    std::vector<instrument_zone_modulator_t> InstrumentZoneModulators;
    std::vector<instrument_zone_generator_t> InstrumentZoneGenerators;

    std::vector<sample_t> Samples;
};

#pragma warning(default: 4820) // x bytes padding

}
