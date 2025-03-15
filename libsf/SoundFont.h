
/** $VER: SoundFont.h (2025.03.14) P. Stuer **/

#pragma once

#include "pch.h"

#include <map>

namespace sf
{

class soundfont_base_t
{
};

struct preset_t
{
    std::string Name;
    uint32_t Bank;
    uint32_t Program;
    uint32_t ZoneIndex;
};

struct preset_zone_t
{
    uint16_t GeneratorIndex;    // Index to the preset zone's list of generators in the PGEN chunk.
    uint16_t ModulatorIndex;    // Index to the preset zone's list of modulators in the PMOD chunk.
};

struct preset_zone_modulator_t
{
    uint16_t SrcOperator;       // Source of data for the modulator.
    uint16_t DstOperator;       // Destination of the modulator.
    int16_t Amount;             // The degree to which the source modulates the destination.
    uint16_t AmountSource;      // The degree to which the source modulates the destination is to be controlled by the specficied modulation source.
    uint16_t SourceTransform;   // Transform that will be applied to the modulation source before application to the modulator.
};

struct preset_zone_generator_t
{
    uint16_t Operator;          // The operator
    uint16_t Amount;            // The value to be assigned to the generator
};

struct instrument_t
{
    std::string Name;
    uint16_t ZoneIndex;         // Index to the instrument's zone list in the IBAG chunk.
};

struct instrument_zone_t
{
    uint16_t GeneratorIndex;    // Index to the instrument zone's list of generators in the IGEN chunk.
    uint16_t Modulatorndex;     // Index to the instrument zone's list of modulators in the IMOD chunk.
};

struct instrument_zone_modulator_t
{
    uint16_t SrcOperator;       // Source of data for the modulator.
    uint16_t DstOperator;       // Destination of the modulator.
    int16_t Amount;             // The degree to which the source modulates the destination.
    uint16_t AmountSource;      // The degree to which the source modulates the destination is to be controlled by the specficied modulation source.
    uint16_t SourceTransform;   // Transform that will be applied to the modulation source before application to the modulator.
};

struct instrument_zone_generator_t
{
    uint16_t Operator;          // The operator
    uint16_t Amount;            // The value to be assigned to the generator
};

struct sample_t
{
    std::string Name;
    uint32_t Start;             // Index from the beginning of the sample data to the start of the sample (in sample data points).
    uint32_t End;               // Index from the beginning of the sample data to the end of the sample (in sample data points).
    uint32_t LoopStart;         // Index from the beginning of the sample data to the loop start of the sample (in sample data points).
    uint32_t LoopEnd;           // Index from the beginning of the sample data to the loop end of the sample (in sample data points).
    uint32_t SampleRate;        // Sample rate in Hz at which this sample was acquired.
    uint8_t Pitch;              // MIDI key number of the recorded pitch of the sample.
    int8_t PitchCorrection;     // Pitch correction in cents that should be applied to the sample on playback.
    uint16_t SampleLink;        // Index of the sample header of the associated right or left stereo sample for SampleTypes LeftSample or RightSample respectively.
    uint16_t SampleType;        // Mask 0xC000 indicates a ROM sample.
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

class soundfont_t : public soundfont_base_t
{
public:
    soundfont_t() : Major(), Minor(), ROMMajor(), ROMMinor() { }

    uint16_t Major;
    uint16_t Minor;

    std::string SoundEngine;
    std::string ROM;
    uint16_t ROMMajor;
    uint16_t ROMMinor;

    std::vector<preset_t> Presets;
    std::vector<preset_zone_t> PresetZones;
    std::vector<preset_zone_modulator_t> PresetZoneModulators;
    std::vector<preset_zone_modulator_t> PresetZoneGenerators;

    std::vector<instrument_t> Instruments;
    std::vector<instrument_zone_t> InstrumentZones;
    std::vector<instrument_zone_modulator_t> InstrumentZoneModulators;
    std::vector<instrument_zone_modulator_t> InstrumentZoneGenerators;

    std::vector<sample_t> Samples;
    std::vector<uint8_t> SampleData;

    std::map<std::string, std::string> Tags;
};

class dls_t : public soundfont_base_t
{
public:
    dls_t() { }

private:
};

}
