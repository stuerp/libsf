
/** $VER: ECWReader.cpp (2025.04.30) P. Stuer - Implements a reader for a ECW wave set. **/

#include "pch.h"

#include "ECWReader.h"
#include "Exception.h"
#include "Encoding.h"

#include <functional>

using namespace ecw;

/// <summary>
/// Processes the complete sound font.
/// </summary>
void reader_t::Process(waveset_t & ws)
{
    header_t Header = { };

    Read(Header);

    ws.Name = Header.Name;
    ws.Copyright = Header.Copyright;
    ws.Description = Header.Description;
    ws.Information = Header.Information;
    ws.FileName = Header.FileName;

    ws.BankMaps.resize(Header.BankMapCount);
    Offset(Header.BankMapOffs);
    Read(ws.BankMaps.data(), Header.BankMapSize);

    ws.DrumKitMaps.resize(Header.DrumKitMapCount);
    Offset(Header.DrumKitMapOffs);
    Read(ws.DrumKitMaps.data(), Header.DrumKitMapSize);

    ws.MIDIPatchMaps.resize(Header.MIDIPatchMapCount);
    Offset(Header.MIDIPatchMapOffs);
    Read(ws.MIDIPatchMaps.data(), Header.MIDIPatchMapSize);

    ws.DrumNoteMaps.resize(Header.DrumNoteMapCount);
    Offset(Header.DrumNoteMapOffs);
    Read(ws.DrumNoteMaps.data(), Header.DrumNoteMapSize);

    ws.InstrumentHeaders.resize(Header.InstrumentHeaderCount);
    Offset(Header.InstrumentHeaderOffs);
    Read(ws.InstrumentHeaders.data(), Header.InstrumentHeaderSize);

    ws.PatchHeaders.resize(Header.PatchHeaderCount);
    Offset(Header.PatchHeaderOffs);
    Read(ws.PatchHeaders.data(), Header.PatchHeaderSize);

    ws.Array1.resize(Header.Array1Count);
    Offset(Header.Array1Offs);
    Read(ws.Array1.data(), Header.Array1Size);

    ws.Array2.resize(Header.Array2Count);
    Offset(Header.Array2Offs);
    Read(ws.Array2.data(), Header.Array2Size);

    ws.Array3.resize(Header.Array3Count);
    Offset(Header.Array3Offs);
    Read(ws.Array3.data(), Header.Array3Size);

    ws.SampleHeaders.resize(Header.SampleHeaderCount);
    Offset(Header.SampleHeaderOffs);
    Read(ws.SampleHeaders.data(), Header.SampleHeaderSize);

    for (auto & SampleHeader : ws.SampleHeaders)
    {
        // Offsets are read as values in bits. Convert to byte offsets.
        SampleHeader.SampleStart /= 8;
        SampleHeader.LoopStart /= 8;
        SampleHeader.LoopStop /= 8;
    }
}
