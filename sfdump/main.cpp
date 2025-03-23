
/** $VER: main.cpp (2025.03.23) P. Stuer **/

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
static void ConvertDLS(const sf::dls::soundfont_t & dls, sf::soundfont_t & sf);

std::wstring FilePath;

const WCHAR * Filters[] = { L".dls", L".sf2", L".sf3" };

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
        ::printf("Failed to access \"%s\": path does not exist.\n", ::WideToUTF8(FilePath).c_str());
        return -1;
    }

    WCHAR DirectoryPath[MAX_PATH];

    if (::GetFullPathNameW(FilePath.c_str(), _countof(DirectoryPath), DirectoryPath, nullptr) == 0)
    {
        ::printf("Failed to expand \"%s\": Error %u.\n", ::WideToUTF8(FilePath).c_str(), (uint32_t) ::GetLastError());
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
    ::printf("\"%s\"\n", ::WideToUTF8(directoryPath).c_str());

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
    ::printf("\n\"%s\", %llu bytes\n", ::WideToUTF8(filePath).c_str(), fileSize);

    const WCHAR * FileExtension;

    HRESULT hr = ::PathCchFindExtension(filePath.c_str(), filePath.length() + 1, &FileExtension);

    if (!SUCCEEDED(hr))
        return;

    try
    {
        if (::_wcsicmp(FileExtension, L".dls") == 0)
            ProcessDLS(filePath);
        else
        if ((::_wcsicmp(FileExtension, L".sf2") == 0) || (::_wcsicmp(FileExtension, L".sf3") == 0))
            ProcessSF(filePath);
    }
    catch (sf::exception e)
    {
        ::printf("Failed to process sound font: %s\n\n", e.what());
    }
}

/// <summary>
/// Processes a DLS sound font.
/// </summary>
static void ProcessDLS(const std::wstring & filePath)
{
    sf::dls::soundfont_t dls;

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

    ::printf("%*s%llu instruments\n", __TRACE_LEVEL * 2, "", dls.Instruments.size());

    size_t i = 1;

    for (const auto & Instrument : dls.Instruments)
    {
        ::printf("%*s%4llu. Regions: %3llu, Articulators: %3llu, Bank: CC0 0x%02X CC32 0x%02X (MMA %5d), Program: %3d, Is Percussion: %-5s, Name: \"%s\"\n", __TRACE_LEVEL * 2, "", i++,
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
            ::printf("%*s%3llu connection blocks\n", __TRACE_LEVEL * 2, "", Articulator.ConnectionBlocks.size());

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

    ::printf("%*s%llu waves\n", __TRACE_LEVEL * 2, "", dls.Waves.size());

    i = 1;

    for (const auto & Wave : dls.Waves)
    {
        ::printf("%*s%4llu. Channels: %d, %5d samples/s, %5d avg. bytes/s, Block Align: %5d, Name: \"%s\"\n", __TRACE_LEVEL * 2, "", i++,
            Wave.Channels, Wave.SamplesPerSec, Wave.AvgBytesPerSec, Wave.BlockAlign, Wave.Name.c_str());
    }

    ::putchar('\n');

    ::printf("%*sProperties:\n", __TRACE_LEVEL * 2, "");

    i = 1;

    for (const auto & [ Name, Value ] : dls.Properties)
    {
        ::printf("%*s%4llu. %s: %s\n", __TRACE_LEVEL * 2, "", i++, Name.c_str(), Value.c_str());
    }
#endif

    sf::soundfont_t sf;

    ConvertDLS(dls, sf);
}

/// <summary>
/// Processes an SF sound font.
/// </summary>
static void ProcessSF(const std::wstring & filePath)
{
    sf::soundfont_t sf;

    riff::memory_stream_t ms;

    if (ms.Open(filePath, 0, 0))
    {
        sf::soundfont_reader_t sr;

        if (sr.Open(&ms, riff::reader_t::option_t::None))
            sr.Process({ false }, sf);

        ms.Close();
    }

#ifdef _DEBUG
    uint32_t __TRACE_LEVEL = 0;

    ::printf("%*sSoundFont specification version: %d.%d\n", __TRACE_LEVEL * 2, "", sf.Major, sf.Minor);
    ::printf("%*sSound Engine: %s\n", __TRACE_LEVEL * 2, "", sf.SoundEngine.c_str());
    ::printf("%*sSound Data ROM: %s v%d.%d\n", __TRACE_LEVEL * 2, "", sf.ROM.c_str(), sf.ROMMajor, sf.ROMMinor);

    {
        ::printf("%*sProperties\n", __TRACE_LEVEL * 2, "");
        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & Item : sf.Properties)
        {
            ::printf("%*s%s: %s\n", __TRACE_LEVEL * 2, "", Item.first.c_str(), Item.second.c_str());
        }

        __TRACE_LEVEL--;
    }

    ::printf("%*sSample Data: %zu bytes\n", __TRACE_LEVEL * 2, "", sf.SampleData.size());

    {
        ::printf("%*sPresets\n", __TRACE_LEVEL * 2, "");
        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & Preset : sf.Presets)
        {
            ::printf("%*s%5zu. %-20s, Zone %6d, Bank %3d, Preset %3d\n", __TRACE_LEVEL * 2, "", ++i, Preset.Name.c_str(), Preset.ZoneIndex, Preset.Bank, Preset.Program);
        }

        __TRACE_LEVEL--;
    }

    {
        ::printf("%*sPreset Zones: %zu\n", __TRACE_LEVEL * 2, "", sf.PresetZones.size());
    }

    {
        ::printf("%*sPreset Zone Modulators: %zu\n", __TRACE_LEVEL * 2, "", sf.PresetZoneModulators.size());
    }

    {
        ::printf("%*sPreset Zone Generators: %zu\n", __TRACE_LEVEL * 2, "", sf.PresetZoneGenerators.size());
    }

    {
        ::printf("%*sInstruments\n", __TRACE_LEVEL * 2, "");
        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & Instrument : sf.Instruments)
        {
            ::printf("%*s%5zu. %-20s, Zone %6d\n", __TRACE_LEVEL * 2, "", ++i, Instrument.Name.c_str(), Instrument.ZoneIndex);
        }

        __TRACE_LEVEL--;
    }

    {
        ::printf("%*sInstrument Zones: %zu\n", __TRACE_LEVEL * 2, "", sf.InstrumentZones.size());
    }

    {
        ::printf("%*sInstrument Zone Modulators: %zu\n", __TRACE_LEVEL * 2, "", sf.InstrumentZoneModulators.size());
    }

    {
        ::printf("%*sInstrument Zone Generators: %zu\n", __TRACE_LEVEL * 2, "", sf.InstrumentZoneGenerators.size());
    }

    {
        ::printf("%*sSamples\n", __TRACE_LEVEL * 2, "");
        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & Sample : sf.Samples)
        {
            ::printf("%*s%5zu. \"%-20s\", %9d-%9d, Loop: %9d-%9d, %6dHz, Pitch: %3d, Pitch Correction: %3d, Type: 0x%04X, Link: %5d\n", __TRACE_LEVEL * 2, "", ++i,
                Sample.Name.c_str(), Sample.Start, Sample.End, Sample.LoopStart, Sample.LoopEnd,
                Sample.SampleRate, Sample.Pitch, Sample.PitchCorrection,
                Sample.SampleType, Sample.SampleLink);
        }

        __TRACE_LEVEL--;
    }
#endif
}

/// <summary>
/// Converts a DLS sound font to a SoundFont sound font.
/// </summary>
static void ConvertDLS(const sf::dls::soundfont_t & dls, sf::soundfont_t & sf)
{
    for (const auto & Instrument : dls.Instruments)
    {
        sf.Instruments.push_back(sf::instrument_t(Instrument.Name, -1));
    }

    for (const auto & Samples : dls.Samples)
    {
    }

    sf.Properties = dls.Properties;
}
