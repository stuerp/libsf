
/** $VER: SoundFontWriter.cpp (2025.04.30) P. Stuer - Writes a SoundFont bank. **/

#include "pch.h"

#define __TRACE

#include "libsf.h"

#include <functional>

#include "SF2.h"

using namespace sf;

/// <summary>
/// Writes the complete SoundFont bank.
/// </summary>
void soundfont_writer_t::Process(const soundfont_writer_options_t & options, bank_t & bank)
{
    TRACE_RESET();
    TRACE_INDENT();

    TRACE_FORM(FOURCC_SFBK, 0);
    TRACE_INDENT();
    {
        WriteChunks(FOURCC_RIFF, FOURCC_SFBK, [this, &options, &bank]() -> uint32_t
        {
            uint32_t FormSize = 0;

            TRACE_LIST(FOURCC_INFO, 0);
            TRACE_INDENT();
            {
                FormSize += WriteChunks(FOURCC_LIST, FOURCC_INFO, [this, &options, &bank]() -> uint32_t
                {
                    uint32_t ListSize = WriteChunk(FOURCC_IFIL, [this, &options, &bank]() -> uint32_t
                    {
                        const uint16_t Version[] = { bank.Major, bank.Minor };

                        return Write(Version, sizeof(Version));
                    });

                    ListSize += WriteChunk(FOURCC_ISNG, [this, &options, &bank]() -> uint32_t
                    {
                        const char * Data = bank.SoundEngine.c_str();
                        const uint32_t Size = (uint32_t) ::strlen(Data) + 1; // Including the string terminator.

                        return Write(Data, Size);
                    });

                    ListSize += WriteChunk(FOURCC_INAM, [this, &options, &bank]() -> uint32_t
                    {
                        const char * Data = bank.BankName.c_str();
                        const uint32_t Size = (uint32_t) ::strlen(Data) + 1; // Including the string terminator.

                        return Write(Data, Size);
                    });

                    if ((bank.ROMName.length() != 0) && !((bank.ROMMajor == 0) && (bank.ROMMinor == 0)))
                    {
                        ListSize += WriteChunk(FOURCC_IROM, [this, &options, &bank]() -> uint32_t
                        {
                            const char * Data = bank.ROMName.c_str();
                            const uint32_t Size = (uint32_t) ::strlen(Data) + 1; // Including the string terminator.

                            return Write(Data, Size);
                        });

                        ListSize += WriteChunk(FOURCC_IVER, [this, &options, &bank]() -> uint32_t
                        {
                            const uint16_t Version[] = { bank.ROMMajor, bank.ROMMinor };

                            return Write(Version, sizeof(Version));
                        });
                    }

                    for (const auto & [ ChunkId, Value ] : bank.Properties)
                    {
                        ListSize += WriteChunk(ChunkId, [this, &options, &bank, Value]() -> uint32_t
                        {
                            const char * Data = Value.c_str();
                            const uint32_t Size = (uint32_t) ::strlen(Data) + 1; // Including the string terminator.

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
                FormSize += WriteChunks(FOURCC_LIST, FOURCC_SDTA, [this, &options, &bank]() -> uint32_t
                {
                    uint32_t ListSize = 0;

                    if (bank.Major == 1)
                    {
                        ListSize += WriteChunk(FOURCC_SNAM, [this, &options, &bank]() -> uint32_t
                        {
                            uint32_t Size = 0;

                            for (const auto & Name : bank.SampleNames)
                            {
                                char Data[20] = { };

                                ::memcpy(Data, Name.c_str(), std::min(Name.length(), sizeof(Data)));

                                Size += Write(Data, sizeof(Data));
                            }

                            return Size;
                        });
                    }

                    if (bank.SampleData.size() != 0)
                    {
                        ListSize += WriteChunk(FOURCC_SMPL, [this, &options, &bank]() -> uint32_t
                        {
                            return Write(bank.SampleData.data(), (uint32_t) bank.SampleData.size());
                        });
                    }

                    return ListSize;
                });
            }
            TRACE_UNINDENT(); // LIST

            /** Write the Hydra. **/

            TRACE_LIST(FOURCC_PDTA, 0);
            TRACE_INDENT();
            {
                FormSize += WriteChunks(FOURCC_LIST, FOURCC_PDTA, [this, &options, &bank]() -> uint32_t
                {
                    uint32_t ListSize = WriteChunk(FOURCC_PHDR, [this, &options, &bank]() -> uint32_t
                    {
                        uint32_t Size = 0;

                        for (const auto & Preset : bank.Presets)
                        {
                            sfPresetHeader ph =
                            {
                                { }, Preset.MIDIProgram, Preset.MIDIBank, Preset.ZoneIndex, Preset.Library, Preset.Genre, Preset.Morphology
                            };

                            ::memcpy(ph.Name, Preset.Name.c_str(), std::min(Preset.Name.length(), sizeof(ph.Name)));

                            Size += Write(&ph, sizeof(ph));
                        }

                        return Size;
                    });

                    ListSize += WriteChunk(FOURCC_PBAG, [this, &options, &bank]() -> uint32_t
                    {
                        uint32_t Size = 0;

                        for (const auto & PresetZone : bank.PresetZones)
                        {
                            const sfPresetBag pb = { PresetZone.GeneratorIndex, PresetZone.ModulatorIndex };

                            Size += Write(&pb, sizeof(pb));
                        }

                        return Size;
                    });

                    ListSize += WriteChunk(FOURCC_PMOD, [this, &options, &bank]() -> uint32_t
                    {
                        uint32_t Size = 0;

                        if (bank.Major == 1)
                        {
                            uint8_t Data[6] = { };

                            Size += Write(Data, sizeof(Data));
                        }
                        else
                        {
                            for (const auto & pzm : bank.PresetZoneModulators)
                            {
                                const sfModList ml = { pzm.SrcOperator, pzm.DstOperator, pzm.Amount, pzm.AmountSource, pzm.SourceTransform };

                                Size += Write(&ml, sizeof(ml));
                            }
                        }

                        return Size;
                    });

                    ListSize += WriteChunk(FOURCC_PGEN, [this, &options, &bank]() -> uint32_t
                    {
                        uint32_t Size = 0;

                        for (const auto & pzg : bank.PresetZoneGenerators)
                        {
                            const sfGenList gl = { pzg.Operator, pzg.Amount };

                            Size += Write(&gl, sizeof(gl));
                        }

                        return Size;
                    });

                    ListSize += WriteChunk(FOURCC_INST, [this, &options, &bank]() -> uint32_t
                    {
                        uint32_t Size = 0;

                        for (const auto & Instrument : bank.Instruments)
                        {
                            sfInst i = { { }, Instrument.ZoneIndex };

                            ::memcpy(i.Name, Instrument.Name.c_str(), std::min(Instrument.Name.length(), sizeof(i.Name)));

                            Size += Write(&i, sizeof(i));
                        }

                        return Size;
                    });

                    ListSize += WriteChunk(FOURCC_IBAG, [this, &options, &bank]() -> uint32_t
                    {
                        uint32_t Size = 0;

                        for (const auto & iz : bank.InstrumentZones)
                        {
                            const sfInstBag pb = { iz.GeneratorIndex, iz.ModulatorIndex };

                            Size += Write(&pb, sizeof(pb));
                        }

                        return Size;
                    });

                    ListSize += WriteChunk(FOURCC_IMOD, [this, &options, &bank]() -> uint32_t
                    {
                        uint32_t Size = 0;

                        if (bank.Major == 1)
                        {
                            uint8_t Data[6] = { };

                            Size += Write(Data, sizeof(Data));
                        }
                        else
                        {
                            for (const auto & izm : bank.InstrumentZoneModulators)
                            {
                                const sfInstModList iml = { izm.SrcOperator, izm.DstOperator, izm.Amount, izm.AmountSource, izm.SourceTransform };

                                Size += Write(&iml, sizeof(iml));
                            }
                        }

                        return Size;
                    });

                    ListSize += WriteChunk(FOURCC_IGEN, [this, &options, &bank]() -> uint32_t
                    {
                        uint32_t Size = 0;

                        for (const auto & izg : bank.InstrumentZoneGenerators)
                        {
                            const sfInstGenList igl = { izg.Operator, izg.Amount };

                            Size += Write(&igl, sizeof(igl));
                        }

                        return Size;
                    });

                    ListSize += WriteChunk(FOURCC_SHDR, [this, &options, &bank]() -> uint32_t
                    {
                        uint32_t Size = 0;

                        for (const auto & Sample : bank.Samples)
                        {
                            if (bank.Major == 1)
                            {
                                sfSample_v1 s =
                                {
                                    Sample.Start, Sample.End, Sample.LoopStart, Sample.LoopEnd,
                                };

                                Size += Write(&s, sizeof(s));
                            }
                            else
                            {
                                sfSample_v2 s =
                                {
                                    { },
                                    Sample.Start, Sample.End, Sample.LoopStart, Sample.LoopEnd,
                                    Sample.SampleRate, Sample.Pitch, Sample.PitchCorrection,
                                    Sample.SampleLink, Sample.SampleType
                                };

                                ::memcpy(s.Name, Sample.Name.c_str(), std::min(Sample.Name.length(), sizeof(s.Name)));

                                Size += Write(&s, sizeof(s));
                            }
                        }

                        return Size;
                    });

                    return ListSize;
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
}
