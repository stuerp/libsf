
/** $VER: BaseTypes.cpp (2025.03.23) P. Stuer - Base class for sound fonts **/

#include "pch.h"

#include "libsf.h"

#include <functional>

using namespace sf;

/// <summary>
/// Handles an Ixxx chunk.
/// </summary>
bool soundfont_reader_base_t::HandleIxxx(uint32_t chunkId, uint32_t chunkSize, property_map_t & properties)
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

    std::string Value;

    Value.resize((size_t) chunkSize + 1);

    Read((void *) Value.c_str(), chunkSize);

    Value[chunkSize] = 0;

    properties.insert({ Name, Value });

    #ifdef __DEEP_TRACE
    ::printf("%*s%s: \"%s\"\n", __TRACE_LEVEL * 2, "", Name, Value.c_str());
    #endif

    return true;
}
