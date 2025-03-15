
/** $VER: DLSReader.h (2025.03.14) P. Stuer **/

#pragma once

#include "pch.h"

#include "SoundFont.h"

#include <libriff.h>

#define FOURCC_DLS  mmioFOURCC('D','L','S',' ')                     // Collection

#define FOURCC_LINS mmioFOURCC('l','i','n','s')                     // List of instruments
    #define FOURCC_INS mmioFOURCC('i','n','s',' ')                  // Instrument
        #define FOURCC_LART mmioFOURCC('l','a','r','t')             // List of articulators (Level 1)
            #define FOURCC_ART1 mmioFOURCC('a','r','t','1')         // Level 1 articulator
        #define FOURCC_INSH mmioFOURCC('i','n','s','h')             // Instrument Header
        #define FOURCC_LRGN mmioFOURCC('l','r','g','n')             // List of regions
            #define FOURCC_RGN mmioFOURCC('r','g','n',' ')          // Region
                #define FOURCC_RGNH mmioFOURCC('r','g','n','h')     // Region Header
                #define FOURCC_WLNK mmioFOURCC('w','l','n','k')     // Wave Link
                #define FOURCC_WSMP mmioFOURCC('w','s','m','p')     // Wave Sample
            #define FOURCC_RGN2 mmioFOURCC('r','g','n','2')         // Region 2
        #define FOURCC_LAR2 mmioFOURCC('l','a','r','2')             // List of articulators (Level 2) (Optional)
            #define FOURCC_ART2 mmioFOURCC('a','r','t','2')         // Level 2 articulator
#define FOURCC_DLID mmioFOURCC('d','l','i','d')                     // (Optional)
#define FOURCC_CDL  mmioFOURCC('c','d','l',' ')                     // Conditional (Optional)
#define FOURCC_PTBL mmioFOURCC('p','t','b','l')                     // Pool Table
#define FOURCC_VERS mmioFOURCC('v','e','r','s')                     // Version (Optional)
#define FOURCC_WVPL mmioFOURCC('w','v','p','l')                     // Wave Pool
//  #define FOURCC_WAVE mmioFOURCC('w','a','v','e')                 // Wave
//      #define FOURCC_WAVE mmioFOURCC('w','s','m','p')             // Wave Sample
        #define FOURCC_FMT  mmioFOURCC('f','m','t',' ')             // Format of the wave data
        #define FOURCC_DATA mmioFOURCC('d','a','t','a')             // Wave data
#define FOURCC_COLH mmioFOURCC('c','o','l','h')                     // Collection Header

namespace sf
{

// "insh" chunk
struct _MIDILOCALE
{
    ULONG ulBank;
    ULONG ulInstrument;
};

constexpr uint32_t F_INSTRUMENT_DRUMS = 0x08000000;

class dls_reader_t : public riff::reader_t
{
public:
    dls_reader_t() { }

    void Process();

private:
    bool HandleIxxx(uint32_t chunkId, uint32_t chunkSize);
};

}
