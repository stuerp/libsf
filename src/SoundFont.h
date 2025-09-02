
/** $VER: Soundfont.h (2025.09.02) P. Stuer - Soundfont data types **/

#pragma once

#include "pch.h"

#include "BaseTypes.h"
#include "DLS.h"

#include <array>

namespace sf
{

#pragma warning(disable: 4820) // x bytes padding

// A keyboard full of sound. Typically the collection of samples and articulation data associated with a particular MIDI preset number.
class preset_t
{
public:
    preset_t() noexcept { preset_t(""); }
    preset_t(const std::string & name, uint16_t program = 0, uint16_t bank = 0, uint16_t zoneIndex = 0, uint32_t library = 0, uint32_t genre = 0, uint32_t morphology = 0) noexcept :
        Name(name), MIDIProgram(program), MIDIBank(bank), ZoneIndex(zoneIndex), Library(library), Genre(genre), Morphology(morphology) { }

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
    preset_zone_t() noexcept : GeneratorIndex(), ModulatorIndex() { }
    preset_zone_t(uint16_t generatorIndex, uint16_t modulatorIndex) noexcept : GeneratorIndex(generatorIndex), ModulatorIndex(modulatorIndex) { }

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
    instrument_zone_t() noexcept : GeneratorIndex(), ModulatorIndex() { }
    instrument_zone_t(uint16_t generatorIndex, uint16_t modulatorIndex) noexcept : GeneratorIndex(generatorIndex), ModulatorIndex(modulatorIndex) { }

public:
    uint16_t GeneratorIndex;    // Index in the instrument zone's list of generators.
    uint16_t ModulatorIndex;    // Index in the instrument zone's list of modulators.
};

class generator_t
{
public:
    generator_t() noexcept : Operator(), Amount() { }
    generator_t(uint16_t oper, uint16_t amount) noexcept : Operator(oper), Amount((int16_t) amount) { }

public:
    uint16_t Operator;          // The operator
    int16_t Amount;             // The value to be assigned to the generator
};

typedef uint16_t ModulatorOperator;
typedef uint16_t TransformOperator;

class modulator_t
{
public:
    modulator_t() noexcept { modulator_t(0, GeneratorOperator::Invalid, 0, 0, 0); }
    modulator_t(ModulatorOperator srcOper, GeneratorOperator dstOper, int16_t amount, ModulatorOperator srcOperAmt, TransformOperator transformOper) noexcept
    {
        SrcOper       = srcOper;
        DstOper       = dstOper;
        Amount        = amount;
        SrcOperAmt    = srcOperAmt;
        TransformOper = transformOper;
    }

public:
    ModulatorOperator SrcOper;     // Indicates the source of data for the modulator.
    GeneratorOperator DstOper;     // Indicates the destination of the modulator.
    int16_t Amount;                  // Indicates the degree to which the source modulates the destination.
    ModulatorOperator SrcOperAmt;  // Indicates the degree to which the source modulates the destination is to be controlled by the specified modulation source.
    TransformOperator TransformOper;   // Indicates that a transform of the specified type will be applied to the modulation source before application to the modulator. 
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

    void ConvertFrom(const dls::collection_t & collection);

    std::string DescribeGenerator(uint16_t generator, uint16_t amount) const noexcept;
    std::string DescribeModulatorSource(uint16_t modulator) const noexcept;
    std::string DescribeModulatorTransform(uint16_t modulator) const noexcept;
    std::string DescribeSampleType(uint16_t sampleType) const noexcept;

private:
    static void ConvertArticulators(const std::vector<dls::articulator_t> & articulators, std::vector<generator_t> & generators, std::vector<modulator_t> & modulators);
    static void ConvertConnectionBlockToModulator(const dls::connection_block_t & connectionBlock, std::vector<modulator_t> & modulators);
    static GeneratorOperator GetSpecialGeneratorOperator(const dls::connection_block_t & connectionBlock) noexcept;

    static void ConvertDLSSourceToModulatorOperator(uint16_t oper, ModulatorOperator & modulatorOperator) noexcept;
    static void ConvertDLSDestinationToGeneratorOperator(const dls::connection_block_t & connectionBlock, GeneratorOperator & dstOperator, int16_t dstAmount) noexcept;

    void GenerateALawTable() noexcept
    {
        for (size_t i = 0; i < 256; ++i)
            _ALawTable[i] = DecodeALawValue((uint8_t) i);
    }

    static constexpr int16_t DecodeALawValue(uint8_t value) noexcept
    {
        value ^= 0x55;

        int16_t Sign     = (value & 0x80) ? -1 : 1;
        int16_t Exponent = (value & 0x70) >> 4;
        int16_t Mantissa = value & 0x0F;
        int16_t Sample   = (Exponent > 0) ? ((Mantissa + 16) << (Exponent + 3)) : ((Mantissa << 4) + 8);

        return Sign * Sample;
    }

public:
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
    std::vector<generator_t> PresetGenerators;
    std::vector<modulator_t> PresetModulators;

    std::vector<instrument_t> Instruments;
    std::vector<instrument_zone_t> InstrumentZones;
    std::vector<generator_t> InstrumentGenerators;
    std::vector<modulator_t> InstrumentModulators;

    std::vector<sample_t> Samples;

    properties_t Properties;

private:
    std::array<int16_t, 256> _ALawTable;
};

#pragma warning(default: 4820) // x bytes padding

}
