
/** $VER: main.cpp (2025.04.27) P. Stuer **/

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
static void ConvertDLS(const sf::dls::collection_t & dls, sf::soundfont_t & sf);

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
}

/// <summary>
/// Processes a DLS sound font.
/// </summary>
static void ProcessDLS(const std::wstring & filePath)
{
    sf::dls::collection_t dls;

    ::file_stream_t fs;

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

    sf::soundfont_t sf;

    ConvertDLS(dls, sf);
}

/// <summary>
/// Processes an SF sound font.
/// </summary>
static void ProcessSF(const std::wstring & filePath)
{
    sf::soundfont_t sf;

    ::memory_stream_t ms;

    if (ms.Open(filePath, 0, 0))
    {
        sf::soundfont_reader_t sr;

        if (sr.Open(&ms, riff::reader_t::option_t::None))
            sr.Process({ true }, sf);

        ms.Close();
    }

#ifdef _DEBUG
    uint32_t __TRACE_LEVEL = 0;

    ::printf("%*sSoundFont specification version: %d.%d\n", __TRACE_LEVEL * 2, "", sf.Major, sf.Minor);
    ::printf("%*sSound Engine: \"%s\"\n", __TRACE_LEVEL * 2, "", sf.SoundEngine.c_str());
    ::printf("%*sBank Name: \"%s\"\n", __TRACE_LEVEL * 2, "", sf.BankName.c_str());

    if ((sf.ROMName.length() != 0) && !((sf.ROMMajor == 0) && (sf.ROMMinor == 0)))
        ::printf("%*sSound Data ROM: %s v%d.%d\n", __TRACE_LEVEL * 2, "", sf.ROMName.c_str(), sf.ROMMajor, sf.ROMMinor);

    {
        ::printf("%*sProperties\n", __TRACE_LEVEL * 2, "");
        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & [ ChunkId, Value ] : sf.Properties)
        {
            ::printf("%*s%s: \"%s\"\n", __TRACE_LEVEL * 2, "", GetChunkName(ChunkId), Value.c_str());
        }

        __TRACE_LEVEL--;
    }

    ::printf("%*sSample Data: %zu bytes\n", __TRACE_LEVEL * 2, "", sf.SampleData.size());

    {
        ::printf("%*sPresets (%zu)\n", __TRACE_LEVEL * 2, "", sf.Presets.size() - 1);
        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & Preset : sf.Presets)
        {
            ::printf("%*s%5zu. \"%-20s\", Zone %6d, Bank %3d, Program %3d\n", __TRACE_LEVEL * 2, "", i++, Preset.Name.c_str(), Preset.ZoneIndex, Preset.Bank, Preset.Program);

            if (i == sf.Presets.size() - 1)
                break;
        }

        __TRACE_LEVEL--;
    }

    {
        ::printf("%*sPreset Zones (%zu)\n", __TRACE_LEVEL * 2, "", sf.PresetZones.size() - 1);

        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & pz : sf.PresetZones)
        {
            ::printf("%*s%5zu. Generator: %5d, Modulator: %5d\n", __TRACE_LEVEL * 2, "", i++, pz.GeneratorIndex, pz.ModulatorIndex);

            if (i == sf.PresetZones.size() - 1)
                break;
        }

        __TRACE_LEVEL--;
    }

    if (sf.PresetZoneModulators.size() > 0)
    {
        ::printf("%*sPreset Zone Modulators (%zu)\n", __TRACE_LEVEL * 2, "", sf.PresetZoneModulators.size() - 1);

        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & pzm : sf.PresetZoneModulators)
        {
            ::printf("%*s%5zu. Src Op: 0x%04X, Dst Op: %2d, Amount: %6d, Amount Source: 0x%04X, Source Transform: 0x%04X, Src Op: \"%s\"\n", __TRACE_LEVEL * 2, "", i++,
                pzm.SrcOperator, pzm.DstOperator, pzm.Amount, pzm.AmountSource, pzm.SourceTransform, DescribeModulatorController(pzm.SrcOperator).c_str());

            if (i == sf.PresetZoneModulators.size() - 1)
                break;
        }

        __TRACE_LEVEL--;
    }

    {
        ::printf("%*sPreset Zone Generators (%zu)\n", __TRACE_LEVEL * 2, "", sf.PresetZoneGenerators.size() - 1);

        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & pzg : sf.PresetZoneGenerators)
        {
            ::printf("%*s%5zu. Operator: 0x%04X, Amount: 0x%04X, \"%s\"\n", __TRACE_LEVEL * 2, "", i++,
                pzg.Operator, pzg.Amount, DescribeGeneratorController(pzg.Operator, pzg.Amount).c_str());

            if (i == sf.PresetZoneGenerators.size() - 1)
                break;
        }

        __TRACE_LEVEL--;
    }

    {
        ::printf("%*sInstruments (%zu)\n", __TRACE_LEVEL * 2, "", sf.Instruments.size() - 1);

        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & Instrument : sf.Instruments)
        {
            ::printf("%*s%5zu. \"%-20s\", Zone %6d\n", __TRACE_LEVEL * 2, "", i++, Instrument.Name.c_str(), Instrument.ZoneIndex);

            if (i == sf.Instruments.size() - 1)
                break;
        }

        __TRACE_LEVEL--;
    }

    {
        ::printf("%*sInstrument Zones (%zu)\n", __TRACE_LEVEL * 2, "", sf.InstrumentZones.size() - 1);

        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & iz : sf.InstrumentZones)
        {
            ::printf("%*s%5zu. Generator %5d, Modulator %5d\n", __TRACE_LEVEL * 2, "", i++, iz.GeneratorIndex, iz.ModulatorIndex);

            if (i == sf.InstrumentZones.size() - 1)
                break;
        }

        __TRACE_LEVEL--;
    }

    if (sf.InstrumentZoneModulators.size() > 0)
    {
        ::printf("%*sInstrument Zone Modulators (%zu)\n", __TRACE_LEVEL * 2, "", sf.InstrumentZoneModulators.size() - 1);

        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & izm : sf.InstrumentZoneModulators)
        {
            ::printf("%*s%5zu. Src Op: 0x%04X, Dst Op: 0x%04X, Amount: %6d, Amount Source: 0x%04X, Source Transform: 0x%04X, Src Op: \"%s\", Dst Op: \"%s\"\n", __TRACE_LEVEL * 2, "", i++,
                izm.SrcOperator, izm.DstOperator, izm.Amount, izm.AmountSource, izm.SourceTransform, DescribeModulatorController(izm.SrcOperator).c_str(), DescribeModulatorController(izm.DstOperator).c_str());

            if (i == sf.InstrumentZoneModulators.size() - 1)
                break;
        }

        __TRACE_LEVEL--;
    }

    {
        ::printf("%*sInstrument Zone Generators (%zu)\n", __TRACE_LEVEL * 2, "", sf.InstrumentZoneGenerators.size() - 1);

        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & izg : sf.InstrumentZoneGenerators)
        {
            ::printf("%*s%5zu. Operator: 0x%04X, Amount: 0x%04X, \"%s\"\n", __TRACE_LEVEL * 2, "", i++, izg.Operator, izg.Amount, DescribeGeneratorController(izg.Operator, izg.Amount).c_str());

            if (i == sf.InstrumentZoneGenerators.size() - 1)
                break;
        }

        __TRACE_LEVEL--;
    }

    {
        ::printf("%*sSamples (%zu)\n", __TRACE_LEVEL * 2, "", sf.Samples.size() - 1);
        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & Sample : sf.Samples)
        {
            ::printf("%*s%5zu. \"%-20s\", %9d-%9d, Loop: %9d-%9d, %6dHz, Pitch: %3d, Pitch Correction: %3d, Link: %5d, Type: 0x%04X \"%s\"\n", __TRACE_LEVEL * 2, "", i++,
                Sample.Name.c_str(), Sample.Start, Sample.End, Sample.LoopStart, Sample.LoopEnd,
                Sample.SampleRate, Sample.Pitch, Sample.PitchCorrection,
                Sample.SampleLink, Sample.SampleType, DescribeSampleType(Sample.SampleType).c_str());

            if (i == sf.Samples.size() - 1)
                break;
        }

        __TRACE_LEVEL--;
    }
#endif

    ::file_stream_t fs;

    WCHAR FilePath[MAX_PATH] = {};

    ::GetTempFileNameW(L".", L"SF2", 0, FilePath);

    ::printf("\n\"%s\"\n", ::WideToUTF8(FilePath).c_str());

    if (fs.Open(FilePath, true))
    {
        sf::soundfont_writer_t sw;

        if (sw.Open(&fs, riff::writer_t::option_t::None))
            sw.Process({ }, sf);

        fs.Close();
    }

    if (fs.Open(FilePath))
    {
        sf::soundfont_reader_t sr;

        if (sr.Open(&fs, riff::reader_t::option_t::None))
            sr.Process({ false }, sf);

        fs.Close();
    }
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
        case  0: Text = "Start Offset"; break;
        case  1: Text = "End Offset"; break;
        case  2: Text = "Loop Start Offset"; break;
        case  3: Text = "Loop End Offset"; break;

        case  4: Text = ::FormatText("Start Coarse Offset %d (startAddrsCoarseOffset)", amount); break;

        case  5: Text = "Modulation LFO to Pitch"; break;
        case  6: Text = "Vibrato LFO to Pitch"; break;
        case  7: Text = "Modulation Envelope to Pitch"; break;

        case  8: Text = "Initial Lowpass Filter Cutoff"; break;
        case  9: Text = "initial Lowpass Filter Gain"; break;

        case 10: Text = "modLfoToFilterFc"; break;
        case 11: Text = "modEnvToFilterFc"; break;

        case 12: Text = "endAddrsCoarseOffset"; break;
        case 13: Text = "modLfoToVolume"; break;

        case 14: Text = "Unused"; break;

        case 15: Text = ::FormatText("Chorus %.1f%% (chorusEffectsSend)", (int16_t) amount / 10.f); break;
        case 16: Text = ::FormatText("Reverb %.1f%% (reverbEffectsSend)", (int16_t) amount / 10.f); break;
        case 17: Text = ::FormatText("Pan %.1f%% (pan)", (int16_t) amount / 10.f); break;

        case 18: Text = "Unused"; break;
        case 19: Text = "Unused"; break;
        case 20: Text = "Unused"; break;

        case 21: Text = "delayModLFO"; break;
        case 22: Text = "freqModLFO"; break;
        case 23: Text = "delayVibLFO"; break;
        case 24: Text = "freqVibLFO"; break;

        case 25: Text = "delayModEnv"; break;
        case 26: Text = "attackModEnv"; break;
        case 27: Text = "holdModEnv"; break;
        case 28: Text = "decayModEnv"; break;
        case 29: Text = "sustainModEnv"; break;
        case 30: Text = "releaseModEnv"; break;
        case 31: Text = "keynumToModEnvHold"; break;
        case 32: Text = "keynumToModEnvDecay"; break;

        case 33: Text = "delayVolEnv"; break;
        case 34: Text = "attackVolEnv"; break;
        case 35: Text = "holdVolEnv"; break;
        case 36: Text = "decayVolEnv"; break;
        case 37: Text = "sustainVolEnv"; break;
        case 38: Text = ::FormatText("Volume Envelope Release %d (releaseVolEnv)", (int16_t) amount); break;
        case 39: Text = "keynumToVolEnvHold"; break;
        case 40: Text = "keynumToVolEnvDecay"; break;

        case 41: Text = ::FormatText("Instrument Index %d (instrument)", amount); break;

        case 42: Text = "Reserved"; break;

        case 43: Text = ::FormatText("Key Range %d - %d (keyRange)", amount & 0x00FF, (amount >> 8) & 0x00FF); break; // Start and End MIDI key
        case 44: Text = "Velocity Range"; break;
        case 45: Text = "startloopAddrsCoarseOffset"; break;
        case 46: Text = "keynum"; break;
        case 47: Text = "Velocity"; break;
        case 48: Text = "Initial Attenuation"; break;

        case 49: Text = "Reserved"; break;

        case 50: Text = "endloopAddrsCoarseOffset"; break;
        case 51: Text = "coarseTune"; break;
        case 52: Text = "fineTune"; break;
        case 53: Text = ::FormatText("Sample %d (sampleID)", amount); break;                    // Index of the sample
        case 54: Text = ::FormatText("Sample Mode %d (sampleModes)", amount); break;            // 0 = no loop, 1 = loop, 2 = reserved, 3 = loop and play till the end in release phase

        case 55: Text = "Reserved"; break;

        case 56: Text = "Scale Tuning"; break;
        case 57: Text = "Exclusive Class"; break;
        case 58: Text = ::FormatText("Overriding Root Key %d (overridingRootKey)", amount); break;

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
/// Converts a DLS sound font to a SoundFont sound font.
/// </summary>
static void ConvertDLS(const sf::dls::collection_t & dls, sf::soundfont_t & sf)
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

/// <summary>
/// Processes an ECW sound font.
/// </summary>
static void ProcessECW(const std::wstring & filePath)
{
    ecw::waveset_t ecw;

    ::file_stream_t fs;

    if (fs.Open(filePath))
    {
        ecw::reader_t er;

        if (er.Open(&fs))
            er.Process(ecw);

        fs.Close();
    }

#ifdef _DEBUG
    uint32_t __TRACE_LEVEL = 0;

    ::printf("%*sName: %s\n", __TRACE_LEVEL * 2, "", ecw.Name.c_str());
    ::printf("%*sCopyright: %s\n", __TRACE_LEVEL * 2, "", ecw.Copyright.c_str());
    ::printf("%*sDescription: %s\n", __TRACE_LEVEL * 2, "", ecw.Description.c_str());
    ::printf("%*sInformation: %s\n", __TRACE_LEVEL * 2, "", ecw.Information.c_str());
    ::printf("%*sFile Name: %s\n", __TRACE_LEVEL * 2, "", ecw.FileName.c_str());

    {
        ::printf("\n%*sBank Maps\n", __TRACE_LEVEL * 2, "");
        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & BankMap : ecw.BankMaps)
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

        for (const auto & DrumKitMap : ecw.DrumKitMaps)
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

        for (const auto & MIDIPatchMap : ecw.MIDIPatchMaps)
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

        for (const auto & DrumNoteMap : ecw.DrumNoteMaps)
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

        for (const auto & InstrumentHeader : ecw.InstrumentHeaders)
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

        for (const auto & SampleHeader : ecw.SampleHeaders)
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
