
/** $VER: SoundFontWriter.h (2025.04.27) P. Stuer **/

#pragma once

#include "pch.h"

#include "SoundFont.h"

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

struct soundfont_writer_options_t
{
};

class soundfont_writer_t : public soundfont_writer_base_t
{
public:
    soundfont_writer_t() noexcept : soundfont_writer_base_t() { }

    void Process(const soundfont_writer_options_t & options, soundfont_t & sf);
};

}
