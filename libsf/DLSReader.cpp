
/** $VER: DLSReader.cpp (2025.03.14) P. Stuer **/

#include "pch.h"

#include "DLSReader.h"
#include "Exception.h"

#include <functional>

using namespace sf;

/// <summary>
/// Processes the complete SoundFont.
/// </summary>
void dls_reader_t::Process()
{
    TRACE_RESET();
    TRACE_INDENT();

    uint32_t FormType;

    ReadHeader(FormType);

    if (FormType != FOURCC_DLS)
        throw sf::exception("Unexpected header type");

    TRACE_FORM(FormType);
    TRACE_INDENT();

    std::function<bool(const riff::chunk_header_t & ch)> ChunkHandler = [this, &ChunkHandler](const riff::chunk_header_t & ch) -> bool
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

            case FOURCC_COLH:
            {
                // The Collection Header chunk defines an instrument collection.
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                uint32_t InstrumentCount;

                Read(&InstrumentCount, sizeof(InstrumentCount));

                #ifdef _TRACE
                ::printf("%*s%d instruments\n", __TRACE_LEVEL * 2, "", InstrumentCount);
                #endif

                TRACE_UNINDENT();
                break;
            }

            case FOURCC_VERS:
            {
                // The Version chunk defines an optional version stamp within a collection. It indicates the version of the contents of the file, not the DLS specification level.
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                uint16_t Major;
                uint16_t Minor;
                uint16_t Revision;
                uint16_t Build;

                Read(&Major, sizeof(Major));
                Read(&Minor, sizeof(Minor));
                Read(&Revision, sizeof(Revision));
                Read(&Build, sizeof(Build));

                #ifdef _TRACE
                ::printf("%*sVersion: %d.%d.%d.%d\n", __TRACE_LEVEL * 2, "", Major, Minor, Revision, Build);
                #endif

                TRACE_UNINDENT();
                break;
            }

            case FOURCC_DLID:
            {
                // The DLSID chunk defines an optional globally unique identifier (DLSID) for a complete <DLS-form> or for an element within it.
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                GUID Id = {};

                Read(&Id.Data1, sizeof(Id.Data1));
                Read(&Id.Data2, sizeof(Id.Data2));
                Read(&Id.Data3, sizeof(Id.Data3));
                Read(&Id.Data4, sizeof(Id.Data4));

                WCHAR Text[32] = {};

                (void) ::StringFromGUID2(Id, Text, _countof(Text));

                #ifdef _TRACE
                ::printf("%*sId: %s\n", __TRACE_LEVEL * 2, "", WideToUTF8(Text).c_str());
                #endif

                TRACE_UNINDENT();
                break;
            }

            case FOURCC_INSH:
            {
                // The Instrument Header chunk defines an instrument within a collection.
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                uint32_t RegionCount;   // cRegions
                // _MIDILOCALE
                uint32_t Bank;
                uint32_t Patch;

                Read(&RegionCount, sizeof(RegionCount));
                Read(&Bank, sizeof(Bank));
                Read(&Patch, sizeof(Patch));

                bool IsPercussion = ((Bank & F_INSTRUMENT_DRUMS) != 0);

                #ifdef _TRACE
                ::printf("%*sRegions: %d, Bank: CC0 0x%02X CC32 0x%02X (MMA %d), Is Percussion: %s, Program: %d\n", __TRACE_LEVEL * 2, "", RegionCount, (Bank >> 8) & 0x7F, Bank & 0x7F, (((Bank >> 8) & 0x7F) * 128 + (Bank & 0x7F)), IsPercussion ? "true" : "false", Patch & 0x7F);
                #endif

                TRACE_UNINDENT();
                break;
            }

            case FOURCC_RGNH:
            {
                // The Region Header defines a region within an instrument.
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                uint16_t LowKey;
                uint16_t HighKey;
                uint16_t LowVelocity;
                uint16_t HighVelocity;
                uint16_t NonExclusive;
                uint16_t KeyGroup;

                Read(&LowKey, sizeof(LowKey));
                Read(&HighKey, sizeof(HighKey));
                Read(&LowVelocity, sizeof(LowVelocity));
                Read(&HighVelocity, sizeof(HighVelocity));
                Read(&NonExclusive, sizeof(NonExclusive));
                Read(&KeyGroup, sizeof(KeyGroup));

                #ifdef _TRACE
                ::printf("%*sKey: %3d - %3d, Velocity: %3d - %3d, Non exclusive: %d, Key Group: %d\n", __TRACE_LEVEL * 2, "", LowKey, HighKey, LowVelocity, HighVelocity, NonExclusive, KeyGroup);
                #endif

                TRACE_UNINDENT();
                break;
            }

            case FOURCC_WSMP:
            {
                // The Wave Sample chunk describes the minimum necessary information needed to allow a synthesis engine to use a WAVE chunk.
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                uint32_t Size;
                uint16_t UnityNote;
                int16_t FineTune;
                int32_t Attenuation;
                uint32_t Options;
                uint32_t LoopCount;

                Read(&Size, sizeof(Size));

                if (Size == 20)
                {
                    Read(&UnityNote, sizeof(UnityNote));
                    Read(&FineTune, sizeof(FineTune));
                    Read(&Attenuation, sizeof(Attenuation));
                    Read(&Options, sizeof(Options));
                    Read(&LoopCount, sizeof(LoopCount));

                    #ifdef _TRACE
                    ::printf("%*sUnityNote: %d, FineTune: %d, Attenuation: %d, Options: 0x%08X, Loops: %d\n", __TRACE_LEVEL * 2, "", UnityNote, FineTune, Attenuation, Options, LoopCount);
                    #endif

                    TRACE_INDENT();

                    while (LoopCount != 0)
                    {
                        uint32_t LoopType;
                        uint32_t LoopStart;
                        uint32_t LoopLength;

                        Read(&Size, sizeof(Size));

                        if (Size != 16)
                        {
                            Skip(Size - 4);
                            continue;
                        }

                        Read(&LoopType, sizeof(LoopType));
                        Read(&LoopStart, sizeof(LoopStart));
                        Read(&LoopLength, sizeof(LoopLength));

                        #ifdef _TRACE
                        ::printf("%*sLoop Type: %d, Start: %6d, Length: %6d\n", __TRACE_LEVEL * 2, "", LoopType, LoopStart, LoopLength);
                        #endif

                        LoopCount--;
                    }

                    TRACE_UNINDENT();
                }
                else
                    Skip(Size - 4);

                TRACE_UNINDENT();
                break;
            }

            case FOURCC_WLNK:
            {
                // The Wave Link chunk specifies where the wave data can be found for an instrument region in a DLS file.
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                uint16_t Options;
                uint16_t PhaseGroup;
                uint32_t Channel;
                uint32_t TableIndex;

                Read(&Options, sizeof(Options));
                Read(&PhaseGroup, sizeof(PhaseGroup));
                Read(&Channel, sizeof(Channel));
                Read(&TableIndex, sizeof(TableIndex));

                #ifdef _TRACE
                ::printf("%*sOptions: 0x%08X, PhaseGroup: %d, Channel: %d, TableIndex: %d\n", __TRACE_LEVEL * 2, "", Options, PhaseGroup, Channel, TableIndex);
                #endif

                TRACE_UNINDENT();
                break;
            }

            case FOURCC_ART1:
            {
                // The Level 1 Articulator chunk specifies parameters which modify the playback of a wave file used in downloadable instruments.
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                uint32_t Size;
                uint32_t ConnectionBlockCount;

                Read(&Size, sizeof(Size));
                Read(&ConnectionBlockCount, sizeof(ConnectionBlockCount));

                #ifdef _TRACE
                ::printf("%*sConnection Blocks: %d\n", __TRACE_LEVEL * 2, "", ConnectionBlockCount);
                #endif

                {
                    TRACE_INDENT();

                    while (ConnectionBlockCount != 0)
                    {
                        uint16_t Source;
                        uint16_t Control;
                        uint16_t Destination;
                        uint16_t Transform;
                        int32_t Scale;

                        Read(&Source, sizeof(Source));
                        Read(&Control, sizeof(Control));
                        Read(&Destination, sizeof(Destination));
                        Read(&Transform, sizeof(Transform));
                        Read(&Scale, sizeof(Scale));

                        #ifdef _TRACE
                        ::printf("%*sSource: %d, Control: %3d, Destination: %3d, Transform: %d, Scale: %10d\n", __TRACE_LEVEL * 2, "", Source, Control, Destination, Transform, Scale);
                        #endif

                        --ConnectionBlockCount;
                    }

                    TRACE_UNINDENT();
                }

                TRACE_UNINDENT();
                break;
            }

            case FOURCC_ART2:
            {
                // The Level 2  Articulator chunk specifies parameters which modify the playback of a sample used in DLS Level 2 downloadable instruments.
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                uint32_t Size;
                uint32_t ConnectionBlockCount;

                Read(&Size, sizeof(Size));
                Read(&ConnectionBlockCount, sizeof(ConnectionBlockCount));

                #ifdef _TRACE
                ::printf("%*sConnection Blocks: %d\n", __TRACE_LEVEL * 2, "", ConnectionBlockCount);
                #endif

                {
                    TRACE_INDENT();

                    while (ConnectionBlockCount != 0)
                    {
                        uint16_t Source;
                        uint16_t Control;
                        uint16_t Destination;
                        uint16_t Transform;
                        int32_t Scale;

                        Read(&Source, sizeof(Source));
                        Read(&Control, sizeof(Control));
                        Read(&Destination, sizeof(Destination));
                        Read(&Transform, sizeof(Transform));
                        Read(&Scale, sizeof(Scale));

                        #ifdef _TRACE
                        ::printf("%*sSource: %d, Control: %3d, Destination: %3d, Transform: %d, Scale: %11d\n", __TRACE_LEVEL * 2, "", Source, Control, Destination, Transform, Scale);
                        #endif

                        --ConnectionBlockCount;
                    }

                    TRACE_UNINDENT();
                }

                TRACE_UNINDENT();
                break;
            }

            case FOURCC_PTBL:
            {
                // The Pool Table chunk contains a list of cross-reference entries to digital audio data within the wave pool.
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                uint32_t Size;
                uint32_t CueCount;

                Read(&Size, sizeof(Size));
                Read(&CueCount, sizeof(CueCount));

                #ifdef _TRACE
                ::printf("%*sCues: %d\n", __TRACE_LEVEL * 2, "", CueCount);
                #endif

                {
                    TRACE_INDENT();

                    while (CueCount != 0)
                    {
                        uint32_t Offset;

                        Read(&Offset, sizeof(Offset));

                        #ifdef _TRACE
                        ::printf("%*sOffset: %8d\n", __TRACE_LEVEL * 2, "", Offset);
                        #endif

                        --CueCount;
                    }

                    TRACE_UNINDENT();
                }

                TRACE_UNINDENT();
                break;
            }

            case FOURCC_FMT:
            {
                // The Format chunk specifies the format of the wave data.
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                uint32_t Size = ch.Size;

                #pragma pack(push, 2)
                struct fmt_common_t
                {
                    uint16_t FormatTag;
                    uint16_t Channels;
                    uint32_t SamplesPerSec;
                    uint32_t AvgBytesPerSec;
                    uint16_t BlockAlign;
                } Common;
                #pragma pack(pop)

                Read(&Common, sizeof(Common));

                Size -= sizeof(Common);

                #ifdef _TRACE
                ::printf("%*sFormat: 0x%04X, Channels: %d, SamplesPerSec: %d, AvgBytesPerSec: %d, BlockAlign: %d\n", __TRACE_LEVEL * 2, "",
                    Common.FormatTag, Common.Channels, Common.SamplesPerSec, Common.AvgBytesPerSec, Common.BlockAlign);
                #endif

                if (Common.FormatTag == WAVE_FORMAT_PCM) // mmeapi.h
                {
                    uint16_t BitsPerSample;

                    Read(&BitsPerSample, sizeof(BitsPerSample));

                    Size -= sizeof(BitsPerSample);

                    ::printf("%*sBitsPerSample: %d\n", __TRACE_LEVEL * 2, "", BitsPerSample);
                }
                else
                    throw sf::exception("Unknown wave data format");

                Skip(Size);

                TRACE_UNINDENT();
                break;
            }

            case FOURCC_DATA:
            {
                // The Data chunk contains the waveform data.
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                SkipChunk(ch);

                TRACE_UNINDENT();
                break;
            }

            default:
            {
                if ((ch.Id & mmioFOURCC(0xFF, 0, 0, 0)) == mmioFOURCC('I', 0, 0, 0))
                {
                    // Information chunks
                    HandleIxxx(ch.Id, ch.Size);
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
/// Handles an Ixxx chunk.
/// </summary>
bool dls_reader_t::HandleIxxx(uint32_t chunkId, uint32_t chunkSize)
{
    const char * Name = "Unknown";

    switch (chunkId)
    {
        case FOURCC_IARL: Name = "Archival Location"; break;
        case FOURCC_IART: Name = "Artist"; break;
        case FOURCC_ICMS: Name = "Commissioned"; break;
        case FOURCC_ICMT: Name = "Comments"; break;
        case FOURCC_ICOP: Name = "Copyright"; break;
        case FOURCC_ICRD: Name = "Creation Date"; break;
        case FOURCC_ICRP: Name = "Cropped"; break;
        case FOURCC_IDIM: Name = "Dimensions"; break;
        case FOURCC_IDPI: Name = "DPI"; break;
        case FOURCC_IENG: Name = "Engineer"; break;
        case FOURCC_IGNR: Name = "Genre"; break;
        case FOURCC_IKEY: Name = "Keywords"; break;
        case FOURCC_ILGT: Name = "Lightness"; break;
        case FOURCC_IMED: Name = "Medium"; break;
        case FOURCC_INAM: Name = "Name"; break;
        case FOURCC_IPLT: Name = "Palette"; break;
        case FOURCC_IPRD: Name = "Product"; break;
        case FOURCC_ISBJ: Name = "Subject"; break;
        case FOURCC_ISFT: Name = "Software"; break;
        case FOURCC_ISHP: Name = "Sharpness"; break;
        case FOURCC_ISRC: Name = "Source"; break;
        case FOURCC_ISRF: Name = "Source Form"; break;
        case FOURCC_ITCH: Name = "Technician"; break;
    }

    std::string Text;

    Text.resize((size_t) chunkSize + 1);

    Read((void *) Text.c_str(), chunkSize);

    Text[chunkSize] = 0;

    #ifdef _TRACE
    ::printf("%*s%s: \"%s\"\n", __TRACE_LEVEL * 2, "", Name, Text.c_str());
    #endif

    return true;
}
