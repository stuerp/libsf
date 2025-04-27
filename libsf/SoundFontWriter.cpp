
/** $VER: SoundFontWriter.cpp (2025.04.27) P. Stuer - Writes an SF2/SF3 compliant sound font. **/

#include "pch.h"

#define __TRACE

#include "libsf.h"

#include <functional>

#include "SF2.h"

using namespace sf;

/// <summary>
/// Writes the SoundFont.
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
                        const uint16_t Version[] = { sf.Major, sf.Minor };

                        return Write(Version, sizeof(Version));
                    });

                    ListSize += WriteChunk(FOURCC_ISNG, [this, &options, &sf]() -> uint32_t
                    {
                        return Write(sf.SoundEngine.c_str(), (uint32_t) ::strlen(sf.SoundEngine.c_str()) + 1); // Include string terminator.
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
                    uint32_t ListSize = WriteChunk(FOURCC_SMPL, [this, &options, &sf]() -> uint32_t
                    {
                        return Write(sf.SampleData.data(), (uint32_t) sf.SampleData.size());
                    });

                    return ListSize;
                });
            }
            TRACE_UNINDENT(); // LIST

            TRACE_LIST(FOURCC_PDTA, 0);
            TRACE_INDENT();
            {
                FormSize += WriteChunks(FOURCC_LIST, FOURCC_PDTA, [this, &options, &sf]() -> uint32_t
                {
                    uint32_t ListSize = WriteChunk(FOURCC_PHDR, [this, &options, &sf]() -> uint32_t
                    {
                        uint32_t Size = 0;

                        for (const auto & Preset : sf.Presets)
                        {
                            sfPresetHeader ph =
                            {
                                "", Preset.Bank, Preset.Program, Preset.ZoneIndex, 0, 0, 0
                            };

                            ::memcpy(ph.Name, Preset.Name.c_str(), std::max(::strlen(Preset.Name.c_str()), sizeof(ph.Name)));

                            Size += Write(&ph, sizeof(ph));
                        }

                        return Size;
                    });

                    ListSize += WriteChunk(FOURCC_PBAG, [this, &options, &sf]() -> uint32_t
                    {
                        uint32_t Size = 0;

                        for (const auto & PresetZone : sf.PresetZones)
                        {
                            const sfPresetBag pb = { PresetZone.GeneratorIndex, PresetZone.ModulatorIndex };

                            Size += Write(&pb, sizeof(pb));
                        }

                        return Size;
                    });

                    ListSize += WriteChunk(FOURCC_PMOD, [this, &options, &sf]() -> uint32_t
                    {
                        uint32_t Size = 0;

                        for (const auto & pzm : sf.PresetZoneModulators)
                        {
                            const sfModList ml = { pzm.SrcOperator, pzm.DstOperator, pzm.Amount, pzm.AmountSource, pzm.SourceTransform };

                            Size += Write(&ml, sizeof(ml));
                        }

                        return Size;
                    });

                    ListSize += WriteChunk(FOURCC_PGEN, [this, &options, &sf]() -> uint32_t
                    {
                        uint32_t Size = 0;

                        for (const auto & pzg : sf.PresetZoneGenerators)
                        {
                            const sfGenList gl = { pzg.Operator, pzg.Amount };

                            Size += Write(&gl, sizeof(gl));
                        }

                        return Size;
                    });

                    ListSize += WriteChunk(FOURCC_INST, [this, &options, &sf]() -> uint32_t
                    {
                        uint32_t Size = 0;

                        for (const auto & Instrument : sf.Instruments)
                        {
                            sfInst i = { "", Instrument.ZoneIndex };

                            ::memcpy(i.Name, Instrument.Name.c_str(), std::max(::strlen(Instrument.Name.c_str()), sizeof(i.Name)));

                            Size += Write(&i, sizeof(i));
                        }

                        return Size;
                    });

                    ListSize += WriteChunk(FOURCC_IBAG, [this, &options, &sf]() -> uint32_t
                    {
                        uint32_t Size = 0;

                        for (const auto & iz : sf.InstrumentZones)
                        {
                            const sfInstBag pb = { iz.GeneratorIndex, iz.ModulatorIndex };

                            Size += Write(&pb, sizeof(pb));
                        }

                        return Size;
                    });

                    ListSize += WriteChunk(FOURCC_IMOD, [this, &options, &sf]() -> uint32_t
                    {
                        uint32_t Size = 0;

                        for (const auto & izm : sf.InstrumentZoneModulators)
                        {
                            const sfInstModList iml = { izm.SrcOperator, izm.DstOperator, izm.Amount, izm.AmountSource, izm.SourceTransform };

                            Size += Write(&iml, sizeof(iml));
                        }

                        return Size;
                    });

                    ListSize += WriteChunk(FOURCC_IGEN, [this, &options, &sf]() -> uint32_t
                    {
                        uint32_t Size = 0;

                        for (const auto & izg : sf.InstrumentZoneGenerators)
                        {
                            const sfInstGenList igl = { izg.Operator, izg.Amount };

                            Size += Write(&igl, sizeof(igl));
                        }

                        return Size;
                    });

                    ListSize += WriteChunk(FOURCC_SHDR, [this, &options, &sf]() -> uint32_t
                    {
                        uint32_t Size = 0;

                        for (const auto & Sample : sf.Samples)
                        {
                            sfSample s =
                            {
                                "",
                                Sample.Start, Sample.End, Sample.LoopStart, Sample.LoopEnd,
                                Sample.SampleRate, Sample.Pitch, Sample.PitchCorrection,
                                Sample.SampleLink, Sample.SampleType
                            };

                            ::memcpy(s.Name, Sample.Name.c_str(), std::max(::strlen(Sample.Name.c_str()), sizeof(s.Name)));

                            Size += Write(&s, sizeof(s));
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
