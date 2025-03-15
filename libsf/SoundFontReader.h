
/** $VER: SFxReader.h (2025.03.14) P. Stuer **/

#pragma once

#include "pch.h"

#include "SoundFont.h"

#include <libriff.h>

#define FOURCC_SFBK mmioFOURCC('s','f','b','k')

// INFO list
    #define FOURCC_IFIL mmioFOURCC('i','f','i','l')
    #define FOURCC_ISNG mmioFOURCC('i','s','n','g')
    #define FOURCC_IROM mmioFOURCC('i','r','o','m')
    #define FOURCC_IVER mmioFOURCC('i','v','e','r')

// sdta list
    #define FOURCC_SMPL mmioFOURCC('s','m','p','l')
    #define FOURCC_SM24 mmioFOURCC('s','m','2','4')

// pdta list
    #define FOURCC_PHDR mmioFOURCC('p','h','d','r')
    #define FOURCC_PBAG mmioFOURCC('p','b','a','g')
    #define FOURCC_PMOD mmioFOURCC('p','m','o','d')
    #define FOURCC_PGEN mmioFOURCC('p','g','e','n')

    #define FOURCC_INST mmioFOURCC('i','n','s','t')
    #define FOURCC_IBAG mmioFOURCC('i','b','a','g')
    #define FOURCC_IMOD mmioFOURCC('i','m','o','d')
    #define FOURCC_IGEN mmioFOURCC('i','g','e','n')

    #define FOURCC_SHDR mmioFOURCC('s','h','d','r')

namespace sf
{

struct soundfont_reader_options_t
{
    bool ReadSampleData = true;
};

class soundfont_reader_t : public riff::reader_t
{
public:
    soundfont_reader_t() { }

    void Process(const soundfont_reader_options_t & options, soundfont_t & sf);

private:
    bool HandleIxxx(uint32_t chunkId, uint32_t chunkSize, soundfont_t & sf);

    static std::string DescribeModulatorController(uint16_t modulator) noexcept;
    static std::string DescribeGeneratorController(uint16_t modulator) noexcept;
};

}
