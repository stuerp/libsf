
/** $VER: SF2.h (2025.04.30) P. Stuer - SF2 data types **/

#pragma once

#include "pch.h"

#include "BaseTypes.h"

namespace sf
{

#pragma warning(disable: 4820) // x bytes padding

#pragma pack(push, 2)

struct sfPresetHeader
{
    char Name[20];
    uint16_t wPreset;           // The MIDI preset number which applies to this preset.
    uint16_t Bank;              // The MIDI bank number which applies to this preset.
    uint16_t ZoneIndex;         // Index in the preset’s zone list.
    uint32_t Library;           // Unused
    uint32_t Genre;             // Unused
    uint32_t Morphology;        // Unused
};

struct sfPresetBag
{
    uint16_t GeneratorIndex;    // Index to the preset zone's list of generators in the PGEN chunk.
    uint16_t ModulatorIndex;    // Index to the preset zone's list of modulators in the PMOD chunk.
};

struct sfModList
{
    uint16_t SrcOperator;       // Source of data for the modulator.
    uint16_t DstOperator;       // Destination of the modulator.
    int16_t Amount;             // The degree to which the source modulates the destination.
    uint16_t AmountSource;      // The degree to which the source modulates the destination is to be controlled by the specficied modulation source.
    uint16_t SourceTransform;   // Transform that will be applied to the modulation source before application to the modulator.
};

struct sfGenList
{
    uint16_t Operator;          // The operator
    uint16_t Amount;            // The value to be assigned to the generator
};

struct sfInst
{
    char Name[20];
    uint16_t ZoneIndex;         // Index to the instrument's zone list in the IBAG chunk.
};

struct sfInstBag
{
    uint16_t GeneratorIndex;    // Index to the instrument zone's list of generators in the IGEN chunk.
    uint16_t ModulatorIndex;    // Index to the instrument zone's list of modulators in the IMOD chunk.
};

struct sfInstModList
{
    uint16_t SrcOperator;       // Source of data for the modulator.
    uint16_t DstOperator;       // Destination of the modulator.
    int16_t Amount;             // The degree to which the source modulates the destination.
    uint16_t AmountSource;      // The degree to which the source modulates the destination is to be controlled by the specficied modulation source.
    uint16_t SourceTransform;   // Transform that will be applied to the modulation source before application to the modulator.
};

struct sfInstGenList
{
    uint16_t Operator;          // The operator
    uint16_t Amount;            // The value to be assigned to the generator
};

struct sfSample_v1
{
//  char Name[20];

    uint32_t Start;             // Index from the beginning of the sample data to the start of the sample (in sample data points).
    uint32_t End;               // Index from the beginning of the sample data to the end of the sample (in sample data points).
    uint32_t LoopStart;         // Index from the beginning of the sample data to the loop start of the sample (in sample data points).
    uint32_t LoopEnd;           // Index from the beginning of the sample data to the loop end of the sample (in sample data points).
};

struct sfSample_v2
{
    char Name[20];

    uint32_t Start;             // Index from the beginning of the sample data to the start of the sample (in sample data points).
    uint32_t End;               // Index from the beginning of the sample data to the end of the sample (in sample data points).
    uint32_t LoopStart;         // Index from the beginning of the sample data to the loop start of the sample (in sample data points).
    uint32_t LoopEnd;           // Index from the beginning of the sample data to the loop end of the sample (in sample data points).

    // SoundFont v2.x and later
    uint32_t SampleRate;        // Sample rate in Hz at which this sample was acquired.
     uint8_t Pitch;             // MIDI key number of the recorded pitch of the sample.
      int8_t PitchCorrection;   // Pitch correction in cents that should be applied to the sample on playback.
    uint16_t SampleLink;        // Index of the sample header of the associated right or left stereo sample for SampleTypes LeftSample or RightSample respectively.
    uint16_t SampleType;        // Mask 0xC000 indicates a ROM sample.
};

#pragma pack(pop)

#pragma warning(default: 4820) // x bytes padding

}
