
/** $VER: SoundFontReader.cpp (2025.04.29) P. Stuer - Reads an SBK/SF2/SF3 compliant sound font. **/

#include "pch.h"

#define __TRACE

#include "libsf.h"

#include <functional>

#include "SF2.h"

using namespace sf;

/// <summary>
/// Reads the complete SoundFont.
/// </summary>
void soundfont_reader_t::Process(const soundfont_reader_options_t & options, soundfont_t & sf)
{
    TRACE_RESET();
    TRACE_INDENT();

    uint32_t FormType;

    ReadHeader(FormType);

    if (FormType != FOURCC_SFBK)
        throw sf::exception("Unexpected header type");

    TRACE_FORM(FormType, _Header.Size);
    TRACE_INDENT();

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

                ReadChunks(ch.Size - sizeof(ListType), ChunkHandler);

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

            // Mandatory sub-chunk identifying the wavetable sound engine for which the file was optimized. (SoundFont 2 RIFF File Format Level 2)
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

            // Mandatory sub-chunk identifying the wavetable sound engine for which the file was optimized. (SoundFont 2 RIFF File Format Level 2)
            case FOURCC_INAM:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                sf.BankName.resize((size_t) ch.Size);

                Read((void *) sf.BankName.c_str(), ch.Size);

                #ifdef __TRACE
                ::printf("%*sBank Name: \"%s\"\n", __TRACE_LEVEL * 2, "", sf.BankName.c_str());
                #endif

                TRACE_UNINDENT();
                break;
            }

            // Optional sub-chunk identifying a particular wavetable sound data ROM to which any ROM samples refer. (SoundFont 2 RIFF File Format Level 2)
            case FOURCC_IROM:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                sf.ROMName.resize((size_t) ch.Size);

                Read((void *) sf.ROMName.c_str(), ch.Size);

                #ifdef __TRACE
                ::printf("%*sROM: \"%s\"\n", __TRACE_LEVEL * 2, "", sf.ROMName.c_str());
                #endif

                TRACE_UNINDENT();
                break;
            }

            // Optional sub-chunk identifying the particular wavetable sound data ROM revision to which any ROM samples refer. (SoundFont 2 RIFF File Format Level 2)
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

            // Optional sub-chunk containing one or more samples of digital audio information in the form of linearly coded 16 bit, signed, little endian (least significant byte first) words.
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

            // Optional sub-chunk containing the least significant byte counterparts to each sample data point contained in the smpl chunk. Note this means for every two bytes in the [smpl] sub-chunk there is a 1-byte counterpart in [sm24] sub-chunk.
            case FOURCC_SM24:
            {
                TRACE_CHUNK(ch.Id, ch.Size);

                SkipChunk(ch);
                break;
            }

            /** The HYDRA Data Structure **/

            // Mandatory sub-chunk listing all presets within the SoundFont compatible file.
            case FOURCC_PHDR:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                sfPresetHeader ph = { };
                const size_t PresetHeaderCount = ch.Size / sizeof(ph);

                for (size_t i = 0; i < PresetHeaderCount; ++i)
                {
                    Read(&ph, sizeof(ph));

                    std::string Name; Name.assign(ph.Name, _countof(ph.Name));

                    sf.Presets.push_back({ Name, ph.Bank, ph.wPreset, ph.ZoneIndex });

                    #ifdef __TRACE
                    ::printf("%*s%5zu. \"%-20s\", Bank %3d, Preset %3d, Zone %6d\n", __TRACE_LEVEL * 2, "", i + 1, Name.c_str(), ph.Bank, ph.wPreset, ph.ZoneIndex);
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

                if (sf.Major > 1)
                {
                    sfModList Modulator = { };
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
                }
                else
                    Skip(ch.Size); // .SBK file, SoundFont v1

                TRACE_UNINDENT();
                break;
            }

            // Mandatory sub-chunk listing all preset zone generators for each preset zone within the SoundFont compatible file.
            case FOURCC_PGEN:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                sfGenList Generator = { };
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

                sfInst Instrument = { };
                const size_t InstrumentCount = ch.Size / sizeof(Instrument);

                for (size_t i = 0; i < InstrumentCount; ++i)
                {
                    Read(&Instrument, sizeof(Instrument));

                    std::string Name; Name.assign(Instrument.Name, _countof(Instrument.Name));

                    sf.Instruments.push_back(instrument_t(Name, Instrument.ZoneIndex));

                    #ifdef __TRACE
                    ::printf("%*s%5zu. \"%-20s\", Zone %5d\n", __TRACE_LEVEL * 2, "", i + 1, Instrument.Name, Instrument.ZoneIndex);
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

                sfInstBag Zone = { };
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

                if (sf.Major > 1)
                {
                    sfInstModList Modulator = { };
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
                }
                else
                    Skip(ch.Size); // .SBK file, SoundFont v1

                TRACE_UNINDENT();
                break;
            }

            // Mandatory sub-chunk listing all instrument zone generators for each instrument zone within the SoundFont compatible file.
            case FOURCC_IGEN:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                sfInstGenList Generator = { };
                const size_t GeneratorCount = ch.Size / sizeof(Generator);

                for (size_t i = 0; i < GeneratorCount; ++i)
                {
                    Read(&Generator, sizeof(Generator));

                    sf.InstrumentZoneGenerators.push_back({ Generator.Operator, Generator.Amount });

                    #ifdef __TRACE
                    ::printf("%*s%5zu. Operator: 0x%04X, Amount: 0x%04X\n", __TRACE_LEVEL * 2, "", i + 1,
                        Generator.Operator, Generator.Amount);
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

                sfSample SampleHeader = { };
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
                    TRACE_CHUNK(ch.Id, ch.Size);
                    TRACE_INDENT();

                    HandleIxxx(ch.Id, ch.Size, sf.Properties);

                    TRACE_UNINDENT();
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

    ReadChunks(_Header.Size - sizeof(FormType), ChunkHandler);

    TRACE_UNINDENT(); // RIFF

    TRACE_UNINDENT(); // File
}
