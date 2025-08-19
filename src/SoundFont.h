
/** $VER: Soundfont.h (2025.08.19) P. Stuer - Soundfont data types **/

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
    uint16_t MIDIProgram;
    uint16_t MIDIBank;
    uint16_t ZoneIndex;         // Index in the preset’s zone list.
    uint32_t Library;           // Unused
    uint32_t Genre;             // Unused
    uint32_t Morphology;        // Unused

public:
    auto operator<=>(const preset_t &) const = default;
};

// A subset of a preset containing an instrument reference and associated articulation data defined to play over certain key numbers and velocities. (Old SF1 name: Layer)
class preset_zone_t
{
public:
    uint16_t GeneratorIndex;    // Index in the preset zone's list of generators.
    uint16_t ModulatorIndex;    // Index in the preset zone's list of modulators.
};

// A collection of zones which represents the sound of a single musical instrument or sound effect set.
class instrument_t
{
public:
    std::string Name;
    uint16_t ZoneIndex;         // Index in the instrument's zone list.
};

// A subset of an instrument containing a sample reference and associated articulation data defined to play over certain key numbers and velocities. (Old SF1 name: Split)
class instrument_zone_t
{
public:
    uint16_t GeneratorIndex;    // Index in the instrument zone's list of generators.
    uint16_t ModulatorIndex;    // Index in the instrument zone's list of modulators.
};

class generator_t
{
public:
    uint16_t Operator;          // The operator
    uint16_t Amount;            // The value to be assigned to the generator
};

class modulator_t
{
public:
    uint16_t sfModSrcOper;      // Indicates the source of data for the modulator.
    uint16_t sfModDestOper;     // Indicates the destination of the modulator.
     int16_t modAmount;         // Indicates the degree to which the source modulates the destination.
    uint16_t sfModAmtSrcOper;   // Indicates the degree to which the source modulates the destination is to be controlled by the specified modulation source.
    uint16_t sfModTransOper;    // Indicates that a transform of the specified type will be applied to the modulation source before application to the modulator. 
};

enum SampleTypes : uint16_t
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

class sample_t
{
public:
    std::string Name;

    uint32_t Start;             // Index from the beginning of the sample data to the start of the sample (in sample data points).
    uint32_t End;               // Index from the beginning of the sample data to the end of the sample (in sample data points).
    uint32_t LoopStart;         // Index from the beginning of the sample data to the loop start of the sample (in sample data points).
    uint32_t LoopEnd;           // Index from the beginning of the sample data to the loop end of the sample (in sample data points).
    uint32_t SampleRate;        // Sample rate in Hz at which this sample was acquired.
     uint8_t Pitch;             // MIDI key number of the recorded pitch of the sample.
      int8_t PitchCorrection;   // Pitch correction in cents that should be applied to the sample on playback.
    uint16_t SampleLink;        // Index of the sample header of the associated right or left stereo sample for SampleTypes LeftSample or RightSample respectively.
    uint16_t SampleType;        // enum SampleTypes
};

/// <summary>
/// Represents an SBK/SF2/SF3-compliant bank.
/// </summary>
class bank_t
{
public:
    bank_t() noexcept : Major(), Minor(), ROMMajor(), ROMMinor() { }

    uint16_t Major;                         // SoundFont specification major version level to which the file complies.
    uint16_t Minor;                         // SoundFont specification minor version level to which the file complies.
    std::string SoundEngine;                // Wavetable sound engine for which the file was optimized (Default “EMU8000”).
    std::string Name;                       // Name of the SoundFont compatible bank.
    std::string ROMName;                    // Wavetable sound data ROM to which any ROM samples refer.
    uint16_t ROMMajor;                      // Wavetable sound data ROM major revision to which any ROM samples refer.
    uint16_t ROMMinor;                      // Wavetable sound data ROM minor revision to which any ROM samples refer.

    std::vector<std::string> SampleNames;   // SoundFont v1.0.0 only
    std::vector<uint8_t> SampleData;
    std::vector<uint8_t> SampleDataLSB;     // SoundFont v2.0.4 or later

    // Hydra

    std::vector<preset_t> Presets;
    std::vector<preset_zone_t> PresetZones;
    std::vector<modulator_t> PresetModulators;
    std::vector<generator_t> PresetGenerators;

    std::vector<instrument_t> Instruments;
    std::vector<instrument_zone_t> InstrumentZones;
    std::vector<generator_t> InstrumentGenerators;
    std::vector<modulator_t> InstrumentModulators;

    std::vector<sample_t> Samples;

    properties_t Properties;

    std::string DescribeGenerator(uint16_t generator, uint16_t amount) const noexcept;
    std::string DescribeModulatorSource(uint16_t modulator) const noexcept;
    std::string DescribeModulatorTransform(uint16_t modulator) const noexcept;
    std::string DescribeSampleType(uint16_t sampleType) const noexcept;
};

#pragma warning(default: 4820) // x bytes padding

}
