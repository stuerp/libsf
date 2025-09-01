
/** $VER: DLSReader.h (2025.09.01) P. Stuer - Implements a reader for a DLS (Level 1 or 2)-compliant collection. **/

#pragma once

#include "pch.h"

#include "DLS.h"

#include <RIFF.h>

#define FOURCC_DLS  mmioFOURCC('D','L','S',' ')                     // Collection

#define FOURCC_LINS mmioFOURCC('l','i','n','s')                     // List of instruments
    #define FOURCC_INS mmioFOURCC('i','n','s',' ')                  // Instrument
//      #define FOURCC_DLID mmioFOURCC('d','l','i','d')             // (Optional) (DLS Level 2.2)
        #define FOURCC_LART mmioFOURCC('l','a','r','t')             // List of articulators (Level 1)
//          #define FOURCC_CDL  mmioFOURCC('c','d','l',' ')         // Conditional (Optional) (DLS Level 2.2)
            #define FOURCC_ART1 mmioFOURCC('a','r','t','1')         // Level 1 articulator
        #define FOURCC_INSH mmioFOURCC('i','n','s','h')             // Instrument Header
        #define FOURCC_LRGN mmioFOURCC('l','r','g','n')             // List of regions
            #define FOURCC_RGN mmioFOURCC('r','g','n',' ')          // Region
                #define FOURCC_RGNH mmioFOURCC('r','g','n','h')     // Region Header
                #define FOURCC_WLNK mmioFOURCC('w','l','n','k')     // Wave Link
                #define FOURCC_WSMP mmioFOURCC('w','s','m','p')     // Wave Sample
            #define FOURCC_RGN2 mmioFOURCC('r','g','n','2')         // Region 2 (DLS Level 2.2)
        #define FOURCC_LAR2 mmioFOURCC('l','a','r','2')             // List of articulators (Level 2) (Optional) (DLS Level 2.2)
//          #define FOURCC_CDL  mmioFOURCC('c','d','l',' ')         // Conditional (Optional) (DLS Level 2.2)
            #define FOURCC_ART2 mmioFOURCC('a','r','t','2')         // Level 2 articulator (DLS Level 2.2)
#define FOURCC_DLID mmioFOURCC('d','l','i','d')                     // (Optional) (DLS Level 2.2)
#define FOURCC_CDL  mmioFOURCC('c','d','l',' ')                     // Conditional (Optional) (DLS Level 2.2)
#define FOURCC_PTBL mmioFOURCC('p','t','b','l')                     // Pool Table
#define FOURCC_VERS mmioFOURCC('v','e','r','s')                     // Version (Optional)
#define FOURCC_WVPL mmioFOURCC('w','v','p','l')                     // Wave Pool
    #define FOURCC_wave mmioFOURCC('w','a','v','e')                 // Wave
//      #define FOURCC_DLID mmioFOURCC('d','l','i','d')             // (Optional) (DLS Level 2.2)
//      #define FOURCC_WAVE mmioFOURCC('w','s','m','p')             // Wave Sample
        #define FOURCC_FMT  mmioFOURCC('f','m','t',' ')             // Format of the wave data
        #define FOURCC_DATA mmioFOURCC('d','a','t','a')             // Wave data
#define FOURCC_COLH mmioFOURCC('c','o','l','h')                     // Collection Header

#pragma warning(disable: 4820)

namespace sf::dls
{

struct reader_options_t
{
    reader_options_t() { reader_options_t(true); }

    reader_options_t(bool readSampleData) : ReadSampleData(readSampleData) { }

    bool ReadSampleData;
};

constexpr uint32_t F_INSTRUMENT_DRUMS = 0x80000000;

class reader_t : public soundfont_reader_base_t
{
public:
    reader_t() noexcept : soundfont_reader_base_t() { }

    void Process(collection_t & dls, const reader_options_t & options);

private:
    void ReadInstruments(const riff::chunk_header_t & ch, std::vector<instrument_t> & instruments);
    void ReadInstrument(const riff::chunk_header_t & ch, instrument_t & instrument);

    void ReadRegions(const riff::chunk_header_t & ch, std::vector<region_t> & regions);
    void ReadRegion(const riff::chunk_header_t & ch, region_t & region);

    void ReadArticulators(const riff::chunk_header_t & ch, std::vector<articulator_t> & articulators);

    void ReadWaves(const riff::chunk_header_t & ch, std::vector<wave_t> & waves);
    void ReadWave(const riff::chunk_header_t & ch, wave_t & wave);

    void ReadWaveSample(const riff::chunk_header_t & ch, wave_sample_t & ws);

private:
    reader_options_t _Options;
};

}
