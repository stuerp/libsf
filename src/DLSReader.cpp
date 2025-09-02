
/** $VER: DLSReader.cpp (2025.09.02) P. Stuer - Implements a reader for a DLS-compliant sound font. **/

#include "pch.h"

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4505 4625 4626 4710 4711 4738 5045 ALL_CPPCORECHECK_WARNINGS)

//#define __TRACE
//#define __DEEP_TRACE

#include "DLSReader.h"
#include "Exception.h"
#include "Encoding.h"

#include <functional>

using namespace sf::dls;

/// <summary>
/// Removes leading whitespace.
/// </summary>
static std::string ltrim(const std::string & s)
{
    auto it = std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); });

    return std::string(it, s.end());
}

/// <summary>
/// Removes trailing whitespace.
/// </summary>
static std::string rtrim(const std::string & s)
{
    auto it = std::find_if(s.rbegin(), s.rend(), [](unsigned char ch)
    {
        return !std::isspace(ch);
    });

    return std::string(s.begin(), it.base());
}

/// <summary>
/// Removes leading and trailing whitespace.
/// </summary>
static std::string trim(const std::string & s)
{
    return ltrim(rtrim(s));
}

/// <summary>
/// Processes the complete sound font.
/// </summary>
void reader_t::Process(collection_t & dls, const reader_options_t & options)
{
    _Options = options;

    TRACE_RESET();
    TRACE_INDENT();

    uint32_t FormType;

    ReadHeader(FormType);

    if (FormType != FOURCC_DLS)
        throw sf::exception("Unexpected header type");

    TRACE_FORM(FormType, _Header.Size);
    TRACE_INDENT();

    std::function<bool(const riff::chunk_header_t & ch)> ChunkHandler = [this, &options, &dls, &ChunkHandler](const riff::chunk_header_t & ch) -> bool
    {
        switch (ch.Id)
        {
            // The Collection Header chunk defines an instrument collection.
            case FOURCC_COLH:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                uint32_t InstrumentCount; // cInstruments, Specifies the count of instruments in this collection, and indicates the number of instrument chunks in the ‘lins’ list.

                Read(InstrumentCount);

                dls.Instruments.reserve(InstrumentCount);

                #ifdef __DEEP_TRACE
                ::printf("%*s%d instruments\n", __TRACE_LEVEL * 2, "", InstrumentCount);
                #endif

                TRACE_UNINDENT();
                break;
            }

            // The Version chunk defines an optional version stamp within a collection. It indicates the version of the contents of the file, not the DLS specification level.
            case FOURCC_VERS:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                uint32_t dwVersionMS;
                uint32_t dwVersionLS;

                Read(dwVersionMS);
                Read(dwVersionLS);

                dls.Major    = HIWORD(dwVersionMS);
                dls.Minor    = LOWORD(dwVersionMS);
                dls.Revision = HIWORD(dwVersionLS);
                dls.Build    = LOWORD(dwVersionMS);

                #ifdef __DEEP_TRACE
                ::printf("%*sVersion: %d.%d.%d.%d\n", __TRACE_LEVEL * 2, "", dls.Major, dls.Minor, dls.Revision, dls.Build);
                #endif

                TRACE_UNINDENT();
                break;
            }

            // The DLSID chunk defines an optional globally unique identifier (DLSID) for a complete <DLS-form> or for an element within it.
            case FOURCC_DLID:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                GUID Id = {};

                Read(&Id.Data1, sizeof(Id.Data1));
                Read(&Id.Data2, sizeof(Id.Data2));
                Read(&Id.Data3, sizeof(Id.Data3));
                Read(&Id.Data4, sizeof(Id.Data4));

                WCHAR Text[32] = {};

                (void) ::StringFromGUID2(Id, Text, _countof(Text));

                #ifdef __DEEP_TRACE
                ::printf("%*sId: %s\n", __TRACE_LEVEL * 2, "", WideToUTF8(Text).c_str());
                #endif

                TRACE_UNINDENT();
                break;
            }

            // The Pool Table chunk contains a list of cross-reference entries to digital audio data within the wave pool.
            case FOURCC_PTBL:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                uint32_t Size;      // Specifies the size of the structure in bytes.

                Read(Size);

                uint32_t CueCount;  // Specifies the number (count) of <poolcue> records that are contained in the <ptbl-ck> chunk.

                Read(CueCount);

                dls.Cues.reserve(CueCount);

                if (Size != 8)
                    Skip(Size - 8);

                #ifdef __DEEP_TRACE
                ::printf("%*sCues: %d\n", __TRACE_LEVEL * 2, "", CueCount);
                #endif

                {
                    TRACE_INDENT();

                    while (CueCount != 0)
                    {
                        uint32_t Offset; // Specifies the absolute offset in bytes from the beginning of the wave pool data to the correct entry in the wave pool.

                        Read(Offset);

                        dls.Cues.push_back(Offset);

                        #ifdef __DEEP_TRACE
                        ::printf("%*sOffset: %8d\n", __TRACE_LEVEL * 2, "", Offset);
                        #endif

                        --CueCount;
                    }

                    TRACE_UNINDENT();
                }

                TRACE_UNINDENT();
                break;
            }

            // A LIST chunk contains an ordered sequence of subchunks.
            case FOURCC_LIST:
            {
                uint32_t ListType;

                Read(ListType);

                TRACE_LIST(ListType, ch.Size);
                TRACE_INDENT();

                switch (ListType)
                {
                    // Instrument List
                    case FOURCC_LINS:
                    {
                        ReadInstruments(ch, dls.Instruments);
                        break;
                    }

                    // Wave Pool
                    case FOURCC_WVPL:
                    {
                        ReadWaves(ch, dls.Waves);
                        break;
                    }

                    default:
                        ReadChunks(ch.Size - sizeof(ListType), ChunkHandler);
                }

                TRACE_UNINDENT();
                break;
            }

            default:
            {
                if ((ch.Id & mmioFOURCC(0xFF, 0, 0, 0)) == mmioFOURCC('I', 0, 0, 0))
                {
                    HandleIxxx(ch.Id, ch.Size, dls.Properties);
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

    TRACE_UNINDENT(); // FORM

    TRACE_UNINDENT(); // File
}

/// <summary>
/// Read the instrument list.
/// </summary>
void reader_t::ReadInstruments(const riff::chunk_header_t & ch, std::vector<instrument_t> & instruments)
{
    std::function<bool(const riff::chunk_header_t & ch)> ChunkHandler = [this, &instruments, &ChunkHandler](const riff::chunk_header_t & ch) -> bool
    {
        switch (ch.Id)
        {
            // A LIST chunk contains an ordered sequence of subchunks.
            case FOURCC_LIST:
            {
                uint32_t ListType;

                Read(ListType);

                TRACE_LIST(ListType, ch.Size);
                TRACE_INDENT();

                switch (ListType)
                {
                    case FOURCC_INS:
                    {
                        instruments.push_back(instrument_t());

                        ReadInstrument(ch, instruments.back());
                        break;
                    }

                    default:
                        ReadChunks(ch.Size - sizeof(ListType), ChunkHandler);
                }

                TRACE_UNINDENT();
                break;
            }

            default:
            {
                if ((ch.Id & mmioFOURCC(0xFF, 0, 0, 0)) == mmioFOURCC('I', 0, 0, 0))
                {
                    properties_t Properties;

                    HandleIxxx(ch.Id, ch.Size, Properties);
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

    ReadChunks(ch.Size - sizeof(ch.Id), ChunkHandler);
}

/// <summary>
/// Reads an instrument.
/// </summary>
void reader_t::ReadInstrument(const riff::chunk_header_t & ch, instrument_t & instrument)
{
    std::function<bool(const riff::chunk_header_t & ch)> ChunkHandler = [this, &instrument, &ChunkHandler](const riff::chunk_header_t & ch) -> bool
    {
        switch (ch.Id)
        {
            // The Instrument Header chunk defines an instrument within a collection.
            case FOURCC_INSH:
            {
                // The instrument header determines the number of regions in an instrument, as well as its bank and program numbers.
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                uint32_t RegionCount;   // Specifies the count of regions for this instrument. (cRegions)
                uint32_t Bank;          // Specifies the MIDI bank location. Bits 0-6 are defined as MIDI CC32 and bits 8-14 are defined as MIDI CC0. (ulBank)
                uint32_t Program;       // Specifies the MIDI Program Change (PC) value. Bits 0-6. (ulInstrument)

                Read(RegionCount);
                Read(Bank);
                Read(Program);

                instrument.Regions.reserve(RegionCount);

                const bool IsPercussion = ((Bank & F_INSTRUMENT_DRUMS) != 0);

                instrument = instrument_t(RegionCount, (Bank >> 8) & 0x7F, Bank & 0x7F, Program & 0x7F, IsPercussion);

                #ifdef __DEEP_TRACE
                ::printf("%*sRegions: %d, Bank: CC0 0x%02X CC32 0x%02X (MMA %d), Is Percussion: %s, Program: %d\n", __TRACE_LEVEL * 2, "", RegionCount, (Bank >> 8) & 0x7F, Bank & 0x7F, (((Bank >> 8) & 0x7F) * 128 + (Bank & 0x7F)), IsPercussion ? "true" : "false", Program & 0x7F);
                #endif

                TRACE_UNINDENT();
                break;
            }

            // A LIST chunk contains an ordered sequence of subchunks.
            case FOURCC_LIST:
            {
                uint32_t ListType;

                Read(ListType);

                TRACE_LIST(ListType, ch.Size);
                TRACE_INDENT();

                switch (ListType)
                {
                    case FOURCC_LRGN:
                    {
                        ReadRegions(ch, instrument.Regions);
                        break;
                    }

                    case FOURCC_LART:
                    case FOURCC_LAR2:
                    {
                        ReadArticulators(ch, instrument.Articulators);
                        break;
                    }

                    default:
                        ReadChunks(ch.Size - sizeof(ListType), ChunkHandler);
                }

                TRACE_UNINDENT();
                break;
            }

            default:
            {
                if ((ch.Id & mmioFOURCC(0xFF, 0, 0, 0)) == mmioFOURCC('I', 0, 0, 0))
                {
                    HandleIxxx(ch.Id, ch.Size, instrument.Properties);
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

    ReadChunks(ch.Size - sizeof(ch.Id), ChunkHandler);

    instrument.Name = rtrim(GetPropertyValue(instrument.Properties, FOURCC_INAM));
}

/// <summary>
/// Read the regions of an instrument.
/// </summary>
void reader_t::ReadRegions(const riff::chunk_header_t & ch, std::vector<region_t> & regions)
{
    std::function<bool(const riff::chunk_header_t & ch)> ChunkHandler = [this, &regions, &ChunkHandler](const riff::chunk_header_t & ch) -> bool
    {
        switch (ch.Id)
        {
            // A LIST chunk contains an ordered sequence of subchunks.
            case FOURCC_LIST:
            {
                uint32_t ListType;

                Read(ListType);

                TRACE_LIST(ListType, ch.Size);
                TRACE_INDENT();

                switch (ListType)
                {
                    case FOURCC_RGN:
                    case FOURCC_RGN2:
                    {
                        regions.push_back(region_t());

                        ReadRegion(ch, regions.back());
                        break;
                    }

                    default:
                        ReadChunks(ch.Size - sizeof(ListType), ChunkHandler);
                }

                TRACE_UNINDENT();
                break;
            }

            default:
            {
                if ((ch.Id & mmioFOURCC(0xFF, 0, 0, 0)) == mmioFOURCC('I', 0, 0, 0))
                {
                    properties_t Properties;

                    HandleIxxx(ch.Id, ch.Size, Properties);
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

    ReadChunks(ch.Size - sizeof(ch.Id), ChunkHandler);
}

/// <summary>
/// Read a region.
/// </summary>
void reader_t::ReadRegion(const riff::chunk_header_t & ch, region_t & region)
{
    std::function<bool(const riff::chunk_header_t & ch)> ChunkHandler = [this, &region, &ChunkHandler](const riff::chunk_header_t & ch) -> bool
    {
        switch (ch.Id)
        {
            // A LIST chunk contains an ordered sequence of subchunks.
            case FOURCC_LIST:
            {
                uint32_t ListType;

                Read(ListType);

                TRACE_LIST(ListType, ch.Size);
                TRACE_INDENT();

                switch (ListType)
                {
                    case FOURCC_LART:
                    case FOURCC_LAR2:
                    {
                        ReadArticulators(ch, region.Articulators);
                        break;
                    }

                    default:
                        ReadChunks(ch.Size - sizeof(ListType), ChunkHandler);
                }

                TRACE_UNINDENT();
                break;
            }

            // The Region Header defines a region within an instrument.
            case FOURCC_RGNH:
            {
                // A region defines the key range and velocity range used by the control logic to select the sample. It also determines a preset value for the overall amplitude of the sample and a preset tuning.
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                Read(region.LowKey);
                Read(region.HighKey);
                Read(region.LowVelocity);
                Read(region.HighVelocity);
                Read(region.Options);
                Read(region.KeyGroup);

                if (ch.Size == 14)
                    Read(region.Layer);

                #ifdef __DEEP_TRACE
                ::printf("%*sKey: %3d - %3d, Velocity: %3d - %3d, Non exclusive: %d, Key Group: %d, Editing Layer: %d\n", __TRACE_LEVEL * 2, "", region.LowKey, region.HighKey, region.LowVelocity, region.HighVelocity, region.Options, region.KeyGroup, region.Layer);
                #endif

                TRACE_UNINDENT();
                break;
            }

            // The Wave Sample chunk describes the minimum necessary information needed to allow a synthesis engine to use a WAVE chunk.
            case FOURCC_WSMP:
            {
                ReadWaveSample(ch, region.WaveSample);
                break;
            }

            // The Wave Link chunk specifies where the wave data can be found for an instrument region in a DLS file.
            case FOURCC_WLNK:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                Read(region.WaveLink.Options);
                Read(region.WaveLink.PhaseGroup);
                Read(region.WaveLink.Channel);
                Read(region.WaveLink.CueIndex);

                #ifdef __DEEP_TRACE
                ::printf("%*sOptions: 0x%08X, PhaseGroup: %d, Channel: %d, TableIndex: %d\n", __TRACE_LEVEL * 2, "", region.WaveLink.Options, region.WaveLink.PhaseGroup, region.WaveLink.Channel, region.WaveLink.CueIndex);
                #endif

                TRACE_UNINDENT();
                break;
            }

            default:
            {
                if ((ch.Id & mmioFOURCC(0xFF, 0, 0, 0)) == mmioFOURCC('I', 0, 0, 0))
                {
                    properties_t Properties;

                    HandleIxxx(ch.Id, ch.Size, Properties);
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

    ReadChunks(ch.Size - sizeof(ch.Id), ChunkHandler);
}

/// <summary>
/// Reads the articulators.
/// </summary>
void reader_t::ReadArticulators(const riff::chunk_header_t & ch, std::vector<articulator_t> & articulators)
{
    std::function<bool(const riff::chunk_header_t & ch)> ChunkHandler = [this, &articulators, &ChunkHandler](const riff::chunk_header_t & ch) -> bool
    {
        switch (ch.Id)
        {
            // A LIST chunk contains an ordered sequence of subchunks.
            case FOURCC_LIST:
            {
                uint32_t ListType;

                Read(ListType);

                TRACE_LIST(ListType, ch.Size);
                TRACE_INDENT();

                ReadChunks(ch.Size - sizeof(ListType), ChunkHandler);

                TRACE_UNINDENT();
                break;
            }

            // The Level 1 Articulator chunk specifies parameters which modify the playback of a wave file used in downloadable instruments.
            case FOURCC_ART1:
            {
                articulators.push_back(articulator_t());

                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                uint32_t Size;
                uint32_t ConnectionBlockCount;

                Read(Size);
                Read(ConnectionBlockCount);

                auto & Articulator = articulators.back();

                Articulator.ConnectionBlocks.reserve(ConnectionBlockCount);

                #ifdef __DEEP_TRACE
                ::printf("%*sConnection Blocks: %d\n", __TRACE_LEVEL * 2, "", ConnectionBlockCount);
                #endif

                {
                    TRACE_INDENT();

                    while (ConnectionBlockCount != 0)
                    {
                        Articulator.ConnectionBlocks.push_back(connection_block_t());

                        auto & ConnectionBlock = Articulator.ConnectionBlocks.back();

                        Read(ConnectionBlock.Source);
                        Read(ConnectionBlock.Control);
                        Read(ConnectionBlock.Destination);
                        Read(ConnectionBlock.Transform);
                        Read(ConnectionBlock.Scale);

                        #ifdef __DEEP_TRACE
                        ::printf("%*sSource: %d, Control: %3d, Destination: %3d, Transform: %d, Scale: %10d\n", __TRACE_LEVEL * 2, "", ConnectionBlock.Source, ConnectionBlock.Control, ConnectionBlock.Destination, ConnectionBlock.Transform, ConnectionBlock.Scale);
                        #endif

                        --ConnectionBlockCount;
                    }

                    TRACE_UNINDENT();
                }

                TRACE_UNINDENT();
                break;
            }

            // The Level 2 Articulator chunk specifies parameters which modify the playback of a sample used in DLS Level 2 downloadable instruments.
            case FOURCC_ART2:
            {
                articulators.push_back(articulator_t());

                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                uint32_t Size;
                uint32_t ConnectionBlockCount;

                Read(&Size, sizeof(Size));
                Read(&ConnectionBlockCount, sizeof(ConnectionBlockCount));

                auto & Articulator = articulators.back();

                Articulator.ConnectionBlocks.reserve(ConnectionBlockCount);

                #ifdef __DEEP_TRACE
                ::printf("%*sConnection Blocks: %d\n", __TRACE_LEVEL * 2, "", ConnectionBlockCount);
                #endif

                {
                    TRACE_INDENT();

                    while (ConnectionBlockCount != 0)
                    {
                        Articulator.ConnectionBlocks.push_back(connection_block_t());

                        auto & ConnectionBlock = Articulator.ConnectionBlocks.back();

                        Read(ConnectionBlock.Source);
                        Read(ConnectionBlock.Control);
                        Read(ConnectionBlock.Destination);
                        Read(ConnectionBlock.Transform);
                        Read(ConnectionBlock.Scale);

                        #ifdef __DEEP_TRACE
                        ::printf("%*sSource: %d, Control: %3d, Destination: %3d, Transform: %d, Scale: %+11d\n", __TRACE_LEVEL * 2, "", ConnectionBlock.Source, ConnectionBlock.Control, ConnectionBlock.Destination, ConnectionBlock.Transform, ConnectionBlock.Scale);
                        #endif

                        --ConnectionBlockCount;
                    }

                    TRACE_UNINDENT();
                }

                TRACE_UNINDENT();
                break;
            }

            default:
            {
                if ((ch.Id & mmioFOURCC(0xFF, 0, 0, 0)) == mmioFOURCC('I', 0, 0, 0))
                {
                    properties_t Properties;

                    HandleIxxx(ch.Id, ch.Size, Properties);
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

    ReadChunks(ch.Size - sizeof(ch.Id), ChunkHandler);
}

/// <summary>
/// Read the waves.
/// </summary>
void reader_t::ReadWaves(const riff::chunk_header_t & ch, std::vector<wave_t> & waves)
{
    std::function<bool(const riff::chunk_header_t & ch)> ChunkHandler = [this, &waves, &ChunkHandler](const riff::chunk_header_t & ch) -> bool
    {
        switch (ch.Id)
        {
            // A LIST chunk contains an ordered sequence of subchunks.
            case FOURCC_LIST:
            {
                uint32_t ListType;

                Read(ListType);

                TRACE_LIST(ListType, ch.Size);
                TRACE_INDENT();

                switch (ListType)
                {
                    case FOURCC_wave:
                    {
                        waves.push_back(wave_t());

                        ReadWave(ch, waves.back());
                        break;
                    }

                    default:
                        ReadChunks(ch.Size - sizeof(ListType), ChunkHandler);
                }

                TRACE_UNINDENT();
                break;
            }

            default:
            {
                if ((ch.Id & mmioFOURCC(0xFF, 0, 0, 0)) == mmioFOURCC('I', 0, 0, 0))
                {
                    properties_t Properties;

                    HandleIxxx(ch.Id, ch.Size, Properties);
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

    ReadChunks(ch.Size - sizeof(ch.Id), ChunkHandler);
}

/// <summary>
/// Reads a wave.
/// </summary>
void reader_t::ReadWave(const riff::chunk_header_t & ch, wave_t & wave)
{
    std::function<bool(const riff::chunk_header_t & ch)> ChunkHandler = [this, &wave, &ChunkHandler](const riff::chunk_header_t & ch) -> bool
    {
        switch (ch.Id)
        {
            // The Wave Format chunk specifies the format of the wave data.
            case FOURCC_FMT:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                uint32_t Size = ch.Size;

                Read(wave.FormatTag);
                Read(wave.Channels);
                Read(wave.SamplesPerSec);
                Read(wave.AvgBytesPerSec);
                Read(wave.BlockAlign);

                Size -= sizeof(wave.FormatTag) + sizeof(wave.Channels) + sizeof(wave.SamplesPerSec) + sizeof(wave.AvgBytesPerSec) + sizeof(wave.BlockAlign);

                #ifdef __DEEP_TRACE
                ::printf("%*sFormat: 0x%04X, Channels: %d, SamplesPerSec: %d, AvgBytesPerSec: %d, BlockAlign: %d\n", __TRACE_LEVEL * 2, "",
                    wave.FormatTag, wave.Channels, wave.SamplesPerSec, wave.AvgBytesPerSec, wave.BlockAlign);
                #endif

                if (wave.FormatTag == WAVE_FORMAT_PCM)
                {
                    Read(wave.BitsPerSample);

                    Size -= sizeof(wave.BitsPerSample);

                    #ifdef __DEEP_TRACE
                    ::printf("%*sBitsPerSample: %d\n", __TRACE_LEVEL * 2, "", wave.BitsPerSample);
                    #endif

                    if ((wave.BitsPerSample != 8) && (wave.BitsPerSample != 16))
                        throw sf::exception(FormatText("%d-bit samples are not supported.", wave.BitsPerSample));
                }

                Skip(Size);

                TRACE_UNINDENT();
                break;
            }

            // The Wave Sample chunk describes the minimum necessary information needed to allow a synthesis engine to use a WAVE chunk. (Optional)
            case FOURCC_WSMP:
            {
                ReadWaveSample(ch, wave.WaveSample);
                break;
            }

            // The Wave Data chunk contains the waveform data.
            case FOURCC_DATA:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();
                {
                    if (_Options.ReadSampleData)
                    {
                        wave.Data.resize(ch.Size);
                        Read(wave.Data.data(), (uint32_t) wave.Data.size());
                    }
                    else
                        SkipChunk(ch);
                }
                TRACE_UNINDENT();
                break;
            }

            // A LIST chunk contains an ordered sequence of subchunks.
            case FOURCC_LIST:
            {
                uint32_t ListType;

                Read(ListType);

                TRACE_LIST(ListType, ch.Size);
                TRACE_INDENT();

                ReadChunks(ch.Size - sizeof(ListType), ChunkHandler);

                TRACE_UNINDENT();
                break;
            }

            default:
            {
                if ((ch.Id & mmioFOURCC(0xFF, 0, 0, 0)) == mmioFOURCC('I', 0, 0, 0))
                {
                    HandleIxxx(ch.Id, ch.Size, wave.Properties);
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

    ReadChunks(ch.Size - sizeof(ch.Id), ChunkHandler);

    wave.Name = GetPropertyValue(wave.Properties, FOURCC_INAM);
}

/// <summary>
/// Reads a wave sample. (1.14.10 Wave Sample)
/// </summary>
void reader_t::ReadWaveSample(const riff::chunk_header_t & ch, wave_sample_t & ws)
{
    TRACE_CHUNK(ch.Id, ch.Size);
    TRACE_INDENT();

    uint32_t Size;

    Read(Size);

    #ifdef __DEEP_TRACE
    if (Size == 0)
        ::printf("%*sInvalid Size field\n", __TRACE_LEVEL * 2, "");
    #endif

    Read(ws.UnityNote);
    Read(ws.FineTune);
    Read(ws.Gain);
    Read(ws.Options);

    uint32_t LoopCount;

    Read(LoopCount);    // Specifies the number (count) of loop records that are contained in the wave sample chunk. One-shot sounds will have the LoopCount field set to 0.

    ws.Loops.reserve(LoopCount);

    #ifdef __DEEP_TRACE
    ::printf("%*sUnityNote: %d, FineTune: %d, Gain: %d, Options: 0x%08X, Loops: %d\n", __TRACE_LEVEL * 2, "", ws.UnityNote, ws.FineTune, ws.Gain, ws.Options, LoopCount);
    #endif

    if (Size > 20)      // Size may be 0 in corrupt files.
        Skip(Size - 20);

    TRACE_INDENT();

    while (LoopCount != 0)
    {
        uint32_t LoopType;
        uint32_t LoopStart;
        uint32_t LoopLength;

        Read(Size);

        #ifdef __DEEP_TRACE
        if (Size == 0)
            ::printf("%*sInvalid Size field\n", __TRACE_LEVEL * 2, "");
        #endif

        Read(LoopType);     // Specifies the loop type.
        Read(LoopStart);    // Specifies the start point of the loop in samples as an absolute offset from the beginning of the data in the 'data' chunk of the sample.
        Read(LoopLength);   // Specifies the length of the loop in samples.

        if (Size > 16)      // Size may be 0 in corrupt files.
            Skip(Size - 16);

        ws.Loops.push_back(wave_sample_loop_t(LoopType, LoopStart, LoopLength));

        #ifdef __DEEP_TRACE
        ::printf("%*sLoop Type: %d, Start: %6d, Length: %6d\n", __TRACE_LEVEL * 2, "", LoopType, LoopStart, LoopLength);
        #endif

        LoopCount--;
    }

    TRACE_UNINDENT();

    TRACE_UNINDENT();
}
