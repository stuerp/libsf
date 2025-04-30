
/** $VER: BaseTypes.cpp (2025.04.30) P. Stuer - Base types for sound font handling **/

#include "pch.h"

#include "libsf.h"

#include <functional>

using namespace sf;

/// <summary>
/// Handles an Ixxx chunk.
/// </summary>
bool soundfont_reader_base_t::HandleIxxx(uint32_t chunkId, uint32_t chunkSize, properties_t & properties)
{
    std::string Value;

    Value.resize((size_t) chunkSize + 1);

    Read((void *) Value.data(), chunkSize);

    Value[chunkSize] = '\0';

    properties.push_back(property_t(chunkId, Value));

    return true;
}
