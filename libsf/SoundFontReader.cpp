
/** $VER: SoundFontReader.cpp (2025.04.30) P. Stuer - Reads a SoundFont bank. **/

#include "pch.h"

//#define __TRACE

#include "libsf.h"

#include <functional>

#include "SF2.h"

using namespace sf;

/// <summary>
/// Reads the complete SoundFont bank.
/// </summary>
void soundfont_reader_t::Process(const soundfont_reader_options_t & options, bank_t & bank)
{
    TRACE_RESET();
    TRACE_INDENT();

    uint32_t FormType;

    ReadHeader(FormType);

    if (FormType != FOURCC_SFBK)
        throw sf::exception("Unexpected RIFF type");

    TRACE_FORM(FormType, _Header.Size);
    TRACE_INDENT();

    std::function<bool(const riff::chunk_header_t & ch)> ChunkHandler = [this, &options, &bank, &ChunkHandler](const riff::chunk_header_t & ch) -> bool
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

                Read(&bank.Major, sizeof(bank.Major));
                Read(&bank.Minor, sizeof(bank.Minor));

                #ifdef __TRACE
                ::printf("%*sSoundFont specification version: %d.%02d\n", __TRACE_LEVEL * 2, "", bank.Major, bank.Minor);
                #endif

                TRACE_UNINDENT();
                break;
            }

            // Mandatory sub-chunk identifying the wavetable sound engine for which the file was optimized. (SoundFont 2 RIFF File Format Level 2)
            case FOURCC_ISNG:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                bank.SoundEngine.resize((size_t) ch.Size);

                Read((void *) bank.SoundEngine.c_str(), ch.Size);

                #ifdef __TRACE
                ::printf("%*sSound Engine: \"%s\"\n", __TRACE_LEVEL * 2, "", bank.SoundEngine.c_str());
                #endif

                TRACE_UNINDENT();
                break;
            }

            // Mandatory sub-chunk identifying the wavetable sound engine for which the file was optimized. (SoundFont 2 RIFF File Format Level 2)
            case FOURCC_INAM:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                bank.BankName.resize((size_t) ch.Size + 1, '\0');

                Read((void *) bank.BankName.data(), ch.Size);

                #ifdef __TRACE
                ::printf("%*sBank Name: \"%s\"\n", __TRACE_LEVEL * 2, "", bank.BankName.c_str());
                #endif

                TRACE_UNINDENT();
                break;
            }

            // Optional sub-chunk identifying a particular wavetable sound data ROM to which any ROM samples refer. (SoundFont 2 RIFF File Format Level 2)
            case FOURCC_IROM:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                bank.ROMName.resize((size_t) ch.Size + 1, '\0');

                Read((void *) bank.ROMName.data(), ch.Size);

                #ifdef __TRACE
                ::printf("%*sROM: \"%s\"\n", __TRACE_LEVEL * 2, "", bank.ROMName.c_str());
                #endif

                TRACE_UNINDENT();
                break;
            }

            // Optional sub-chunk identifying the particular wavetable sound data ROM revision to which any ROM samples refer. (SoundFont 2 RIFF File Format Level 2)
            case FOURCC_IVER:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                Read(&bank.ROMMajor, sizeof(bank.ROMMajor));
                Read(&bank.ROMMinor, sizeof(bank.ROMMinor));

                #ifdef __TRACE
                ::printf("%*sROM Version: %d.%02d\n", __TRACE_LEVEL * 2, "", bank.ROMMajor, bank.ROMMinor);
                #endif

                TRACE_UNINDENT();
                break;
            }

            /** SDTA List **/

            // Mandatory sub-chunk containing the sample names. (SoundFont v1.0.0 only)
            case FOURCC_SNAM:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                if (bank.Major != 1)
                    throw sf::exception(riff::FormatText("snam chunk not allowed in SoundFont v%d.%02d bank", bank.Major, bank.Minor).c_str());

                char Data[20] = { };
                const size_t Count = ch.Size / sizeof(Data);

                bank.SampleNames.resize(Count);

                for (size_t i = 0; i < Count; ++i)
                {
                    Read(&Data, sizeof(Data));

                    bank.SampleNames[i] = std::string(Data, sizeof(Data));

                    #ifdef __TRACE
                    ::printf("%*s%5zu. \"%-20s\"\n", __TRACE_LEVEL * 2, "", i, Data);
                    #endif
                }

                TRACE_UNINDENT();
                break;
            }

            // Optional sub-chunk containing one or more samples of digital audio information in the form of linearly coded 16 bit, signed, little endian (least significant byte first) words.
            case FOURCC_SMPL:
            {
                TRACE_CHUNK(ch.Id, ch.Size);

                if (options.ReadSampleData)
                {
                    bank.SampleData.resize((size_t) ch.Size);

                    Read(bank.SampleData.data(), ch.Size);
                }
                else
                    SkipChunk(ch);
                break;
            }

            // Optional sub-chunk containing the least significant byte counterparts to each sample data point contained in the smpl chunk. Note this means for every two bytes in the [smpl] sub-chunk there is a 1-byte counterpart in [sm24] sub-chunk.
            case FOURCC_SM24:
            {
                TRACE_CHUNK(ch.Id, ch.Size);

                if (options.ReadSampleData)
                {
                    bank.SampleDataLSB.resize((size_t) ch.Size);

                    Read(bank.SampleDataLSB.data(), ch.Size);
                }
                else
                    SkipChunk(ch);
                break;
            }

            /** The Hydra data structure **/

            // Mandatory sub-chunk listing all presets within the SoundFont compatible file.
            case FOURCC_PHDR:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                sfPresetHeader ph = { };
                const size_t Count = ch.Size / sizeof(ph);

                bank.Presets.resize(Count);

                for (size_t i = 0; i < Count; ++i)
                {
                    Read(&ph, sizeof(ph));

                    bank.Presets[i] = preset_t(std::string(ph.Name, sizeof(ph.Name)), ph.wPreset, ph.Bank, ph.ZoneIndex, ph.Library, ph.Genre, ph.Morphology);

                    #ifdef __TRACE
                    ::printf("%*s%5zu. \"%-20s\", MIDI Preset %3d, MIDI Bank %3d, Zone %6d\n", __TRACE_LEVEL * 2, "", i, ph.Name, ph.wPreset, ph.Bank, ph.ZoneIndex);
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
                const size_t Count = ch.Size / sizeof(Zone);

                bank.PresetZones.resize(Count);

                for (size_t i = 0; i < Count; ++i)
                {
                    Read(&Zone, sizeof(Zone));

                    bank.PresetZones[i] = preset_zone_t(Zone.GeneratorIndex, Zone.ModulatorIndex);

                    #ifdef __TRACE
                    ::printf("%*s%5zu. Generator %5d, Modulator %5d\n", __TRACE_LEVEL * 2, "", i, Zone.GeneratorIndex, Zone.ModulatorIndex);
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

                if (bank.Major > 1)
                {
                    sfModList Modulator = { };
                    const size_t Count = ch.Size / sizeof(Modulator);

                    bank.PresetZoneModulators.resize(Count);

                    for (size_t i = 0; i < Count; ++i)
                    {
                        Read(&Modulator, sizeof(Modulator));

                        bank.PresetZoneModulators[i] = preset_zone_modulator_t(Modulator.SrcOperator, Modulator.DstOperator, Modulator.Amount, Modulator.AmountSource, Modulator.SourceTransform);

                        #ifdef __TRACE
                        ::printf("%*s%5zu. Src Op: 0x%04X, Dst Op: %2d, Amount: %6d, Amount Source: 0x%04X, Source Transform: 0x%04X\n", __TRACE_LEVEL * 2, "", i,
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
                const size_t Count = ch.Size / sizeof(Generator);

                bank.PresetZoneGenerators.resize(Count);

                for (size_t i = 0; i < Count; ++i)
                {
                    Read(&Generator, sizeof(Generator));

                    bank.PresetZoneGenerators[i] = preset_zone_generator_t(Generator.Operator, Generator.Amount);

                    #ifdef __TRACE
                    ::printf("%*s%5zu. Operator: 0x%04X, Amount: 0x%04X\n", __TRACE_LEVEL * 2, "", i, Generator.Operator, Generator.Amount);
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

                sfInst Data = { };
                const size_t Count = ch.Size / sizeof(Data);

                bank.Instruments.resize(Count);

                for (size_t i = 0; i < Count; ++i)
                {
                    Read(&Data, sizeof(Data));

                    bank.Instruments[i] = instrument_t(std::string(Data.Name, sizeof(Data.Name)), Data.ZoneIndex);

                    #ifdef __TRACE
                    ::printf("%*s%5zu. \"%-20s\", Zone %5d\n", __TRACE_LEVEL * 2, "", i, Instrument.Name, Instrument.ZoneIndex);
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

                sfInstBag ib = { };
                const size_t Count = ch.Size / sizeof(ib);

                bank.InstrumentZones.resize(Count);

                for (size_t i = 0; i < Count; ++i)
                {
                    Read(&ib, sizeof(ib));

                    bank.InstrumentZones[i] = instrument_zone_t(ib.GeneratorIndex, ib.ModulatorIndex);

                    #ifdef __TRACE
                    ::printf("%*s%5zu. Generator %5d, Modulator %5d\n", __TRACE_LEVEL * 2, "", i, ib.GeneratorIndex, ib.ModulatorIndex);
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

                if (bank.Major > 1)
                {
                    sfInstModList Modulator = { };
                    const size_t Count = ch.Size / sizeof(Modulator);

                    bank.InstrumentZoneModulators.resize(Count);

                    for (size_t i = 0; i < Count; ++i)
                    {
                        Read(&Modulator, sizeof(Modulator));

                        bank.InstrumentZoneModulators[i] = instrument_zone_modulator_t(Modulator.SrcOperator, Modulator.DstOperator, Modulator.Amount, Modulator.AmountSource, Modulator.SourceTransform);

                        #ifdef __TRACE
                        ::printf("%*s%5zu. Src Op: 0x%04X, Dst Op: %2d, Amount: %6d, Amount Source: 0x%04X, Source Transform: 0x%04X\n", __TRACE_LEVEL * 2, "", i,
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
                const size_t Count = ch.Size / sizeof(Generator);

                bank.InstrumentZoneGenerators.resize(Count);

                for (size_t i = 0; i < Count; ++i)
                {
                    Read(&Generator, sizeof(Generator));

                    bank.InstrumentZoneGenerators[i] = instrument_zone_generator_t(Generator.Operator, Generator.Amount);

                    #ifdef __TRACE
                    ::printf("%*s%5zu. Operator: 0x%04X, Amount: 0x%04X\n", __TRACE_LEVEL * 2, "", i,
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

                if (bank.Major == 1)
                {
                    auto SampleType = SampleTypes::RomMonoSample;

                    sfSample_v1 sh = { };
                    const size_t Count = ch.Size / sizeof(sh);

                    bank.Samples.resize(Count);

                    for (size_t i = 0; i < Count; ++i)
                    {
                        Read(&sh, sizeof(sh));

                        // The first sample with address 0 is used to change the sample type from ROM to RAM.
                        if (sh.Start == 0)
                            SampleType = SampleTypes::MonoSample;

                        bank.Samples[i] = sample_t("", sh.Start, sh.End, sh.LoopStart, sh.LoopEnd, 22050, 60, 0, 0, SampleType);

                        #ifdef __TRACE
                        ::printf("%*s%5zu.%9d-%9d, Loop: %9d-%9d\n", __TRACE_LEVEL * 2, "", i, sh.Start, sh.End, sh.LoopStart, sh.LoopEnd);
                        #endif
                    }
                }
                else
                {
                    sfSample_v2 sh = { };
                    const size_t Count = ch.Size / sizeof(sh);

                    bank.Samples.resize(Count);

                    for (size_t i = 0; i < Count; ++i)
                    {
                        Read(&sh, sizeof(sh));

                        bank.Samples[i] = sample_t(std::string(sh.Name, sizeof(sh.Name)), sh.Start, sh.End, sh.LoopStart, sh.LoopEnd, sh.SampleRate, sh.Pitch, sh.PitchCorrection, sh.SampleLink, sh.SampleType);

                        #ifdef __TRACE
                        ::printf("%*s%5zu. \"%-20s\", %9d-%9d, Loop: %9d-%9d, %6dHz, Pitch: %3d, Pitch Correction: %3d, Type: 0x%04X, Link: %5d\n", __TRACE_LEVEL * 2, "", i,
                            sh.Name, sh.Start, sh.End, sh.LoopStart, sh.LoopEnd,
                            sh.SampleRate, sh.Pitch, sh.PitchCorrection,
                            sh.SampleType, sh.SampleLink);
                        #endif
                    }
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

                    HandleIxxx(ch.Id, ch.Size, bank.Properties);

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
