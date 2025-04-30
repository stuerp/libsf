
/** $VER: main.cpp (2025.04.30) P. Stuer **/

#include "pch.h"

#include <pathcch.h>
#include <shlwapi.h>

#pragma comment(lib, "pathcch")
#pragma comment(lib, "shlwapi")

#include <libsf.h>

#include "Encoding.h"

static void ProcessDirectory(const std::wstring & directoryPath, const std::wstring & searchPattern);
static void ProcessFile(const std::wstring & filePath, uint64_t fileSize);
static void ProcessDLS(const std::wstring & filePath);
static void ProcessSF(const std::wstring & filePath);
static void ProcessECW(const std::wstring & filePath);
static void ConvertDLS(const sf::dls::collection_t & dls, sf::bank_t & sf);

static std::string DescribeModulatorController(uint16_t modulator) noexcept;
static std::string DescribeGeneratorController(uint16_t generator, uint16_t amount) noexcept;
static std::string DescribeSampleType(uint16_t sampleType) noexcept;

static const char * GetChunkName(const uint32_t chunkId) noexcept;

std::wstring FilePath;

const WCHAR * Filters[] = { L".dls", L".sbk", L".sf2", L".sf3", L".ecw" };

typedef std::unordered_map<uint32_t, const char *> info_map_t;

int wmain(int argc, wchar_t * argv[])
{
    ::printf("\xEF\xBB\xBF"); // UTF-8 BOM

    if (argc < 2)
    {
        ::printf("Error: Insufficient arguments.");
        return -1;
    }

    FilePath = argv[1];

    if (!::PathFileExistsW(FilePath.c_str()))
    {
        ::printf("Failed to access \"%s\": path does not exist.\n", riff::WideToUTF8(FilePath).c_str());
        return -1;
    }

    WCHAR DirectoryPath[MAX_PATH];

    if (::GetFullPathNameW(FilePath.c_str(), _countof(DirectoryPath), DirectoryPath, nullptr) == 0)
    {
        ::printf("Failed to expand \"%s\": Error %u.\n", riff::WideToUTF8(FilePath).c_str(), (uint32_t) ::GetLastError());
        return -1;
    }

    if (!::PathIsDirectoryW(FilePath.c_str()))
    {
        ::PathCchRemoveFileSpec(DirectoryPath, _countof(DirectoryPath));

        ProcessDirectory(DirectoryPath, ::PathFindFileNameW(FilePath.c_str()));
    }
    else
        ProcessDirectory(DirectoryPath, L"*.*");

    return 0;
}

/// <summary>
/// Processes a directory.
/// </summary>
static void ProcessDirectory(const std::wstring & directoryPath, const std::wstring & searchPattern)
{
    ::printf("\"%s\"\n", riff::WideToUTF8(directoryPath).c_str());

    WCHAR PathName[MAX_PATH];

    if (!SUCCEEDED(::PathCchCombineEx(PathName, _countof(PathName), directoryPath.c_str(), searchPattern.c_str(), PATHCCH_ALLOW_LONG_PATHS)))
        return;

    WIN32_FIND_DATA fd = {};

    HANDLE hFind = ::FindFirstFileW(PathName, &fd);

    if (hFind == INVALID_HANDLE_VALUE)
        return;

    BOOL Success = TRUE;

    if (::wcscmp(fd.cFileName, L".") == 0)
    {
        Success = ::FindNextFileW(hFind, &fd);

        if (Success && ::wcscmp(fd.cFileName, L"..") == 0)
            Success = ::FindNextFileW(hFind, &fd);
    }

    while (Success)
    {
        if (SUCCEEDED(::PathCchCombineEx(PathName, _countof(PathName), directoryPath.c_str(), fd.cFileName, PATHCCH_ALLOW_LONG_PATHS)))
        {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                ProcessDirectory(PathName, searchPattern);
            else
            {
                uint64_t FileSize = (((uint64_t) fd.nFileSizeHigh) << 32) + fd.nFileSizeLow;

                ProcessFile(PathName, FileSize);
            }
        }

        Success = ::FindNextFileW(hFind, &fd);
    }

    ::FindClose(hFind);
}

/// <summary>
/// Processes the specified file.
/// </summary>
static void ProcessFile(const std::wstring & filePath, uint64_t fileSize)
{
    ::printf("\n\"%s\", %llu bytes\n", riff::WideToUTF8(filePath).c_str(), fileSize);

    const WCHAR * FileExtension;

    HRESULT hr = ::PathCchFindExtension(filePath.c_str(), filePath.length() + 1, &FileExtension);

    if (!SUCCEEDED(hr))
        return;

    try
    {
        if (::_wcsicmp(FileExtension, L".dls") == 0)
            ProcessDLS(filePath);
        else
        if ((::_wcsicmp(FileExtension, L".sbk") == 0) || (::_wcsicmp(FileExtension, L".sf2") == 0) || (::_wcsicmp(FileExtension, L".sf3") == 0))
            ProcessSF(filePath);
        else
        if (::_wcsicmp(FileExtension, L".ecw") == 0)
            ProcessECW(filePath);
    }
    catch (sf::exception e)
    {
        ::printf("Failed to process sound font: %s\n\n", e.what());
    }
    catch (riff::exception e)
    {
        ::printf("Failed to process RIFF file: %s\n\n", e.what());
    }

    ::fflush(stdout);
}

/// <summary>
/// Processes a DLS collection.
/// </summary>
static void ProcessDLS(const std::wstring & filePath)
{
    sf::dls::collection_t dls;

    riff::file_stream_t fs;

    if (fs.Open(filePath))
    {
        sf::dls::reader_t dr;

        if (dr.Open(&fs, riff::reader_t::option_t::None))
            dr.Process({ false }, dls);

        fs.Close();
    }

#ifdef _DEBUG
    uint32_t __TRACE_LEVEL = 0;

    ::printf("%*sContent Version: %d.%d.%d.%d\n", __TRACE_LEVEL * 2, "", dls.Major, dls.Minor, dls.Revision, dls.Build);

    ::printf("%*s%zu instruments\n", __TRACE_LEVEL * 2, "", dls.Instruments.size());

    size_t i = 1;

    for (const auto & Instrument : dls.Instruments)
    {
        ::printf("%*s%4zu. Regions: %3zu, Articulators: %3zu, Bank: CC0 0x%02X CC32 0x%02X (MMA %5d), Program: %3d, Is Percussion: %-5s, Name: \"%s\"\n", __TRACE_LEVEL * 2, "", i++,
            Instrument.Regions.size(), Instrument.Articulators.size(), Instrument.BankMSB, Instrument.BankLSB, ((uint16_t) Instrument.BankMSB << 7) + Instrument.BankLSB, Instrument.Program, Instrument.IsPercussion ? "true" : "false", Instrument.Name.c_str());

        __TRACE_LEVEL += 2;

        ::printf("%*sRegions:\n", __TRACE_LEVEL * 2, "");

        __TRACE_LEVEL++;

        for (const auto & Region : Instrument.Regions)
        {
            ::printf("%*sKey: %3d - %3d, Velocity: %3d - %3d, Options: 0x%04X, Key Group: %d, Editing Layer: %d\n", __TRACE_LEVEL * 2, "",
                Region.LowKey, Region.HighKey, Region.LowVelocity, Region.HighVelocity, Region.Options, Region.KeyGroup, Region.Layer);
        }

        __TRACE_LEVEL--;
        __TRACE_LEVEL -=2;

        __TRACE_LEVEL += 2;

        ::printf("%*sArticulators:\n", __TRACE_LEVEL * 2, "");

        __TRACE_LEVEL++;

        for (const auto & Articulator : Instrument.Articulators)
        {
            ::printf("%*s%3zu connection blocks\n", __TRACE_LEVEL * 2, "", Articulator.ConnectionBlocks.size());

            __TRACE_LEVEL++;

            for (const auto & ConnectionBlock: Articulator.ConnectionBlocks)
            {
                ::printf("%*sSource: %d, Control: %3d, Destination: %3d, Transform: %d, Scale: %11d\n", __TRACE_LEVEL * 2, "",
                    ConnectionBlock.Source, ConnectionBlock.Control, ConnectionBlock.Destination, ConnectionBlock.Transform, ConnectionBlock.Scale);
            }

            __TRACE_LEVEL--;
        }

        __TRACE_LEVEL--;
        __TRACE_LEVEL -= 2;
    }

    ::putchar('\n');

    ::printf("%*s%zu waves\n", __TRACE_LEVEL * 2, "", dls.Waves.size());

    i = 1;

    for (const auto & Wave : dls.Waves)
    {
        ::printf("%*s%4zu. Channels: %d, %5d samples/s, %5d avg. bytes/s, Block Align: %5d, Name: \"%s\"\n", __TRACE_LEVEL * 2, "", i++,
            Wave.Channels, Wave.SamplesPerSec, Wave.AvgBytesPerSec, Wave.BlockAlign, Wave.Name.c_str());
    }

    ::putchar('\n');

    ::printf("%*sProperties:\n", __TRACE_LEVEL * 2, "");

    i = 1;

    for (const auto & [ ChunkId, Value ] : dls.Properties)
    {
        ::printf("%*s%4zu. %s: %s\n", __TRACE_LEVEL * 2, "", i++, GetChunkName(ChunkId), Value.c_str());
    }
#endif

    sf::bank_t sf;

    ConvertDLS(dls, sf);
}

/// <summary>
/// Processes an SF bank.
/// </summary>
static void ProcessSF(const std::wstring & filePath)
{
    sf::bank_t Bank;

    riff::memory_stream_t ms;

    if (ms.Open(filePath, 0, 0))
    {
        sf::soundfont_reader_t sr;

        if (sr.Open(&ms, riff::reader_t::option_t::None))
            sr.Process({ true }, Bank);

        ms.Close();
    }

#ifdef _DEBUG
    uint32_t __TRACE_LEVEL = 0;

    ::printf("%*sSoundFont specification version: v%d.%02d\n", __TRACE_LEVEL * 2, "", Bank.Major, Bank.Minor);
    ::printf("%*sSound Engine: \"%s\"\n", __TRACE_LEVEL * 2, "", Bank.SoundEngine.c_str());
    ::printf("%*sBank Name: \"%s\"\n", __TRACE_LEVEL * 2, "", Bank.BankName.c_str());

    if ((Bank.ROMName.length() != 0) && !((Bank.ROMMajor == 0) && (Bank.ROMMinor == 0)))
        ::printf("%*sSound Data ROM: %s v%d.%02d\n", __TRACE_LEVEL * 2, "", Bank.ROMName.c_str(), Bank.ROMMajor, Bank.ROMMinor);

    {
        ::printf("%*sProperties\n", __TRACE_LEVEL * 2, "");
        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & [ ChunkId, Value ] : Bank.Properties)
        {
            ::printf("%*s%s: \"%s\"\n", __TRACE_LEVEL * 2, "", GetChunkName(ChunkId), riff::CodePageToUTF8(850, Value.c_str(), Value.length()).c_str());
        }

        __TRACE_LEVEL--;
    }

    ::printf("%*sSample Data: %zu bytes\n", __TRACE_LEVEL * 2, "", Bank.SampleData.size());
    ::printf("%*sSample Data 24: %zu bytes\n", __TRACE_LEVEL * 2, "", Bank.SampleDataLSB.size());

    {
        ::printf("%*sPresets (%zu)\n", __TRACE_LEVEL * 2, "", Bank.Presets.size() - 1);
        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & Preset : Bank.Presets)
        {
            ::printf("%*s%5zu. \"%-20s\", MIDI Bank %3d, MIDI Program %3d, Zone %6d\n", __TRACE_LEVEL * 2, "", i++, Preset.Name.c_str(), Preset.MIDIBank, Preset.MIDIProgram, Preset.ZoneIndex);

            if (i == Bank.Presets.size() - 1)
                break;
        }

        __TRACE_LEVEL--;
    }

    {
        ::printf("%*sPreset Zones (%zu)\n", __TRACE_LEVEL * 2, "", Bank.PresetZones.size() - 1);

        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & pz : Bank.PresetZones)
        {
            ::printf("%*s%5zu. Generator: %5d, Modulator: %5d\n", __TRACE_LEVEL * 2, "", i++, pz.GeneratorIndex, pz.ModulatorIndex);

            if (i == Bank.PresetZones.size() - 1)
                break;
        }

        __TRACE_LEVEL--;
    }

    if (Bank.PresetZoneModulators.size() > 0)
    {
        ::printf("%*sPreset Zone Modulators (%zu)\n", __TRACE_LEVEL * 2, "", Bank.PresetZoneModulators.size() - 1);

        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & pzm : Bank.PresetZoneModulators)
        {
            ::printf("%*s%5zu. Src Op: 0x%04X, Dst Op: %2d, Amount: %6d, Amount Source: 0x%04X, Source Transform: 0x%04X, Src Op: \"%s\"\n", __TRACE_LEVEL * 2, "", i++,
                pzm.SrcOperator, pzm.DstOperator, pzm.Amount, pzm.AmountSource, pzm.SourceTransform, DescribeModulatorController(pzm.SrcOperator).c_str());

            if (i == Bank.PresetZoneModulators.size() - 1)
                break;
        }

        __TRACE_LEVEL--;
    }

    {
        ::printf("%*sPreset Zone Generators (%zu)\n", __TRACE_LEVEL * 2, "", Bank.PresetZoneGenerators.size() - 1);

        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & pzg : Bank.PresetZoneGenerators)
        {
            ::printf("%*s%5zu. Operator: 0x%04X, Amount: 0x%04X, \"%s\"\n", __TRACE_LEVEL * 2, "", i++,
                pzg.Operator, pzg.Amount, DescribeGeneratorController(pzg.Operator, pzg.Amount).c_str());

            if (pzg.Operator == 41 /* instrument */)
                ::putchar('\n');

            if (i == Bank.PresetZoneGenerators.size() - 1)
                break;
        }

        __TRACE_LEVEL--;
    }

    {
        ::printf("%*sInstruments (%zu)\n", __TRACE_LEVEL * 2, "", Bank.Instruments.size() - 1);

        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & Instrument : Bank.Instruments)
        {
            ::printf("%*s%5zu. \"%-20s\", Zone %6d\n", __TRACE_LEVEL * 2, "", i++, Instrument.Name.c_str(), Instrument.ZoneIndex);

            if (i == Bank.Instruments.size() - 1)
                break;
        }

        __TRACE_LEVEL--;
    }

    {
        ::printf("%*sInstrument Zones (%zu)\n", __TRACE_LEVEL * 2, "", Bank.InstrumentZones.size() - 1);

        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & iz : Bank.InstrumentZones)
        {
            ::printf("%*s%5zu. Generator %5d, Modulator %5d\n", __TRACE_LEVEL * 2, "", i++, iz.GeneratorIndex, iz.ModulatorIndex);

            if (i == Bank.InstrumentZones.size() - 1)
                break;
        }

        __TRACE_LEVEL--;
    }

    if (Bank.InstrumentZoneModulators.size() > 0)
    {
        ::printf("%*sInstrument Zone Modulators (%zu)\n", __TRACE_LEVEL * 2, "", Bank.InstrumentZoneModulators.size() - 1);

        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & izm : Bank.InstrumentZoneModulators)
        {
            ::printf("%*s%5zu. Src Op: 0x%04X, Dst Op: 0x%04X, Amount: %6d, Amount Source: 0x%04X, Source Transform: 0x%04X, Src Op: \"%s\", Dst Op: \"%s\"\n", __TRACE_LEVEL * 2, "", i++,
                izm.SrcOperator, izm.DstOperator, izm.Amount, izm.AmountSource, izm.SourceTransform, DescribeModulatorController(izm.SrcOperator).c_str(), DescribeModulatorController(izm.DstOperator).c_str());

            if (i == Bank.InstrumentZoneModulators.size() - 1)
                break;
        }

        __TRACE_LEVEL--;
    }

    {
        ::printf("%*sInstrument Zone Generators (%zu)\n", __TRACE_LEVEL * 2, "", Bank.InstrumentZoneGenerators.size() - 1);

        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & izg : Bank.InstrumentZoneGenerators)
        {
            ::printf("%*s%5zu. Operator: 0x%04X, Amount: 0x%04X, \"%s\"\n", __TRACE_LEVEL * 2, "", i++, izg.Operator, izg.Amount, DescribeGeneratorController(izg.Operator, izg.Amount).c_str());

            if (izg.Operator == 53 /* sampleID */)
                ::putchar('\n');

            if (i == Bank.InstrumentZoneGenerators.size() - 1)
                break;
        }

        __TRACE_LEVEL--;
    }

    if (Bank.Major == 1)
    {
        ::printf("%*sSample Names (%zu)\n", __TRACE_LEVEL * 2, "", Bank.SampleNames.size() - 1);
        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & SampleName : Bank.SampleNames)
        {
            ::printf("%*s%5zu. \"%-20s\"\n", __TRACE_LEVEL * 2, "", i++, SampleName.c_str());

            if (i == Bank.Samples.size() - 1)
                break;
        }

        __TRACE_LEVEL--;
    }

    {
        ::printf("%*sSamples (%zu)\n", __TRACE_LEVEL * 2, "", Bank.Samples.size() - 1);
        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & Sample : Bank.Samples)
        {
            ::printf("%*s%5zu. \"%-20s\", %9d-%9d, Loop: %9d-%9d, %6dHz, Pitch: %3d, Pitch Correction: %3d, Link: %5d, Type: 0x%04X \"%s\"\n", __TRACE_LEVEL * 2, "", i++,
                Sample.Name.c_str(), Sample.Start, Sample.End, Sample.LoopStart, Sample.LoopEnd,
                Sample.SampleRate, Sample.Pitch, Sample.PitchCorrection,
                Sample.SampleLink, Sample.SampleType, DescribeSampleType(Sample.SampleType).c_str());

            if (i == Bank.Samples.size() - 1)
                break;
        }

        __TRACE_LEVEL--;
    }
#endif

    riff::file_stream_t fs;

    WCHAR FilePath[MAX_PATH] = {};

    ::GetTempFileNameW(L".", L"SF2", 0, FilePath);

    ::printf("\n\"%s\"\n", riff::WideToUTF8(FilePath).c_str());

    // Tests the Sound Font writer.
    if (fs.Open(FilePath, true))
    {
        sf::soundfont_writer_t sw;

        if (sw.Open(&fs, riff::writer_t::option_t::None))
            sw.Process({ }, Bank);

        fs.Close();
    }

    // Reload the written Sound Font to test if we can read our own output.
    if (fs.Open(FilePath))
    {
        sf::soundfont_reader_t sr;

        if (sr.Open(&fs, riff::reader_t::option_t::None))
            sr.Process({ false }, Bank);

        fs.Close();
    }
}

/// <summary>
/// Processes an ECW wave set.
/// </summary>
static void ProcessECW(const std::wstring & filePath)
{
    ecw::waveset_t ws;

    riff::file_stream_t fs;

    if (fs.Open(filePath))
    {
        ecw::reader_t er;

        if (er.Open(&fs))
            er.Process(ws);

        fs.Close();
    }

#ifdef _DEBUG
    uint32_t __TRACE_LEVEL = 0;

    ::printf("%*sName: %s\n", __TRACE_LEVEL * 2, "", riff::CodePageToUTF8(850, ws.Name.c_str(), ws.Name.length()).c_str());
    ::printf("%*sCopyright: %s\n", __TRACE_LEVEL * 2, "", riff::CodePageToUTF8(850, ws.Copyright.c_str(), ws.Copyright.length()).c_str());
    ::printf("%*sDescription: %s\n", __TRACE_LEVEL * 2, "", riff::CodePageToUTF8(850, ws.Description.c_str(), ws.Description.length()).c_str());
    ::printf("%*sInformation: %s\n", __TRACE_LEVEL * 2, "", riff::CodePageToUTF8(850, ws.Information.c_str(), ws.Information.length()).c_str());
    ::printf("%*sFile Name: %s\n", __TRACE_LEVEL * 2, "", riff::CodePageToUTF8(850, ws.FileName.c_str(), ws.FileName.length()).c_str());

    {
        ::printf("\n%*sBank Maps\n", __TRACE_LEVEL * 2, "");
        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & BankMap : ws.BankMaps)
        {
            ::printf("%*sBank Map %zu\n", __TRACE_LEVEL * 2, "", i++);
            __TRACE_LEVEL++;

            size_t j = 0;

            for (const auto & MIDIPatchMap : BankMap.MIDIPatchMap)
            {
                ::printf("%*s%5zu. %5d\n", __TRACE_LEVEL * 2, "", j++, MIDIPatchMap);
            }

            __TRACE_LEVEL--;
        }

        __TRACE_LEVEL--;
    }

    {
        ::printf("\n%*sDrum Kit Maps\n", __TRACE_LEVEL * 2, "");
        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & DrumKitMap : ws.DrumKitMaps)
        {
            ::printf("%*sDrum Kit Map %zu\n", __TRACE_LEVEL * 2, "", i++);
            __TRACE_LEVEL++;

            size_t j = 0;

            for (const auto & DrumNoteMap : DrumKitMap.DrumNoteMap)
            {
                ::printf("%*s%5zu. %5d\n", __TRACE_LEVEL * 2, "", j++, DrumNoteMap);
            }

            __TRACE_LEVEL--;
        }

        __TRACE_LEVEL--;
    }

    {
        ::printf("\n%*sMIDI Patch Maps\n", __TRACE_LEVEL * 2, "");
        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & MIDIPatchMap : ws.MIDIPatchMaps)
        {
            ::printf("%*sMIDI Patch Map %zu\n", __TRACE_LEVEL * 2, "", i++);
            __TRACE_LEVEL++;

            size_t j = 0;

            for (const auto & Instrument : MIDIPatchMap.Instrument)
            {
                ::printf("%*s%5zu. %5d\n", __TRACE_LEVEL * 2, "", j++, Instrument);
            }

            __TRACE_LEVEL--;
        }

        __TRACE_LEVEL--;
    }

    {
        ::printf("\n%*sDrum Note Maps\n", __TRACE_LEVEL * 2, "");
        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & DrumNoteMap : ws.DrumNoteMaps)
        {
            ::printf("%*sDrum Note Map %zu\n", __TRACE_LEVEL * 2, "", i++);
            __TRACE_LEVEL++;

            size_t j = 0;

            for (const auto & Instrument : DrumNoteMap.Instrument)
            {
                ::printf("%*s%5zu. %5d\n", __TRACE_LEVEL * 2, "", j++, Instrument);
            }

            __TRACE_LEVEL--;
        }

        __TRACE_LEVEL--;
    }

    {
        ::printf("\n%*sInstruments\n", __TRACE_LEVEL * 2, "");
        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & InstrumentHeader : ws.InstrumentHeaders)
        {
            if (InstrumentHeader.Type == 2)
            {
                const auto & ih = (ecw::instrument_header_v1_t &) InstrumentHeader;

                ::printf("%*s%5zu. v1, Sub Type: %d, Note: %3d, Patch: %5d, Amplitude: %4d, Pan: %3d, Coarse Tune: %4d, Fine Tune: %4d, Delay: %5d, Group: %3d\n", __TRACE_LEVEL * 2, "", i++,
                    ih.SubType,
                    ih.NoteThreshold,
                    ih.SubHeaders[0].Patch,
                    ih.SubHeaders[0].Amplitude,
                    ih.SubHeaders[0].Pan,
                    ih.SubHeaders[0].CoarseTune,
                    ih.SubHeaders[0].FineTune,
                    ih.SubHeaders[0].Delay,
                    ih.SubHeaders[0].Group
                );
            }
            else
            if (InstrumentHeader.Type == 255)
            {
                const auto & ih = (ecw::instrument_header_v2_t &) InstrumentHeader;

                ::printf("%*s%5zu. v2", __TRACE_LEVEL * 2, "", i++);

                for (const auto & sh : ih.SubHeaders)
                    ::printf(", Index: %5d, Note: %3d", sh.Index, sh.NoteThreshold);

                ::putchar('\n');
            }
            else
                ::printf("%*s%5zu. Unknown instrument type\n", __TRACE_LEVEL * 2, "", i++);
        }

        __TRACE_LEVEL--;
    }

    {
        uint32_t SampleStart = ~0;

        ::printf("\n%*sSamples\n", __TRACE_LEVEL * 2, "");
        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & SampleHeader : ws.SampleHeaders)
        {
            ::printf("%*s%5zu. %3d, Flags: 0x%02X, Fine Tune: %4d, Coarse Tune: %4d, Offset: 0x%08X, Loop: 0x%08X-%08X\n", __TRACE_LEVEL * 2, "", i++,
                SampleHeader.NoteMax,
                SampleHeader.Flags,
                SampleHeader.FineTune,
                SampleHeader.CoarseTune,
                SampleHeader.SampleStart,
                SampleHeader.LoopStart,
                SampleHeader.LoopStop
            );

            if (SampleHeader.SampleStart < SampleStart)
                SampleStart = SampleHeader.SampleStart;
        }

        __TRACE_LEVEL--;
    }
#endif
}

/// <summary>
/// Describes a SoundFont modulator controller.
/// </summary>
static std::string DescribeModulatorController(uint16_t modulator) noexcept
{
    std::string Text;

    if (modulator & 0x0080) //  MIDI Continuous Controller Flag set?
    {
        char t[64];

        // Use the MIDI Controller palette of controllers.
        ::snprintf(t, _countof(t), "MIDI Controller %d", modulator & 0x007F);

        Text = t;
    }
    else
    {
        // Use the General Controller palette of controllers.
        switch (modulator & 0x007F)
        {
            case   0: Text = "No controller"; break;            // No controller is to be used. The output of this controller module should be treated as if its value were set to ‘1’. It should not be a means to turn off a modulator.

            case   2: Text = "Note-On Velocity"; break;         // The controller source to be used is the velocity value which is sent from the MIDI note-on command which generated the given sound.
            case   3: Text = "Note-On Key Number"; break;       // The controller source to be used is the key number value which was sent from the MIDI note-on command which generated the given sound.
            case  10: Text = "Poly Pressure"; break;            // The controller source to be used is the poly-pressure amount that is sent from the MIDI poly-pressure command.
            case  13: Text = "Channel Pressure"; break;         // The controller source to be used is the channel pressure amount that is sent from the MIDI channel-pressure command.
            case  14: Text = "Pitch Wheel"; break;              // The controller source to be used is the pitch wheel amount which is sent from the MIDI pitch wheel command
            case  16: Text = "Pitch Wheel Sensitivity"; break;  // The controller source to be used is the pitch wheel sensitivity amount which is sent from the MIDI RPN 0 pitch wheel sensitivity command.
            case 127: Text = "Link"; break;                     // The controller source is the output of another modulator. This is NOT SUPPORTED as an Amount Source.

            default: Text = "Reserved"; break;
        }
    }

    // Direction
    Text.append((modulator & 0x0100) ? ", Max to min" : ", Min to max");

    // Polarity
    Text.append((modulator & 0x0200) ? ", -1 to 1 (Bipolar)" : ", 0 to 1 (Unipolar)");

    // Type
    switch (modulator >> 10)
    {
        case   0: Text += ", Linear"; break;    // The controller moves linearly from the minimum to the maximum value in the direction and with the polarity specified by the ‘D’ and ‘P’ bits.
        case   1: Text += ", Concave"; break;   // The controller moves in a concave fashion from the minimum to the maximum value in the direction and with the polarity specified by the ‘D’ and ‘P’ bits.
        case   2: Text += ", Convex"; break;    // The controller moves in a concex fashion from the minimum to the maximum value in the direction and with the polarity specified by the ‘D’ and ‘P’ bits.
        case   3: Text += ", Switch"; break;    // The controller output is at a minimum value while the controller input moves from the minimum to half of the maximum, after which the controller output is at a maximum. This occurs in the direction and with the polarity specified by the ‘D’ and ‘P’ bits.

        default: Text += ", Reserved"; break;
    }

    return Text;
}

/// <summary>
/// Describes a SoundFont generator controller.
/// </summary>
static std::string DescribeGeneratorController(uint16_t generator, uint16_t amount) noexcept
{
    std::string Text;

    switch (generator & 0x007F)
    {
        case  0: Text = riff::FormatText("Start Address Offset: %d data points (startAddrsOffset)", (int16_t) amount); break;
        case  1: Text = riff::FormatText("End Address Offset: %d data points (endAddrsOffset)", (int16_t) amount); break;
        case  2: Text = riff::FormatText("Start Loop Address Offset: %d data points (startloopAddrsOffset)", (int16_t) amount); break;
        case  3: Text = riff::FormatText("End Loop Address Offset: %d data points (endloopAddrsOffset)", (int16_t) amount); break;

        case  4: Text = riff::FormatText("Start Address Coarse Offset: %d data points (startAddrsCoarseOffset)", (int32_t) amount * 32768); break;

        case  5: Text = "modLfoToPitch"; break;
        case  6: Text = "vibLfoToPitch"; break;
        case  7: Text = "modEnvToPitch"; break;

        case  8: Text = "initialFilterFc"; break;
        case  9: Text = "initialFilterQ"; break;

        case 10: Text = "modLfoToFilterFc"; break;
        case 11: Text = "modEnvToFilterFc"; break;

        case 12: Text = riff::FormatText("End Address Coarse Offset: %d data points (endAddrsCoarseOffset)", (int32_t) amount * 32768); break;

        case 13: Text = riff::FormatText("Modulation LFO Volume Influence: %d centibels (modLfoToVolume)", (int16_t) amount); break;

        case 14: Text = "Unused"; break;

        case 15: Text = riff::FormatText("Chorus: %.1f%% (chorusEffectsSend)", (int16_t) amount / 10.); break;
        case 16: Text = riff::FormatText("Reverb: %.1f%% (reverbEffectsSend)", (int16_t) amount / 10.); break;
        case 17: Text = riff::FormatText("Pan: %.1f%% (pan)", (int16_t) amount / 10.f); break;

        case 18: Text = "Unused"; break;
        case 19: Text = "Unused"; break;
        case 20: Text = "Unused"; break;

        case 21: Text = riff::FormatText("Modulation LFO Delay: %.0f ms (delayModLFO)", std::exp2((int16_t) amount / 1200.) * 1000.); break;
        case 22: Text = riff::FormatText("Modulation LFO Frequency: %.0f mHz (freqModLFO)", std::exp2((int16_t) amount / 1200.) * 8176.); break;

        case 23: Text = riff::FormatText("Vibrato LFO Delay: %.0f ms (delayVibLFO)", std::exp2((int16_t) amount / 1200.) * 1000.); break;
        case 24: Text = riff::FormatText("Vibrato LFO Frequency: %.0f mHz (freqVibLFO)", std::exp2((int16_t) amount / 1200.) * 8176.); break;

        case 25: Text = riff::FormatText("Modulation Envelope Delay: %.0f ms (delayModEnv)", std::exp2((int16_t) amount / 1200.) * 1000.); break;
        case 26: Text = riff::FormatText("Modulation Envelope Attack: %.0f ms (attackModEnv)", std::exp2((int16_t) amount / 1200.) * 1000.); break;
        case 27: Text = riff::FormatText("Modulation Envelope Hold: %.0f ms (holdModEnv)", std::exp2((int16_t) amount / 1200.) * 1000.); break;
        case 28: Text = riff::FormatText("Modulation Envelope Decay: %.0f ms (decayModEnv)", std::exp2((int16_t) amount / 1200.) * 1000.); break;
        case 29: Text = riff::FormatText("Modulation Envelope Sustain: %.0f dB (sustainModEnv)", (int16_t) amount / 10.); break;
        case 30: Text = riff::FormatText("Modulation Envelope Release: %.0f ms (releaseModEnv)", std::exp2((int16_t) amount / 1200.) * 1000.); break;
        case 31: Text = riff::FormatText("Modulation Envelope Hold Decrease: %.0f ms (keynumToModEnvHold)", std::exp2((int16_t) amount / 1200.) * 1000.); break;
        case 32: Text = riff::FormatText("Modulation Envelope Decay Decrease: %.0f ms (keynumToModEnvDecay)", std::exp2((int16_t) amount / 1200.) * 1000.); break;

        case 33: Text = riff::FormatText("Volume Envelope Delay: %.0f ms (delayVolEnv)", std::exp2((int16_t) amount / 1200.) * 1000.); break;
        case 34: Text = riff::FormatText("Volume Envelope Attack: %.0f ms (attackVolEnv)", std::exp2((int16_t) amount / 1200.) * 1000.); break;
        case 35: Text = riff::FormatText("Volume Envelope Hold: %.0f ms (holdVolEnv)", std::exp2((int16_t) amount / 1200.) * 1000.); break;
        case 36: Text = riff::FormatText("Volume Envelope Decay: %.0f ms (decayVolEnv)", std::exp2((int16_t) amount / 1200.) * 1000.); break;
        case 37: Text = riff::FormatText("Volume Envelope Sustain: %.0f dB (sustainVolEnv)", (int16_t) amount / 10.); break;
        case 38: Text = riff::FormatText("Volume Envelope Release: %.0f ms (releaseVolEnv)", std::exp2((int16_t) amount / 1200.) * 1000.); break;
        case 39: Text = riff::FormatText("Volume Envelope Hold Decrease: %.0f ms (keynumToVolEnvHold)", std::exp2((int16_t) amount / 1200.) * 1000.); break;
        case 40: Text = riff::FormatText("Volume Envelope Decay Decrease: %.0f ms (keynumToVolEnvDecay)", std::exp2((int16_t) amount / 1200.) * 1000.); break;

        case 41: Text = riff::FormatText("Instrument Index: %d (instrument)", amount); break;

        case 42: Text = "Reserved"; break;

        case 43: Text = riff::FormatText("Key Range: %d - %d (keyRange)", amount & 0x00FF, (amount >> 8) & 0x00FF); break;
        case 44: Text = riff::FormatText("Velocity: Range %d - %d (velocityRange)", amount & 0x00FF, (amount >> 8) & 0x00FF); break;

        case 45: Text = riff::FormatText("Start Loop Address Coarse Offset: %d data points (startloopAddrsCoarseOffset)", (int32_t) amount * 32768); break;

        case 46: Text = riff::FormatText("MIDI Key: %d (keynum)", amount); break;
        case 47: Text = riff::FormatText("MIDI Velocity: %d (velocity)", amount); break;
        case 48: Text = riff::FormatText("Initial Attenuation: %.0f dB (initialAttenuation)", (int16_t) amount / 10.); break;

        case 49: Text = "Reserved"; break;

        case 50: Text = riff::FormatText("End Loop Address Coarse Offset: %d data points (endloopAddrsCoarseOffset)", (int32_t) amount * 32768); break;

        case 51: Text = riff::FormatText("Coarse Tune: %d semitones (coarseTune)", (int16_t) amount); break;
        case 52: Text = riff::FormatText("Fine Tune: %d cents (fineTune)", (int16_t) amount); break;

        case 53: Text = riff::FormatText("Sample: %d (sampleID)", amount); break;           // Index of the sample
        case 54: Text = riff::FormatText("Sample Mode: %d (sampleModes)", amount); break;   // 0 = no loop, 1 = loop, 2 = reserved, 3 = loop and play till the end in release phase

        case 55: Text = "Reserved"; break;

        case 56: Text = riff::FormatText("Scale Tuning: %d (scaleTuning)", (int16_t) amount); break;
        case 57: Text = riff::FormatText("Exclusive Class: %s (exclusiveClass)", (amount != 0) ? "Yes" : "No"); break;
        case 58: Text = riff::FormatText("Overriding Root Key: %d (overridingRootKey)", amount); break;

        case 59: Text = "Unused"; break;
        case 60: Text = "Unused"; break;

        default: Text = "Unknown"; break;
    }

    return Text;
}

/// <summary>
/// Describes a sample type.
/// </summary>
static std::string DescribeSampleType(uint16_t sampleType) noexcept
{
    std::string Text;

    switch (sampleType)
    {
        case sf::SampleTypes::MonoSample: Text = "Mono Sample"; break;
        case sf::SampleTypes::RightSample: Text = "Right Sample"; break;
        case sf::SampleTypes::LeftSample: Text = "Left Sample"; break;
        case sf::SampleTypes::LinkedSample: Text = "Linked Sample"; break;

        case sf::SampleTypes::RomMonoSample: Text = "Mono ROM Sample"; break;
        case sf::SampleTypes::RomRightSample: Text = "Right ROM Sample"; break;
        case sf::SampleTypes::RomLeftSample: Text = "Left ROM Sample"; break;
        case sf::SampleTypes::RomLinkedSample: Text = "Linked ROM Sample"; break;

        default: Text = "Unknown sample type"; break;
    }

    return Text;
}

/// <summary>
/// Gets the name of a chunk.
/// </summary>
static const char * GetChunkName(const uint32_t chunkId) noexcept
{
    const info_map_t ChunkNames =
    {
        { FOURCC_IARL, "Archival Location" },
        { FOURCC_IART, "Artist" },
        { FOURCC_ICMS, "Commissioned" },
        { FOURCC_ICMT, "Comments" },
        { FOURCC_ICOP, "Copyright" },
        { FOURCC_ICRD, "Creation Date" },
        { FOURCC_ICRP, "Cropped" },
        { FOURCC_IDIM, "Dimensions" },
        { FOURCC_IDPI, "DPI" },
        { FOURCC_IENG, "Engineer" },
        { FOURCC_IGNR, "Genre" },
        { FOURCC_IKEY, "Keywords" },
        { FOURCC_ILGT, "Lightness" },
        { FOURCC_IMED, "Medium" },
        { FOURCC_INAM, "Name" },
        { FOURCC_IPLT, "Palette" },
        { FOURCC_IPRD, "Product" },
        { FOURCC_ISBJ, "Subject" },
        { FOURCC_ISFT, "Software" },
        { FOURCC_ISHP, "Sharpness" },
        { FOURCC_ISRC, "Source" },
        { FOURCC_ISRF, "Source Form" },
        { FOURCC_ITCH, "Technician" },
    };

    auto it = ChunkNames.find(chunkId);

    return (it != ChunkNames.end()) ? it->second : "Unknown";
}

/// <summary>
/// Converts a DLS collection to a SoundFont bank.
/// </summary>
static void ConvertDLS(const sf::dls::collection_t & dls, sf::bank_t & bank)
{
    for (const auto & Instrument : dls.Instruments)
    {
        bank.Instruments.push_back(sf::instrument_t(Instrument.Name, -1));
    }

    bank.Properties = dls.Properties;
}
