
/** $VER: SoundFontWriter.cpp (2025.04.25) P. Stuer - Writes an SF2/SF3 compliant sound font. **/

#include "pch.h"

#define __TRACE

#include "libsf.h"

#include <functional>

using namespace sf;

#pragma region Internal

#pragma pack(push, 2)

struct _preset_header_t
{
    char Name[20];
    uint16_t Preset;            // The MIDI preset number which applies to this preset.
    uint16_t Bank;              // The MIDI bank number which applies to this preset.
    uint16_t ZoneIndex;         // Index in the preset’s zone list. 
    uint32_t Library;           // Unused
    uint32_t Genre;             // Unused
    uint32_t Morphology;        // Unused
};

struct _preset_zone_t
{
    uint16_t GeneratorIndex;    // Index to the preset zone's list of generators in the PGEN chunk.
    uint16_t ModulatorIndex;    // Index to the preset zone's list of modulators in the PMOD chunk.
};

struct _preset_zone_modulator_t
{
    uint16_t SrcOperator;       // Source of data for the modulator.
    uint16_t DstOperator;       // Destination of the modulator.
    int16_t Amount;             // The degree to which the source modulates the destination.
    uint16_t AmountSource;      // The degree to which the source modulates the destination is to be controlled by the specficied modulation source.
    uint16_t SourceTransform;   // Transform that will be applied to the modulation source before application to the modulator.
};

struct _preset_zone_generator_t
{
    uint16_t Operator;          // The operator
    uint16_t Amount;            // The value to be assigned to the generator
};

struct _instrument_t
{
    char Name[20];
    uint16_t ZoneIndex;         // Index to the instrument's zone list in the IBAG chunk.
};

struct _instrument_zone_t
{
    uint16_t GeneratorIndex;    // Index to the instrument zone's list of generators in the IGEN chunk.
    uint16_t ModulatorIndex;    // Index to the instrument zone's list of modulators in the IMOD chunk.
};

struct _instrument_zone_modulator_t
{
    uint16_t SrcOperator;       // Source of data for the modulator.
    uint16_t DstOperator;       // Destination of the modulator.
    int16_t Amount;             // The degree to which the source modulates the destination.
    uint16_t AmountSource;      // The degree to which the source modulates the destination is to be controlled by the specficied modulation source.
    uint16_t SourceTransform;   // Transform that will be applied to the modulation source before application to the modulator.
};

struct _instrument_zone_generator_t
{
    uint16_t Operator;          // The operator
    uint16_t Amount;            // The value to be assigned to the generator
};

struct _sample_header_t
{
    char Name[20];
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

#pragma pack(pop)

#pragma endregion

/// <summary>
/// Processes the complete SoundFont.
/// </summary>
void soundfont_writer_t::Process(const soundfont_writer_options_t & options, soundfont_t & sf)
{
    TRACE_RESET();
    TRACE_INDENT();

    TRACE_FORM(FOURCC_SFBK, 0);
    TRACE_INDENT();
    {
        WriteChunks(FOURCC_RIFF, FOURCC_SFBK, [this, &options, &sf]() -> uint32_t
        {
            uint32_t FormSize = 0;

            TRACE_LIST(FOURCC_INFO, 0);
            TRACE_INDENT();
            {
                FormSize += WriteChunks(FOURCC_LIST, FOURCC_INFO, [this, &options, &sf]() -> uint32_t
                {
                    uint32_t ListSize = WriteChunk(FOURCC_IFIL, [this, &options, &sf]() -> uint32_t
                    {
                        constexpr const uint16_t Version[] = { 2, 1 };

                        return Write(Version, sizeof(Version));
                    });

                    ListSize += WriteChunk(FOURCC_ISNG, [this, &options, &sf]() -> uint32_t
                    {
                        constexpr const char * Data = "EMU8000";
                        constexpr uint32_t Size = 8; // Include string terminator.

                        return Write(Data, Size);
                    });

                    for (const auto & [ ChunkId, Value ] : sf.Properties)
                    {
                        ListSize += WriteChunk(ChunkId, [this, &options, &sf, Value]() -> uint32_t
                        {
                            const char * Data = Value.c_str();
                            const uint32_t Size = (uint32_t) ::strlen(Data) + 1; // Include string terminator.

                            return Write(Data, Size);
                        });
                    }

                    return ListSize;
                });
            }
            TRACE_UNINDENT(); // LIST

            TRACE_LIST(FOURCC_SDTA, 0);
            TRACE_INDENT();
            {
                FormSize += WriteChunks(FOURCC_LIST, FOURCC_SDTA, [this, &options, &sf]() -> uint32_t
                {
                    return 0;
                });
            }
            TRACE_UNINDENT(); // LIST

            TRACE_LIST(FOURCC_PDTA, 0);
            TRACE_INDENT();
            {
                FormSize += WriteChunks(FOURCC_LIST, FOURCC_PDTA, [this, &options, &sf]() -> uint32_t
                {
                    return 0;
                });
            }
            TRACE_UNINDENT(); // LIST

            return FormSize;
        });
    }
    TRACE_UNINDENT(); // RIFF

    TRACE_UNINDENT(); // File

    // Update the chunk sizes.
    for (const auto & Marker : _Markers)
    {
        SetOffset(Marker.Offset);

        Write(Marker.Size);
    }

#ifdef later
    std::function<bool(const riff::chunk_header_t & ch)> ChunkHandler = [this, &options, &sf, &ChunkHandler](const riff::chunk_header_t & ch) -> bool
    {
        switch (ch.Id)
        {
            case FOURCC_LIST:
            {
                uint32_t ListType;

                if (ch.Size < sizeof(ListType))
                    throw sf::exception("Invalid list chunk");

                Read(&ListType, sizeof(ListType));

                TRACE_LIST(ListType, ch.Size);
                TRACE_INDENT();

                ReadChunks(ch.Id, ch.Size - sizeof(ListType), ChunkHandler);

                TRACE_UNINDENT();
                break;
            }

            // Mandatory sub-chunk identifying the SoundFont specification version level to which the file complies.
            case FOURCC_IFIL:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                Read(&sf.Major, sizeof(sf.Major));
                Read(&sf.Minor, sizeof(sf.Minor));

                #ifdef __TRACE
                ::printf("%*sSoundFont specification version: %d.%d\n", __TRACE_LEVEL * 2, "", sf.Major, sf.Minor);
                #endif

                TRACE_UNINDENT();
                break;
            }

            // Mandatory sub-chunk identifying the wavetable sound engine for which the file was optimized. 
            case FOURCC_ISNG:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                sf.SoundEngine.resize((size_t) ch.Size);

                Read((void *) sf.SoundEngine.c_str(), ch.Size);

                #ifdef __TRACE
                ::printf("%*sSound Engine: \"%s\"\n", __TRACE_LEVEL * 2, "", sf.SoundEngine.c_str());
                #endif

                TRACE_UNINDENT();
                break;
            }

            // Optional sub-chunk identifying a particular wavetable sound data ROM to which any ROM samples refer.
            case FOURCC_IROM:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                sf.ROM.resize((size_t) ch.Size);

                Read((void *) sf.ROM.c_str(), ch.Size);

                #ifdef __TRACE
                ::printf("%*sROM: \"%s\"\n", __TRACE_LEVEL * 2, "", sf.ROM.c_str());
                #endif

                TRACE_UNINDENT();
                break;
            }

            // Optional sub-chunk identifying the particular wavetable sound data ROM revision to which any ROM samples refer.
            case FOURCC_IVER:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                Read(&sf.ROMMajor, sizeof(sf.ROMMajor));
                Read(&sf.ROMMinor, sizeof(sf.ROMMinor));

                #ifdef __TRACE
                ::printf("%*sROM Version: %d.%d\n", __TRACE_LEVEL * 2, "", sf.ROMMajor, sf.ROMMinor);
                #endif

                TRACE_UNINDENT();
                break;
            }

            // Contains one or more samples of digital audio information in the form of linearly coded 16 bit, signed, little endian (least significant byte first) words.
            case FOURCC_SMPL:
            {
                TRACE_CHUNK(ch.Id, ch.Size);

                if (options.ReadSampleData)
                {
                    sf.SampleData.resize(ch.Size);

                    Read(sf.SampleData.data(), ch.Size);
                }
                else    
                    SkipChunk(ch);

                break;
            }

            // Contains the least significant byte counterparts to each sample data point contained in the smpl chunk. Note this means for every two bytes in the [smpl] sub-chunk there is a 1-byte counterpart in [sm24] sub-chunk.
            case FOURCC_SM24:
            {
                TRACE_CHUNK(ch.Id, ch.Size);

                SkipChunk(ch);
                break;
            }

            // Mandatory sub-chunk listing all presets within the SoundFont compatible file.
            case FOURCC_PHDR:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                _preset_header_t PresetHeader = { };
                const size_t PresetHeaderCount = ch.Size / sizeof(PresetHeader);

                for (size_t i = 0; i < PresetHeaderCount; ++i)
                {
                    Read(&PresetHeader, sizeof(PresetHeader));

                    std::string Name; Name.assign(PresetHeader.Name, _countof(PresetHeader.Name));

                    sf.Presets.push_back({ Name, PresetHeader.Bank, PresetHeader.Preset, PresetHeader.ZoneIndex });

                    #ifdef __TRACE
                    ::printf("%*s%5zu. %-20s, Bank %3d, Preset %3d, Zone %6d\n", __TRACE_LEVEL * 2, "", i + 1, Name.c_str(), PresetHeader.Bank, PresetHeader.Preset, PresetHeader.ZoneIndex);
                    #endif
                }

                TRACE_UNINDENT();
                break;
            }

            // Mandatory sub-chunk listing all preset zones within the SoundFont compatible file.
            case FOURCC_PBAG:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                preset_zone_t Zone = { };
                const size_t PresetZoneCount = ch.Size / sizeof(Zone);

                for (size_t i = 0; i < PresetZoneCount; ++i)
                {
                    Read(&Zone, sizeof(Zone));

                    sf.PresetZones.push_back({ Zone.GeneratorIndex, Zone.ModulatorIndex });

                    #ifdef __TRACE
                    ::printf("%*s%5zu. Generator %5d, Modulator %5d\n", __TRACE_LEVEL * 2, "", i + 1, Zone.GeneratorIndex, Zone.ModulatorIndex);
                    #endif
                }

                TRACE_UNINDENT();
                break;
            }

            // Mandatory sub-chunk listing listing all preset zone modulators within the SoundFont compatible file.
            case FOURCC_PMOD:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                _preset_zone_modulator_t Modulator = { };
                const size_t ModulatorCount = ch.Size / sizeof(Modulator);

                for (size_t i = 0; i < ModulatorCount; ++i)
                {
                    Read(&Modulator, sizeof(Modulator));

                    sf.PresetZoneModulators.push_back({ Modulator.SrcOperator, Modulator.DstOperator, Modulator.Amount, Modulator.AmountSource, Modulator.SourceTransform });

                    #ifdef __TRACE
                    ::printf("%*s%5zu. Src Op: 0x%04X, Dst Op: %2d, Amount: %6d, Amount Source: 0x%04X, Source Transform: 0x%04X\n", __TRACE_LEVEL * 2, "", i + 1,
                        Modulator.SrcOperator, Modulator.DstOperator, Modulator.Amount, Modulator.AmountSource, Modulator.SourceTransform);
                    #endif
                }

                TRACE_UNINDENT();
                break;
            }

            // Mandatory sub-chunk listing all preset zone generators for each preset zone within the SoundFont compatible file.
            case FOURCC_PGEN:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                _preset_zone_generator_t Generator = { };
                const size_t GeneratorCount = ch.Size / sizeof(Generator);

                for (size_t i = 0; i < GeneratorCount; ++i)
                {
                    Read(&Generator, sizeof(Generator));

                    sf.PresetZoneGenerators.push_back({ Generator.Operator, Generator.Amount });

                    #ifdef __TRACE
                    ::printf("%*s%5zu. Operator: 0x%04X, Amount: 0x%04X\n", __TRACE_LEVEL * 2, "", i + 1, Generator.Operator, Generator.Amount);
                    #endif
                }

                TRACE_UNINDENT();
                break;
            }

            // Mandatory sub-chunk listing all instruments within the SoundFont compatible file.
            case FOURCC_INST:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                _instrument_t Instrument = { };
                const size_t InstrumentCount = ch.Size / sizeof(Instrument);

                for (size_t i = 0; i < InstrumentCount; ++i)
                {
                    Read(&Instrument, sizeof(Instrument));

                    std::string Name; Name.assign(Instrument.Name, _countof(Instrument.Name));

                    sf.Instruments.push_back(instrument_t(Name, Instrument.ZoneIndex));

                    #ifdef __TRACE
                    ::printf("%*s%5zu. \"%-20s\", Zone %5d\n", __TRACE_LEVEL * 2, "", i + 1, Instrument.Name.c_str(), Instrument.ZoneIndex);
                    #endif
                }

                TRACE_UNINDENT();
                break;
            }

            // Mandatory sub-chunk listing all instrument zones within the SoundFont compatible file.
            case FOURCC_IBAG:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                _instrument_zone_t Zone = { };
                const size_t InstrumentZoneCount = ch.Size / sizeof(Zone);

                for (size_t i = 0; i < InstrumentZoneCount; ++i)
                {
                    Read(&Zone, sizeof(Zone));

                    sf.InstrumentZones.push_back({ Zone.GeneratorIndex, Zone.ModulatorIndex });

                    #ifdef __TRACE
                    ::printf("%*s%5zu. Generator %5d, Modulator %5d\n", __TRACE_LEVEL * 2, "", i + 1, Zone.GeneratorIndex, Zone.ModulatorIndex);
                    #endif
                }

                TRACE_UNINDENT();
                break;
            }

            // Mandatory sub-chunk listing all instrument zone modulators within the SoundFont compatible file.
            case FOURCC_IMOD:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                _instrument_zone_modulator_t Modulator = { };
                const size_t ModulatorCount = ch.Size / sizeof(Modulator);

                for (size_t i = 0; i < ModulatorCount; ++i)
                {
                    Read(&Modulator, sizeof(Modulator));

                    sf.InstrumentZoneModulators.push_back({ Modulator.SrcOperator, Modulator.DstOperator, Modulator.Amount, Modulator.AmountSource, Modulator.SourceTransform });

                    #ifdef __TRACE
                    ::printf("%*s%5zu. Src Op: 0x%04X, Dst Op: %2d, Amount: %6d, Amount Source: 0x%04X, Source Transform: 0x%04X\n", __TRACE_LEVEL * 2, "", i + 1,
                        Modulator.SrcOperator, Modulator.DstOperator, Modulator.Amount, Modulator.AmountSource, Modulator.SourceTransform);
                    #endif
                }

                TRACE_UNINDENT();
                break;
            }

            // Mandatory sub-chunk listing all instrument zone generators for each instrument zone within the SoundFont compatible file.
            case FOURCC_IGEN:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                _instrument_zone_generator_t Generator = { };
                const size_t GeneratorCount = ch.Size / sizeof(Generator);

                for (size_t i = 0; i < GeneratorCount; ++i)
                {
                    Read(&Generator, sizeof(Generator));

                    sf.InstrumentZoneGenerators.push_back({ Generator.Operator, Generator.Amount });

                    #ifdef __TRACE
                    ::printf("%*s%5zu. Operator: 0x%04X, Amount: 0x%04X, %s\n", __TRACE_LEVEL * 2, "", i + 1,
                        Generator.Operator, Generator.Amount,
                        DescribeGeneratorController(Generator.Operator).c_str());
                    #endif
                }

                TRACE_UNINDENT();
                break;
            }

            // Mandatory sub-chunk listing all samples within the SMPL sub-chunk and any referenced ROM samples.
            case FOURCC_SHDR:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                _sample_header_t SampleHeader = { };
                const size_t SampleHeaderCount = ch.Size / sizeof(SampleHeader);

                for (size_t i = 0; i < SampleHeaderCount; ++i)
                {
                    Read(&SampleHeader, sizeof(SampleHeader));

                    std::string Name; Name.assign(SampleHeader.Name, _countof(SampleHeader.Name));

                    sf.Samples.push_back({ Name, SampleHeader.Start, SampleHeader.End, SampleHeader.LoopStart, SampleHeader.LoopEnd,
                        SampleHeader.SampleRate, SampleHeader.Pitch, SampleHeader.PitchCorrection,
                        SampleHeader.SampleLink, SampleHeader.SampleType });

                    #ifdef __TRACE
                    ::printf("%*s%5zu. \"%-20s\", %9d-%9d, Loop: %9d-%9d, %6dHz, Pitch: %3d, Pitch Correction: %3d, Type: 0x%04X, Link: %5d\n", __TRACE_LEVEL * 2, "", i + 1,
                        Name.c_str(), SampleHeader.Start, SampleHeader.End, SampleHeader.LoopStart, SampleHeader.LoopEnd,
                        SampleHeader.SampleRate, SampleHeader.Pitch, SampleHeader.PitchCorrection,
                        SampleHeader.SampleType, SampleHeader.SampleLink);
                    #endif
                }

                TRACE_UNINDENT();
                break;
            }

            default:
            {
                if ((ch.Id & mmioFOURCC(0xFF, 0, 0, 0)) == mmioFOURCC('I', 0, 0, 0))
                {
                    // Information chunks
                    HandleIxxx(ch.Id, ch.Size, sf.Properties);
                }
                else
                {
                    TRACE_CHUNK(ch.Id, ch.Size);

                    SkipChunk(ch);
                }
            }
        }

        return true;
    };

    ReadChunks(ChunkHandler);
#endif
}
