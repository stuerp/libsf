
/** $VER: BaseTypes.cpp (2025.04.24) P. Stuer - Base class for sound fonts **/

#include "pch.h"

#include "libsf.h"

#include <functional>

using namespace sf;

/// <summary>
/// Handles an Ixxx chunk.
/// </summary>
bool soundfont_reader_base_t::HandleIxxx(uint32_t chunkId, uint32_t chunkSize, info_map_t & infoMap)
{
    std::string Value;

    Value.resize((size_t) chunkSize + 1);

    Read((void *) Value.data(), chunkSize);

    Value[chunkSize] = '\0';

    infoMap.insert({ chunkId, Value });

    return true;
}
