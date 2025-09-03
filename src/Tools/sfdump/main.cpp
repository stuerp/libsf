
/** $VER: main.cpp (2025.08.25) P. Stuer **/

#include "pch.h"

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 4738 5045 ALL_CPPCORECHECK_WARNINGS)

#include <libsf.h>

#include "Encoding.h"

using namespace sf;

static void ProcessDirectory(const fs::path & directoryPath);
static void ProcessFile(const fs::path & filePath);
static void ExamineFile(const fs::path & filePath);

static void ProcessDLS(const fs::path & filePath);
static void ProcessSF(const fs::path & filePath);
static void ProcessECW(const fs::path & filePath);

static void ConvertECW(const ecw::waveset_t & ws, sf::bank_t & bank);

static void DumpPresets(const bank_t & bank) noexcept;
static void DumpPresetZoneList(const bank_t & bank, size_t fromIndex, size_t toIndex) noexcept;
static void DumpPresetZoneGenerators(const bank_t & bank, size_t fromIndex, size_t toIndex) noexcept;
static void DumpPresetZoneModulators(const bank_t & bank, size_t fromIndex, size_t toIndex) noexcept;

static void DumpInstruments(const bank_t & bank) noexcept;
static void DumpInstrumentZoneList(const bank_t & bank, size_t fromIndex, size_t toIndex) noexcept;
static void DumpInstrumentZoneGenerators(const bank_t & bank, size_t fromIndex, size_t toIndex) noexcept;
static void DumpInstrumentZoneModulators(const bank_t & bank, size_t fromIndex, size_t toIndex) noexcept;

static void DumpPresetZones(const bank_t & bank) noexcept;

static void DumpSamples(const bank_t & bank) noexcept;

static const char * GetChunkName(const uint32_t chunkId) noexcept;

fs::path FilePath;
uint32_t __TRACE_LEVEL = 0;

const std::vector<fs::path> Filters = { ".sbk", ".sf2", ".sf3", ".dls", ".ecw" };

class arguments_t
{
public:
    void Initialize(int argc, char * argv[]) noexcept
    {
        for (int i = 1; i < argc; ++i)
        {
            if (argv[i][0] == '-')
            {
                if ((::_stricmp(argv[i], "-all") == 0) || (::_stricmp(argv[i], "-presets") == 0)) Items["presets"] = "";

                if (::_stricmp(argv[i], "-presetzones") == 0)          Items["presetzones"] = "";
                if (::_stricmp(argv[i], "-presetzonemodulators") == 0) Items["presetzonemodulators"] = "";
                if (::_stricmp(argv[i], "-presetzonegenerators") == 0) Items["presetzonegenerators"] = "";

                if ((::_stricmp(argv[i], "-all") == 0) || (::_stricmp(argv[i], "-instruments") == 0)) Items["instruments"] = "";

                if (::_stricmp(argv[i], "-instrumentzones") == 0)          Items["instrumentzones"] = "";
                if (::_stricmp(argv[i], "-instrumentzonemodulators") == 0) Items["instrumentzonemodulators"] = "";
                if (::_stricmp(argv[i], "-instrumentzonegenerators") == 0) Items["instrumentzonegenerators"] = "";

                if (::_stricmp(argv[i], "-samplenames") == 0) Items["samplenames"] = "";

                if ((::_stricmp(argv[i], "-all") == 0) || (::_stricmp(argv[i], "-samples") == 0)) Items["samples"] = "";

            }
            else
            if (Items["pathname"].empty())
                Items["pathname"] = argv[i];
        }
    }

    std::string operator[] (const std::string & key)
    {
        return Items[key];
    }

    bool IsSet(const std::string & key) const noexcept
    {
        return Items.contains(key);
    }

private:
    std::map<std::string, std::string> Items;
};

arguments_t Arguments;

typedef std::unordered_map<uint32_t, const char *> info_map_t;

int main(int argc, char * argv[])
{
    Arguments.Initialize(argc, argv);

    if (argc < 2)
    {
        ::printf("Error: Insufficient arguments.");

        return -1;
    }

    if (!::fs::exists(Arguments["pathname"]))
    {
        ::printf("Failed to access \"%s\": path does not exist.\n", Arguments["pathname"].c_str());

        return -1;
    }

    fs::path Path = std::filesystem::canonical(Arguments["pathname"]);

    if (fs::is_directory(Path))
        ProcessDirectory(Path);
    else
        ProcessFile(Path);

    return 0;
}

/// <summary>
/// Returns true if the string matches one of the list.
/// </summary>
static bool IsOneOf(const fs::path & item, const std::vector<fs::path> & list) noexcept
{
    for (const auto & Item : list)
    {
        if (::_stricmp(item.string().c_str(), Item.string().c_str()) == 0)
            return true;
    }

    return false;
}

static void ProcessDirectory(const fs::path & directoryPath)
{
    ::printf("\"%s\"\n", (const char *) directoryPath.u8string().c_str());

    for (const auto & Entry : fs::directory_iterator(directoryPath))
    {
        if (Entry.is_directory())
        {
            ProcessDirectory(Entry.path());
        }
        else
        if (IsOneOf(Entry.path().extension(), Filters))
        {
            ProcessFile(Entry.path());
        }
    }
}

/// <summary>
/// Processes the specified file.
/// </summary>
static void ProcessFile(const fs::path & filePath)
{
    int StdOutFD = ::_dup(::_fileno(stdout));

    FILE * fp = nullptr;

    fs::path StdOut = filePath;

    if ((::freopen_s(&fp, StdOut.replace_extension(".log").string().c_str(), "w", stdout) != 0) || (fp == nullptr))
        return;

    ::printf("\xEF\xBB\xBF"); // UTF-8 BOM

    auto FileSize = fs::file_size(filePath);

    ::printf("\"%s\", %llu bytes\n", (const char *) filePath.u8string().c_str(), (uint64_t) FileSize);

    ExamineFile(filePath);

    (void) _dup2(StdOutFD, _fileno(stdout));

    _close(StdOutFD);
}

/// <summary>
/// Examines the specified file.
/// </summary>
static void ExamineFile(const fs::path & filePath)
{
    const std::string FileExtension = filePath.extension().string();

    try
    {
        if (::_stricmp(FileExtension.c_str(), ".dls") == 0)
            ProcessDLS(filePath);
        else
        if ((::_stricmp(FileExtension.c_str(), ".sbk") == 0) || (::_stricmp(FileExtension.c_str(), ".sf2") == 0) || (::_stricmp(FileExtension.c_str(), ".sf3") == 0))
            ProcessSF(filePath);
        else
        if (::_stricmp(FileExtension.c_str(), ".ecw") == 0)
            ProcessECW(filePath);
    }
    catch (const sf::exception & e)
    {
        ::printf("Failed to process soundfont: %s\n\n", e.what());
    }
    catch (const riff::exception & e)
    {
        ::printf("Failed to process RIFF file: %s\n\n", e.what());
    }

    ::fflush(stdout);
}

/// <summary>
/// Processes an SF bank.
/// </summary>
static void ProcessSF(const fs::path & filePath)
{
    sf::bank_t Bank;

    riff::memory_stream_t ms;

    if (ms.Open(filePath, 0, 0))
    {
        sf::reader_t sr;

        if (sr.Open(&ms, riff::reader_t::option_t::None))
            sr.Process(Bank, { true });

        ms.Close();
    }

    ::printf("%*sSoundFont specification version: v%d.%02d\n", __TRACE_LEVEL * 2, "", Bank.Major, Bank.Minor);
    ::printf("%*sSound Engine: \"%s\"\n", __TRACE_LEVEL * 2, "", Bank.SoundEngine.c_str());
    ::printf("%*sBank Name: \"%s\"\n", __TRACE_LEVEL * 2, "", Bank.Name.c_str());

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
    ::printf("%*sSample Data LSB: %zu bytes\n", __TRACE_LEVEL * 2, "", Bank.SampleDataLSB.size());

    if (Arguments.IsSet("presets"))
        DumpPresets(Bank);

    if (Arguments.IsSet("presetzones"))
        DumpPresetZones(Bank);

    // Dump the preset zone modulators.
    if (Arguments.IsSet("presetzonemodulators"))
    {
        ::printf("%*sPreset Zone Modulators (%zu)\n", __TRACE_LEVEL * 4, "", Bank.PresetModulators.size());

        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & Modulator : Bank.PresetModulators)
        {
            ::printf("%*s%5zu. Src Op: 0x%04X (%s), Dst Op: 0x%04X (%s), Amount: %6d, Amount Src Op: 0x%04X (%s), Transform Op: 0x%04X (%s)\n", __TRACE_LEVEL * 4, "", i++,
                Modulator.SrcOper,         Bank.DescribeModulatorSource(Modulator.SrcOper).c_str(),
                Modulator.DstOper,         Bank.DescribeGenerator(Modulator.DstOper, Modulator.Amount).c_str(),
                Modulator.Amount,
                Modulator.SrcOperAmt,      Bank.DescribeModulatorSource(Modulator.SrcOperAmt).c_str(),
                Modulator.TransformOper,   Bank.DescribeModulatorTransform(Modulator.TransformOper).c_str());
        }

        __TRACE_LEVEL--;
    }

    // Dump the preset zone generators.
    if (Arguments.IsSet("presetzonegenerators"))
    {
        ::printf("%*sPreset Zone Generators (%zu)\n", __TRACE_LEVEL * 4, "", Bank.PresetGenerators.size());

        __TRACE_LEVEL++;

        bool StartOfList = true;

        size_t i = 0;

        for (const auto & Generator : Bank.PresetGenerators)
        {
            ::printf("%*sZone %5zu. Operator: 0x%04X, Amount: 0x%04X, \"%s\"\n", __TRACE_LEVEL * 4, "", i++,
                Generator.Operator, Generator.Amount, Bank.DescribeGenerator(Generator.Operator, Generator.Amount).c_str());

            // 8.1.2 Should only appear in the PGEN sub-chunk, and it must appear as the last generator enumerator in all but the global preset zone.
            if (Generator.Operator == GeneratorOperator::instrument) // 8.1.2 Should only appear in the PGEN sub-chunk, and it must appear as the last generator enumerator in all but the global preset zone.
            {
                ::putchar('\n');

                StartOfList = true;
            }
        }

        __TRACE_LEVEL--;
    }

    // Dump the instruments.
    if (Arguments.IsSet("instruments"))
        DumpInstruments(Bank);

    // Dump the instrument zones.
    if (Arguments.IsSet("instrumentzones"))
    {
        ::printf("%*sInstrument Zones (%zu)\n", __TRACE_LEVEL * 4, "", Bank.InstrumentZones.size());

        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & iz : Bank.InstrumentZones)
        {
            ::printf("%*sZone %5zu. Generator %5d, Modulator %5d\n", __TRACE_LEVEL * 4, "", i++, iz.GeneratorIndex, iz.ModulatorIndex);
        }

        __TRACE_LEVEL--;
    }

    // Dump the instrument zone modulators.
    if (Arguments.IsSet("instrumentzonemodulators"))
    {
        ::printf("%*sInstrument Zone Modulators (%zu)\n", __TRACE_LEVEL * 4, "", Bank.InstrumentModulators.size());

        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & Modulator : Bank.InstrumentModulators)
        {
            ::printf("%*s%5zu. Src Op: 0x%04X, Dst Op: 0x%04X, Amount: %6d, Amount Src Op: 0x%04X, Transform Op: 0x%04X\n", __TRACE_LEVEL * 4, "", i++,
                Modulator.SrcOper,
                Modulator.DstOper,
                Modulator.Amount,
                Modulator.SrcOperAmt,
                Modulator.TransformOper);
        }

        __TRACE_LEVEL--;
    }

    // Dump the instrument zone generators.
    if (Arguments.IsSet("instrumentzonegenerators"))
    {
        ::printf("%*sInstrument Zone Generators (%zu)\n", __TRACE_LEVEL * 4, "", Bank.InstrumentGenerators.size());

        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & Generator : Bank.InstrumentGenerators)
        {
            ::printf("%*s%5zu. Operator: 0x%04X, Amount: 0x%04X, \"%s\"\n", __TRACE_LEVEL * 4, "", i++,
                Generator.Operator, Generator.Amount, Bank.DescribeGenerator(Generator.Operator, Generator.Amount).c_str());
        }

        __TRACE_LEVEL--;
    }

    // Dump the sample names (SoundFont v1.0 only).
    if (Arguments.IsSet("samplenames"))
    {
        if ((Bank.Major == 1) && (Bank.SampleNames.size() != 0))
        {
            ::printf("%*sSample Names (%zu)\n", __TRACE_LEVEL * 4, "", Bank.SampleNames.size() - 1);
            __TRACE_LEVEL++;

            size_t i = 0;

            for (const auto & SampleName : Bank.SampleNames)
            {
                ::printf("%*s%5zu. \"%-20s\"\n", __TRACE_LEVEL * 4, "", i++, SampleName.c_str());

                if (i == Bank.Samples.size() - 1)
                    break;
            }

            __TRACE_LEVEL--;
        }
    }

    // Dump the samples.
    if (Arguments.IsSet("samples"))
        DumpSamples(Bank);
/*
    std::filesystem::path FilePath(filePath);

    FilePath.replace_extension(L".new.sf2");

    riff::file_stream_t fs;

    ::printf("\n\"%s\"\n", riff::WideToUTF8(FilePath).c_str());

    // Tests the Sound Font writer.
    if (fs.Open(FilePath, true))
    {
        sf::writer_t sw;

        if (sw.Open(&fs, riff::writer_t::option_t::None))
            sw.Process({ }, Bank);

        fs.Close();
    }

    // Reload the written Sound Font to test if we can read our own output.
    if (fs.Open(FilePath))
    {
        sf::reader_t sr;

        if (sr.Open(&fs, riff::reader_t::option_t::None))
            sr.Process({ false }, Bank);

        fs.Close();
    }
*/
}

/// <summary>
/// Processes a DLS collection.
/// </summary>
static void ProcessDLS(const fs::path & filePath)
{
    sf::dls::collection_t dls;

    riff::file_stream_t fs;

    if (fs.Open(filePath))
    {
        sf::dls::reader_t dr;

        if (dr.Open(&fs, riff::reader_t::option_t::None))
        {
            try
            {
                dr.Process(dls, sf::dls::reader_options_t(true));
            }
            catch (const std::exception & e)
            {
                ::printf("Failed to process \"%s\": %s\n", filePath.string().c_str(), e.what());

                return;
            }
        }

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
            ::printf("%*sMIDI Key: %3d - %3d, Velocity: %3d - %3d, Options: 0x%04X, Key Group: %d, Zone: %d\n", __TRACE_LEVEL * 2, "",
                Region.LowKey, Region.HighKey, Region.LowVelocity, Region.HighVelocity, Region.Options, Region.KeyGroup, Region.Layer);

            if (!Region.Articulators.empty())
            {
                __TRACE_LEVEL += 2;

                ::printf("%*sArticulators:\n", __TRACE_LEVEL * 2, "");

                __TRACE_LEVEL++;

                for (const auto & Articulator : Region.Articulators)
                {
                    ::printf("%*s%3zu connection blocks\n", __TRACE_LEVEL * 2, "", Articulator.ConnectionBlocks.size());

                    __TRACE_LEVEL++;

                    for (const auto & ConnectionBlock: Articulator.ConnectionBlocks)
                    {
                        ::printf("%*sSource: 0x%04X, Control: 0x%04X, Destination: 0x%04X, Transform: 0x%04X, Scale: 0x%08X\n", __TRACE_LEVEL * 2, "",
                            ConnectionBlock.Source, ConnectionBlock.Control, ConnectionBlock.Destination, ConnectionBlock.Transform, ConnectionBlock.Scale);
                    }

                    __TRACE_LEVEL--;
                }

                __TRACE_LEVEL--;
                __TRACE_LEVEL -= 2;
            }
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
                ::printf("%*sSource: 0x%04X, Control: 0x%04X, Destination: 0x%04X, Transform: 0x%04X, Scale: 0x%08X\n", __TRACE_LEVEL * 2, "",
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

    sf::bank_t Bank;

    try
    {
        Bank.ConvertFrom(dls);

        fs::path FilePath(filePath);
    }
    catch (const sf::exception & e)
    {
        ::printf("Failed to convert DLS to SF2: %s\n\n", e.what());

        return;
    } 

    FilePath.replace_extension(L".sf2");

    ::printf("\n\"%s\"\n", (const char *) FilePath.u8string().c_str());

    try
    {
        // Tests the Sound Font writer.
        if (fs.Open(FilePath, true))
        {
            sf::writer_t sw;

            if (sw.Open(&fs, riff::writer_t::Options::PolyphoneCompatible))
                sw.Process(Bank);

            fs.Close();
        }

        ProcessSF(FilePath);
    }
    catch (const sf::exception & e)
    {
        ::printf("Failed to write converted SF2: %s\n\n", e.what());

        return;
    } 
}

/// <summary>
/// Processes an ECW wave set.
/// </summary>
static void ProcessECW(const fs::path & filePath)
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

    ::printf("%*sName: \"%s\"\n", __TRACE_LEVEL * 2, "", riff::CodePageToUTF8(850, ws.Name.c_str(), ws.Name.length()).c_str());
    ::printf("%*sCopyright: \"%s\"\n", __TRACE_LEVEL * 2, "", riff::CodePageToUTF8(850, ws.Copyright.c_str(), ws.Copyright.length()).c_str());
    ::printf("%*sDescription: \"%s\"\n", __TRACE_LEVEL * 2, "", riff::CodePageToUTF8(850, ws.Description.c_str(), ws.Description.length()).c_str());
    ::printf("%*sInformation: \"%s\"\n", __TRACE_LEVEL * 2, "", riff::CodePageToUTF8(850, ws.Information.c_str(), ws.Information.length()).c_str());
    ::printf("%*sFile Name: \"%s\"\n", __TRACE_LEVEL * 2, "", riff::CodePageToUTF8(850, ws.FileName.c_str(), ws.FileName.length()).c_str());

    // Dump the bank maps.
    {
        ::printf("\n%*sBank Maps\n", __TRACE_LEVEL * 2, "");
        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & BankMap : ws.BankMaps)
        {
            ::printf("%*sBank Map %zu\n", __TRACE_LEVEL * 2, "", i++);
            __TRACE_LEVEL++;

            size_t j = 0;

            for (const auto & MIDIPatchMap : BankMap.MIDIPatchMaps)
            {
                ::printf("%*s%5zu. Patch Map %5d\n", __TRACE_LEVEL * 2, "", j++, MIDIPatchMap);
            }

            __TRACE_LEVEL--;
        }

        __TRACE_LEVEL--;
    }

    // Dump the drum kit maps.
    {
        ::printf("\n%*sDrum Kit Maps\n", __TRACE_LEVEL * 2, "");
        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & DrumKitMap : ws.DrumKitMaps)
        {
            ::printf("%*sDrum Kit Map %zu\n", __TRACE_LEVEL * 2, "", i++);
            __TRACE_LEVEL++;

            size_t j = 0;

            for (const auto & DrumNoteMap : DrumKitMap.DrumNoteMaps)
            {
                ::printf("%*s%5zu. Drum Note Map %5d\n", __TRACE_LEVEL * 2, "", j++, DrumNoteMap);
            }

            __TRACE_LEVEL--;
        }

        __TRACE_LEVEL--;
    }

    // Dump the MIDI Patch maps.
    {
        ::printf("\n%*sMIDI Patch Maps\n", __TRACE_LEVEL * 2, "");
        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & MIDIPatchMap : ws.MIDIPatchMaps)
        {
            ::printf("%*sMIDI Patch Map %zu\n", __TRACE_LEVEL * 2, "", i++);
            __TRACE_LEVEL++;

            size_t j = 0;

            for (const auto & Instrument : MIDIPatchMap.Instruments)
            {
                ::printf("%*sMIDI Program %3zu = ECW Instrument %5d\n", __TRACE_LEVEL * 2, "", j++, Instrument);
            }

            __TRACE_LEVEL--;
        }

        __TRACE_LEVEL--;
    }

    // Dump the MIDI Drum Note maps.
    {
        ::printf("\n%*sDrum Note Maps\n", __TRACE_LEVEL * 2, "");
        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & DrumNoteMap : ws.DrumNoteMaps)
        {
            ::printf("%*sDrum Note Map %zu\n", __TRACE_LEVEL * 2, "", i++);
            __TRACE_LEVEL++;

            size_t j = 0;

            for (const auto & Instrument : DrumNoteMap.Instruments)
            {
                ::printf("%*sMIDI Program %3zu = ECW Instrument %5d\n", __TRACE_LEVEL * 2, "", j++, Instrument);
            }

            __TRACE_LEVEL--;
        }

        __TRACE_LEVEL--;
    }

    // Dump the instruments.
    {
        ::printf("\n%*sInstruments\n", __TRACE_LEVEL * 2, "");
        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & InstrumentHeader : ws.Instruments)
        {
            if (InstrumentHeader.Type == 2)
            {
                const auto & ih = (ecw::instrument_v1_t &) InstrumentHeader;

                ::printf("%*s%5zu. v1, Sub Type: %d, Note: %3d\n", __TRACE_LEVEL * 2, "", i++, ih.SubType, ih.NoteThreshold);

                __TRACE_LEVEL++;

                if (ih.SubType < 3)
                    ::printf("%*s       Header 0, Patch: %5d, Amplitude: %4d, Pan: %4d, Coarse Tune: %4d, Fine Tune: %4d, Delay: %5d, Group: %3d\n", __TRACE_LEVEL * 2, "",
                        ih.SubHeaders[0].PatchIndex,
                        ih.SubHeaders[0].Amplitude,
                        ih.SubHeaders[0].Pan,
                        ih.SubHeaders[0].CoarseTune,
                        ih.SubHeaders[0].FineTune,
                        ih.SubHeaders[0].Delay,
                        ih.SubHeaders[0].Group
                    );

                if (ih.SubType > 0)
                    ::printf("%*s       Header 1, Patch: %5d, Amplitude: %4d, Pan: %4d, Coarse Tune: %4d, Fine Tune: %4d, Delay: %5d, Group: %3d\n", __TRACE_LEVEL * 2, "",
                        ih.SubHeaders[1].PatchIndex,
                        ih.SubHeaders[1].Amplitude,
                        ih.SubHeaders[1].Pan,
                        ih.SubHeaders[1].CoarseTune,
                        ih.SubHeaders[1].FineTune,
                        ih.SubHeaders[1].Delay,
                        ih.SubHeaders[1].Group
                    );

                __TRACE_LEVEL--;
            }
            else
            if (InstrumentHeader.Type == 255)
            {
                const auto & ih = (ecw::instrument_v2_t &) InstrumentHeader;

                ::printf("%*s%5zu. v2\n", __TRACE_LEVEL * 2, "", i++);

                for (const auto & sh : ih.SubHeaders)
                    ::printf("%*s       Instrument: %5d, Note: %3d\n", __TRACE_LEVEL * 2, "", sh.InstrumentIndex, sh.NoteThreshold);
            }
            else
                ::printf("%*s%5zu. Unknown instrument type\n", __TRACE_LEVEL * 2, "", i++);
        }

        __TRACE_LEVEL--;
    }

    // Dump the patches.
    {
        ::printf("\n%*sPatches\n", __TRACE_LEVEL * 2, "");
        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & ph : ws.Patches)
        {
            ::printf("%*s%5zu. Pitch Env: %4d, Modulation: %4d, Scale: %4d, Array 1 Index: %5d, Detune: %4d\n", __TRACE_LEVEL * 2, "", i++,
                ph.PitchEnvelopeLevel,
                ph.ModulationSensitivity,
                ph.Scale,
                ph.Array1Index,
                ph.Detune
            );
        }

        __TRACE_LEVEL--;
    }

    {
        ::printf("\n%*sArray 1 (%d)\n", __TRACE_LEVEL * 2, "", (int) ws.Array1.size());
        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & Item : ws.Array1)
        {
            if (Item.Index != 0xFFFF)
                ::printf("%*s%5zu. Slot: %5d, Name: \"%s\"\n", __TRACE_LEVEL * 2, "", i, Item.Index, Item.Name.c_str());
            else
                ::printf("%*s%5zu. Unused\n", __TRACE_LEVEL * 2, "", i);

            ++i;
        }

        __TRACE_LEVEL--;
    }

    {
        ::printf("\n%*sArray 2 (%d)\n", __TRACE_LEVEL * 2, "", (int) ws.Array2.size());
        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & Item : ws.Array2)
        {
            if (Item.Index != 0)
                ::printf("%*s%5zu. Slot: %5d, Name: \"%s\"\n", __TRACE_LEVEL * 2, "", i, Item.Index, Item.Name.c_str());
            else
                ::printf("%*s%5zu. Unused\n", __TRACE_LEVEL * 2, "", i);

            ++i;
        }

        __TRACE_LEVEL--;
    }

    {
        ::printf("\n%*sArray 3 (%d)\n", __TRACE_LEVEL * 2, "", (int) ws.Array3.size());
        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & Item : ws.Array3)
        {
            if (Item.Index != 0)
                ::printf("%*s%5zu. Slot: %5d, Name: \"%s\"\n", __TRACE_LEVEL * 2, "", i, Item.Index, Item.Name.c_str());
            else
                ::printf("%*s%5zu. Unused\n", __TRACE_LEVEL * 2, "", i);

            ++i;
        }

        __TRACE_LEVEL--;
    }

    {
        ::printf("\n%*sSamples\n", __TRACE_LEVEL * 2, "");
        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & s : ws.Samples)
        {
            ::printf("%*s%5zu. \"%-14s\", MIDI Key: %3d-%3d, Flags: 0x%02X, Fine Tune: %4d, Coarse Tune: %4d, Offset: %9d, Loop: %9d-%9d\n", __TRACE_LEVEL * 2, "", i++,
                s.Name.c_str(),
                s.LowKey, s.HighKey,
                s.Flags,
                s.FineTune,
                s.CoarseTune,
                s.SampleStart,
                s.LoopStart,
                s.LoopEnd
            );
        }

        __TRACE_LEVEL--;
    }
#endif

    // Test the waveset to SoundFont bank conversion.
    {
        sf::bank_t Bank;

//      ConvertECW(ws, Bank);

        {
            std::filesystem::path FilePath(filePath);

            FilePath.replace_extension(L".sf2");

            riff::file_stream_t fs;

            ::printf("\n\"%s\"\n", FilePath.string().c_str());

            {
                if (fs.Open(FilePath, true))
                {
                    sf::writer_t sw;

                    if (sw.Open(&fs, riff::writer_t::Options::PolyphoneCompatible))
                        sw.Process(Bank);

                    fs.Close();
                }
            }

            ProcessSF(FilePath);
        }
    }
}

/// <summary>
/// Converts an ECW waveset to a SoundFont bank.
/// </summary>
static void ConvertECW(const ecw::waveset_t & ws, sf::bank_t & bank)
{
    bank.Major = 2;
    bank.Minor = 4;
    bank.SoundEngine = "E-mu 10K1";
    bank.Name = ws.Name;

    {
         SYSTEMTIME st = {};

        ::GetLocalTime(&st);

        char Date[32] = { };
        char Time[32] = { };

        ::GetDateFormatA(LOCALE_USER_DEFAULT, 0, &st, "yyyy-MM-dd", Date, sizeof(Date));
        ::GetTimeFormatA(LOCALE_USER_DEFAULT, 0, &st, "HH:mm:ss", Time, sizeof(Time));

        bank.Properties.push_back(sf::property_t(FOURCC_ICRD, std::string(Date) + " " + std::string(Time)));
    }

    if (!ws.Information.empty())
        bank.Properties.push_back(sf::property_t(FOURCC_ICMT, ws.Information));

    if (!ws.Copyright.empty())
        bank.Properties.push_back(sf::property_t(FOURCC_ICOP, ws.Copyright));

    if (!ws.Description.empty())
        bank.Properties.push_back(sf::property_t(FOURCC_ISBJ, ws.Description));

    bank.Properties.push_back(sf::property_t(FOURCC_ISFT, "sfdump"));

    bank.SampleData = ws.SampleData;

    // Initialize Hydra.
    {
        // Add the samples.
        {
            for (const auto & s : ws.Samples)
            {
                uint8_t Pitch = 127 + s.CoarseTune;

                if (s.FineTune < 0)
                    Pitch--;

                int8_t PitchCorrection = 0;

                bank.Samples.push_back(sf::sample_t
                (
                    s.Name,
                    s.SampleStart / sizeof(int16_t),
                    s.LoopEnd     / sizeof(int16_t),
                    s.LoopStart   / sizeof(int16_t),
                    s.LoopEnd     / sizeof(int16_t),
                    22050,
                    Pitch,
                    PitchCorrection,
                    0,
                    sf::SampleTypes::MonoSample
                ));
            }

            bank.Samples.push_back(sf::sample_t("EOS"));
        }

        // Add the instruments.
        {
            for (const auto & Slot : ws.Array3)
            {
                if (Slot.Index == 0)
                    continue;

                uint16_t i = Slot.Index;

                bank.Instruments.push_back(sf::instrument_t(Slot.Name, (uint16_t) bank.InstrumentZones.size()));

                for (auto it = std::next(ws.Samples.begin(), i); (it != ws.Samples.end()); ++it)
                {
                    if (it->Name != Slot.Name)
                        break;

                    bank.InstrumentZones.push_back(sf::instrument_zone_t((uint16_t) bank.InstrumentGenerators.size(), (uint16_t) bank.InstrumentModulators.size()));

                    bank.InstrumentGenerators.push_back(sf::generator_t(GeneratorOperator::keyRange, MAKEWORD(it->LowKey, it->HighKey)));
                    bank.InstrumentGenerators.push_back(sf::generator_t(GeneratorOperator::sampleID, i));

                    ++i;
                }
            }

            bank.Instruments.push_back(sf::instrument_t("EOI"));

            // Add the instrument zone modulators.
            bank.InstrumentModulators.push_back(sf::modulator_t());
        }
/*
        // Add the presets.
        {
            constexpr const uint16_t Banks[] = { 0, 127 };

            for (const uint16_t & Bank : Banks)
            {
                const uint16_t Index = ws.BankMaps[0].MIDIPatchMaps[Bank];

                const auto & mpm = ws.MIDIPatchMaps[Index];

                for (const auto & Instrument : mpm.Instruments)
                {
                    bank.Presets.push_back(sf::preset_t(bank.Instruments[Instrument].Name, Instrument, Bank, (uint16_t) bank.PresetZones.size(), 0, 0, 0));

                    bank.PresetZones.push_back(sf::preset_zone_t((uint16_t) bank.PresetZoneGenerators.size(), (uint16_t) bank.PresetZoneModulators.size()));

                    bank.PresetZoneGenerators.push_back(sf::preset_zone_generator_t(41, Instrument)); // Generator "instrument"
                }
            }

            bank.Presets.push_back(sf::preset_t("EOP"));

            // Add the preset zone modulators.
            bank.PresetZoneModulators.push_back(sf::preset_zone_modulator_t(0, 0, 0, 0, 0));
        }
*/
    }
}

/// <summary>
/// Dumps the presets.
/// </summary>
static void DumpPresets(const bank_t & bank) noexcept
{
    if (bank.Presets.size() == 0)
        return;

    ::printf("%*sPresets (%zu)\n", __TRACE_LEVEL * 4, "", bank.Presets.size() - 1);

    __TRACE_LEVEL++;

    size_t i = 0;

    for (auto Preset = bank.Presets.begin(); Preset < bank.Presets.end() - 1; ++Preset)
    {
        ::printf("%*s%5zu. \"%s\", Bank %d, Program %d, Zone %d\n", __TRACE_LEVEL * 4, "", i++, Preset->Name.c_str(), Preset->MIDIBank, Preset->MIDIProgram, Preset->ZoneIndex);

        DumpPresetZoneList(bank, Preset->ZoneIndex, (Preset + 1)->ZoneIndex);
    }

    __TRACE_LEVEL--;
}

/// <summary>
/// Dumps a preset zone list.
/// </summary>
static void DumpPresetZoneList(const bank_t & bank, size_t fromIndex, size_t toIndex) noexcept
{
    __TRACE_LEVEL++;

    for (size_t Index = fromIndex; Index < toIndex; ++Index)
    {
        const auto & z1 = bank.PresetZones[Index];
        const auto & z2 = bank.PresetZones[Index + 1];

        // 7.3 A global zone is determined by the fact that the last generator in the list is not an instrument generator.
        bool IsGlobalZone = (fromIndex != toIndex) &&
            ((z1.GeneratorIndex == z2.GeneratorIndex) ||
             (z1.GeneratorIndex < z2.GeneratorIndex) && (bank.PresetGenerators[(size_t) z2.GeneratorIndex - 1].Operator != GeneratorOperator::instrument));

        ::printf("%*sZone %5zu. Generator: %d, Modulator: %d%s\n", __TRACE_LEVEL * 4, "", Index,
            z1.GeneratorIndex, z1.ModulatorIndex,
            (IsGlobalZone ? " (Global zone)" : ""));

        DumpPresetZoneGenerators(bank, z1.GeneratorIndex, z2.GeneratorIndex);
        DumpPresetZoneModulators(bank, z1.ModulatorIndex, z2.ModulatorIndex);
    }

    __TRACE_LEVEL--;
}

/// <summary>
/// Dumps a preset zone generator list.
/// </summary>
static void DumpPresetZoneGenerators(const bank_t & bank, size_t fromIndex, size_t toIndex) noexcept
{
    __TRACE_LEVEL++;

    uint16_t OldOperator = GeneratorOperator::Invalid;

    auto Generator = bank.PresetGenerators.begin() + fromIndex;

    for (size_t Index = fromIndex; Index < toIndex; ++Generator, ++Index)
    {
        ::printf("%*s%5zu. Operator: 0x%04X, Amount: 0x%04X, \"%s\"", __TRACE_LEVEL * 4, "", Index,
            Generator->Operator, (uint16_t) Generator->Amount, bank.DescribeGenerator(Generator->Operator, Generator->Amount).c_str());

        if (Generator->Operator == GeneratorOperator::keyRange)
        {
            if (Index != fromIndex)
                ::printf(" Warning: keyRange must be the first generator in the zone generator list.\n"); // 8.1.2
            else
                ::putchar('\n');
        }
        else
        if (Generator->Operator == GeneratorOperator::velRange)
        {
            if ((Index != fromIndex) && (OldOperator != GeneratorOperator::keyRange))
                ::printf(" Warning: velRange must be only preceded by keyRange.\n"); // 8.1.2
            else
                ::putchar('\n');
        }
        else
        if (Generator->Operator == GeneratorOperator::instrument)
        {
            if (Index != toIndex - 1)
                ::printf(" Warning: instrument must be the last generator.\n"); // 8.1.2 Should only appear in the PGEN sub-chunk, and it must appear as the last generator enumerator in all but the global preset zone.
            else
                ::putchar('\n');
        }
        else
            ::putchar('\n'); // 8.1.2 

        OldOperator = Generator->Operator;
    }

    __TRACE_LEVEL--;
}

/// <summary>
/// Dumps a preset zone modulator list.
/// </summary>
static void DumpPresetZoneModulators(const bank_t & bank, size_t fromIndex, size_t toIndex) noexcept
{
    __TRACE_LEVEL++;

    auto Modulator = bank.PresetModulators.begin() + fromIndex;

    for (size_t Index = fromIndex; Index < toIndex; ++Modulator, ++Index)
    {
        ::printf("%*s%5zu. Src Op: 0x%04X (%s), Dst Op: 0x%04X (%s), Amount: %6d, Src Op Amount: 0x%04X (%s), Transform Op: 0x%04X (%s)\n", __TRACE_LEVEL * 4, "", Index,
            Modulator->SrcOper,        bank.DescribeModulatorSource(Modulator->SrcOper).c_str(),
            Modulator->DstOper,        bank.DescribeGenerator(Modulator->DstOper, Modulator->Amount).c_str(),
            Modulator->Amount,
            Modulator->SrcOperAmt,     bank.DescribeModulatorSource(Modulator->SrcOperAmt).c_str(),
            Modulator->TransformOper,  bank.DescribeModulatorTransform(Modulator->TransformOper).c_str()
        );
    }

    __TRACE_LEVEL--;
}

/// <summary>
/// Dumps the instruments.
/// </summary>
static void DumpInstruments(const bank_t & bank) noexcept
{
    if (bank.Instruments.size() == 0)
        return;

    ::printf("%*sInstruments (%zu)\n", __TRACE_LEVEL * 4, "", bank.Instruments.size() - 1);

    __TRACE_LEVEL++;

    size_t i = 0;

    for (auto Instrument = bank.Instruments.begin(); Instrument < bank.Instruments.end() - 1; ++Instrument)
    {
        ::printf("%*s%5zu. \"%s\", Instrument Zone %d\n", __TRACE_LEVEL * 4, "", i++, Instrument->Name.c_str(), Instrument->ZoneIndex);

        DumpInstrumentZoneList(bank, Instrument->ZoneIndex, (Instrument + 1)->ZoneIndex);
    }

    __TRACE_LEVEL--;
}

/// <summary>
/// Dumps an instrument zone list.
/// </summary>
static void DumpInstrumentZoneList(const bank_t & bank, size_t fromIndex, size_t toIndex) noexcept
{
    __TRACE_LEVEL++;

    for (size_t Index = fromIndex; Index < toIndex; ++Index)
    {
        const auto & z1 = bank.InstrumentZones[Index];
        const auto & z2 = bank.InstrumentZones[Index + 1];

        // 7.7 A global zone is determined by the fact that the last generator in the list is not a sampleID generator.
        bool IsGlobalZone = (fromIndex != toIndex) &&
            ((z1.GeneratorIndex == z2.GeneratorIndex) ||
             (z1.GeneratorIndex < z2.GeneratorIndex) && (bank.InstrumentGenerators[(size_t) z2.GeneratorIndex - 1].Operator != GeneratorOperator::sampleID));

        ::printf("%*s%5zu. Generator: %d, Modulator: %d%s\n", __TRACE_LEVEL * 4, "", Index,
            z1.GeneratorIndex, z1.ModulatorIndex,
            (IsGlobalZone ? " (Global zone)" : ""));

        DumpInstrumentZoneGenerators(bank, z1.GeneratorIndex, z2.GeneratorIndex);
        DumpInstrumentZoneModulators(bank, z1.ModulatorIndex, z2.ModulatorIndex);
    }

    __TRACE_LEVEL--;
}

/// <summary>
/// Dumps an instrument zone generator list.
/// </summary>
static void DumpInstrumentZoneGenerators(const bank_t & bank, size_t fromIndex, size_t toIndex) noexcept
{
    __TRACE_LEVEL++;

    uint16_t OldOperator = GeneratorOperator::Invalid;

    auto Generator = bank.InstrumentGenerators.begin() + fromIndex;

    for (size_t Index = fromIndex; Index < toIndex; ++Generator, ++Index)
    {
        ::printf("%*s%5zu. Operator: 0x%04X, Amount: 0x%04X, \"%s\"", __TRACE_LEVEL * 4, "", Index,
            Generator->Operator, (uint16_t) Generator->Amount, bank.DescribeGenerator(Generator->Operator, Generator->Amount).c_str());

        if (Generator->Operator == GeneratorOperator::keyRange)
        {
            if (Index != fromIndex)
                ::printf(" Warning: keyRange must be the first generator in the zone generator list.\n"); // 8.1.2
            else
                ::putchar('\n');
        }
        else
        if (Generator->Operator == GeneratorOperator::velRange)
        {
            if ((Index != fromIndex) && (OldOperator != GeneratorOperator::keyRange))
                ::printf(" Warning: velRange must be only preceded by keyRange.\n"); // 8.1.2
            else
                ::putchar('\n');
        }
        else
        if (Generator->Operator == GeneratorOperator::sampleID)
        {
            if (Index != toIndex - 1)
                ::printf(" Warning: sampleID must be the last generator.\n"); // 8.1.2 Should appear only in the IGEN sub-chunk and must appear as the last generator enumerator in all but the global zone.
            else
                ::putchar('\n');
        }
        else
            ::putchar('\n'); // 8.1.2 

        OldOperator = Generator->Operator;
    }

    __TRACE_LEVEL--;
}

/// <summary>
/// Dumps an instrument zone modulator list.
/// </summary>
static void DumpInstrumentZoneModulators(const bank_t & bank, size_t fromIndex, size_t toIndex) noexcept
{
    __TRACE_LEVEL++;

    auto Modulator = bank.InstrumentModulators.begin() + fromIndex;

    for (size_t Index = fromIndex; Index < toIndex; ++Modulator, ++Index)
    {
        ::printf("%*s%5zu. Src Op: 0x%04X (%s), Dst Op: 0x%04X (%s), Amount: %6d, Src Op Amount: 0x%04X (%s), Transform Op: 0x%04X (%s)\n", __TRACE_LEVEL * 4, "", Index,
            Modulator->SrcOper,        bank.DescribeModulatorSource(Modulator->SrcOper).c_str(),
            Modulator->DstOper,        bank.DescribeGenerator(Modulator->DstOper, Modulator->Amount).c_str(),
            Modulator->Amount,
            Modulator->SrcOperAmt,     bank.DescribeModulatorSource(Modulator->SrcOperAmt).c_str(),
            Modulator->TransformOper,  bank.DescribeModulatorTransform(Modulator->TransformOper).c_str()
        );
    }

    __TRACE_LEVEL--;
}

/// <summary>
/// Dumps the preset zones.
/// </summary>
static void DumpPresetZones(const bank_t & bank) noexcept
{
    ::printf("%*sPreset Zones (%zu)\n", __TRACE_LEVEL * 2, "", bank.PresetZones.size());

    __TRACE_LEVEL++;

    size_t i = 0;

    for (const auto & pz : bank.PresetZones)
    {
        ::printf("%*s%5zu. Generator: %5d, Modulator: %5d\n", __TRACE_LEVEL * 2, "", i++, pz.GeneratorIndex, pz.ModulatorIndex);
    }

    __TRACE_LEVEL--;
}

/// <summary>
/// Dumps the samples.
/// </summary>
static void DumpSamples(const bank_t & bank) noexcept
{
    if (bank.Samples.size() == 0)
        return;

    ::printf("%*sSamples (%zu)\n", __TRACE_LEVEL * 4, "", bank.Samples.size() - 1);

    __TRACE_LEVEL++;

    size_t i = 0;

    for (const auto & Sample : bank.Samples)
    {
        ::printf("%*s%5zu. \"%-20s\", %9d-%9d, Loop: %9d-%9d, %6d Hz, Pitch (MIDI Key): %3d, Pitch Correction: %3d, Linked Sample: %5d, Type: 0x%04X \"%s\"", __TRACE_LEVEL * 4, "", i++,
            Sample.Name.c_str(), Sample.Start, Sample.End, Sample.LoopStart, Sample.LoopEnd,
            Sample.SampleRate, Sample.Pitch, Sample.PitchCorrection,
            Sample.SampleLink, Sample.SampleType, bank.DescribeSampleType(Sample.SampleType).c_str());

        if ((Sample.End - Sample.Start) < 48)
            ::printf(" Warning: Sample should have at least 48 data points.\n");
        else
        if (Sample.LoopStart != Sample.LoopEnd)
        {
            if (Sample.Start >= (Sample.LoopStart - 7))
                ::printf(" Warning: Sample start should be at least 8 data points before sample loop start.\n");
            else
            if (Sample.End <= (Sample.LoopEnd - 7))
                ::printf(" Warning: Sample end should be at least 8 data points after sample loop end.\n");
            else
            if ((Sample.LoopEnd - Sample.LoopStart) < 32)
                ::printf(" Warning: Sample loop should have at least 32 data points.\n");
            else
                ::putchar('\n');
        }
        else
            ::putchar('\n');

        if (i == bank.Samples.size() - 1)
            break;
    }

    __TRACE_LEVEL--;
}


/// <summary>
/// Describes a SoundFont Generator (8.1).
/// </summary>
std::string bank_t::DescribeGenerator(uint16_t generator, uint16_t amount) const noexcept
{
    std::string Text;

    switch (generator & 0x007F)
    {
        // Index generators
        case GeneratorOperator::instrument:                 Text = riff::FormatText("Instrument Index %d, \"%s\" (instrument)", amount, Instruments[amount].Name.c_str()); break;
        case GeneratorOperator::sampleID:                   Text = riff::FormatText("Sample Index %d, \"%s\" (sampleID)", amount, Samples[amount].Name.c_str()); break;

        // Range generators
        case GeneratorOperator::keyRange:                   Text = riff::FormatText("Key Range %d - %d (keyRange)", amount & 0x00FF, (amount >> 8) & 0x00FF); break;
        case GeneratorOperator::velRange:                   Text = riff::FormatText("Velocity Range %d - %d (velRange)", amount & 0x00FF, (amount >> 8) & 0x00FF); break;

        // Substitution generators

        // Sample generators directly affect a sample's properties.
        case GeneratorOperator::startAddrsOffset:           Text = riff::FormatText("Start Address Offset: %d data points (startAddrsOffset)", (int16_t) amount); break;
        case GeneratorOperator::endAddrsOffset:             Text = riff::FormatText("End Address Offset: %d data points (endAddrsOffset)", (int16_t) amount); break;

        case GeneratorOperator::startAddrsCoarseOffset:     Text = riff::FormatText("Start Address Coarse Offset: %d data points (startAddrsCoarseOffset)", (int32_t) amount * 32768); break;
        case GeneratorOperator::endAddrsCoarseOffset:       Text = riff::FormatText("End Address Coarse Offset: %d data points (endAddrsCoarseOffset)", (int32_t) amount * 32768); break;

        case GeneratorOperator::startloopAddrsOffset:       Text = riff::FormatText("Start Loop Address Offset: %d data points (startloopAddrsOffset)", (int16_t) amount); break;
        case GeneratorOperator::endloopAddrsOffset:         Text = riff::FormatText("End Loop Address Offset: %d data points (endloopAddrsOffset)", (int16_t) amount); break;

        case GeneratorOperator::startloopAddrsCoarseOffset: Text = riff::FormatText("Start Loop Address Coarse Offset: %d data points (startloopAddrsCoarseOffset)", (int32_t) amount * 32768); break;
        case GeneratorOperator::endloopAddrsCoarseOffset:   Text = riff::FormatText("End Loop Address Coarse Offset: %d data points (endloopAddrsCoarseOffset)", (int32_t) amount * 32768); break;

        case GeneratorOperator::sampleModes:                Text = riff::FormatText("Sample Mode: %d (sampleModes)", amount); break;   // 0 = no loop, 1 = loop, 2 = reserved, 3 = loop and play till the end in release phase

        case GeneratorOperator::overridingRootKey:          Text = riff::FormatText("Overriding Root Key: %d (overridingRootKey)", amount); break;

        case GeneratorOperator::exclusiveClass:             Text = riff::FormatText("Exclusive Class: %s (exclusiveClass)", (amount != 0) ? "Yes" : "No"); break;

        // Value generators directly affects a signal processing parameter.
        case GeneratorOperator::initialFilterFc:            Text = riff::FormatText("Initial Lowpass Filter Cutoff Frequency: %d cents (initialFilterFc)", (int16_t) amount); break;
        case GeneratorOperator::initialFilterQ:             Text = riff::FormatText("Initial Lowpass Filter Resonance: %d centibels (initialFilterQ)", (int16_t) amount); break;
        case GeneratorOperator::initialAttenuation:         Text = riff::FormatText("Initial Attenuation: %.0f dB (initialAttenuation)", (int16_t) amount / 10.); break;

        case GeneratorOperator::modLfoToPitch:              Text = riff::FormatText("Modulation LFO influence on Pitch: %d cents (modLfoToPitch)", (int16_t) amount); break;
        case GeneratorOperator::modLfoToFilterFc:           Text = riff::FormatText("Modulation LFO influence on Filter Cutoff Frequency: %d cents (modLfoToFilterFc)", (int16_t) amount); break;
        case GeneratorOperator::modLfoToVolume:             Text = riff::FormatText("Modulation LFO influence on Volume: %d centibels (modLfoToVolume)", (int16_t) amount); break;

        case GeneratorOperator::modEnvToPitch:              Text = riff::FormatText("Modulation Envelope influence on Pitch: %d cents (modEnvToPitch)", (int16_t) amount); break;
        case GeneratorOperator::modEnvToFilterFc:           Text = riff::FormatText("Modulation Envelope influence on Filter Cutoff Frequency: %d cents (modEnvToFilterFc)", (int16_t) amount); break;

        case GeneratorOperator::vibLfoToPitch:              Text = riff::FormatText("Vibrato LFO influence on Pitch: %d cents (vibLfoToPitch)", (int16_t) amount); break;

        case GeneratorOperator::chorusEffectsSend:          Text = riff::FormatText("Chorus: %.1f%% (chorusEffectsSend)", (int16_t) amount / 10.); break;
        case GeneratorOperator::reverbEffectsSend:          Text = riff::FormatText("Reverb: %.1f%% (reverbEffectsSend)", (int16_t) amount / 10.); break;

        case GeneratorOperator::pan:                        Text = riff::FormatText("Pan: %.1f%% (pan)", (int16_t) amount / 10.f); break;

        case GeneratorOperator::delayModLFO:                Text = riff::FormatText("Modulation LFO Delay: %.0f ms (delayModLFO)", std::exp2((int16_t) amount / 1200.) * 1000.); break;
        case GeneratorOperator::freqModLFO:                 Text = riff::FormatText("Modulation LFO Frequency: %.0f mHz (freqModLFO)", std::exp2((int16_t) amount / 1200.) * 8176.); break;

        case GeneratorOperator::delayVibLFO:                Text = riff::FormatText("Vibrato LFO Delay: %.0f ms (delayVibLFO)", std::exp2((int16_t) amount / 1200.) * 1000.); break;
        case GeneratorOperator::freqVibLFO:                 Text = riff::FormatText("Vibrato LFO Frequency: %.0f mHz (freqVibLFO)", std::exp2((int16_t) amount / 1200.) * 8176.); break;

        case GeneratorOperator::delayModEnv:                Text = riff::FormatText("Modulation Envelope Delay: %.0f ms (delayModEnv)", std::exp2((int16_t) amount / 1200.) * 1000.); break;
        case GeneratorOperator::attackModEnv:               Text = riff::FormatText("Modulation Envelope Attack: %.0f ms (attackModEnv)", std::exp2((int16_t) amount / 1200.) * 1000.); break;
        case GeneratorOperator::holdModEnv:                 Text = riff::FormatText("Modulation Envelope Hold: %.0f ms (holdModEnv)", std::exp2((int16_t) amount / 1200.) * 1000.); break;
        case GeneratorOperator::decayModEnv:                Text = riff::FormatText("Modulation Envelope Decay: %.0f ms (decayModEnv)", std::exp2((int16_t) amount / 1200.) * 1000.); break;
        case GeneratorOperator::sustainModEnv:              Text = riff::FormatText("Modulation Envelope Sustain: %.0f dB (sustainModEnv)", (int16_t) amount / 10.); break;
        case GeneratorOperator::releaseModEnv:              Text = riff::FormatText("Modulation Envelope Release: %.0f ms (releaseModEnv)", std::exp2((int16_t) amount / 1200.) * 1000.); break;
        case GeneratorOperator::keynumToModEnvHold:         Text = riff::FormatText("Modulation Envelope Hold Decrease: %.0f ms (keynumToModEnvHold)", std::exp2((int16_t) amount / 1200.) * 1000.); break;
        case GeneratorOperator::keynumToModEnvDecay:        Text = riff::FormatText("Modulation Envelope Decay Decrease: %.0f ms (keynumToModEnvDecay)", std::exp2((int16_t) amount / 1200.) * 1000.); break;

        case GeneratorOperator::delayVolEnv:                Text = riff::FormatText("Volume Envelope Delay: %.0f ms (delayVolEnv)", std::exp2((int16_t) amount / 1200.) * 1000.); break;
        case GeneratorOperator::attackVolEnv:               Text = riff::FormatText("Volume Envelope Attack: %.0f ms (attackVolEnv)", std::exp2((int16_t) amount / 1200.) * 1000.); break;
        case GeneratorOperator::holdVolEnv:                 Text = riff::FormatText("Volume Envelope Hold: %.0f ms (holdVolEnv)", std::exp2((int16_t) amount / 1200.) * 1000.); break;
        case GeneratorOperator::decayVolEnv:                Text = riff::FormatText("Volume Envelope Decay: %.0f ms (decayVolEnv)", std::exp2((int16_t) amount / 1200.) * 1000.); break;
        case GeneratorOperator::sustainVolEnv:              Text = riff::FormatText("Volume Envelope Sustain: %.0f dB (sustainVolEnv)", (int16_t) amount / 10.); break;
        case GeneratorOperator::releaseVolEnv:              Text = riff::FormatText("Volume Envelope Release: %.0f ms (releaseVolEnv)", std::exp2((int16_t) amount / 1200.) * 1000.); break;
        case GeneratorOperator::keynumToVolEnvHold:         Text = riff::FormatText("Volume Envelope Hold Decrease: %.0f ms (keynumToVolEnvHold)", std::exp2((int16_t) amount / 1200.) * 1000.); break;
        case GeneratorOperator::keynumToVolEnvDecay:        Text = riff::FormatText("Volume Envelope Decay Decrease: %.0f ms (keynumToVolEnvDecay)", std::exp2((int16_t) amount / 1200.) * 1000.); break;

        case GeneratorOperator::keyNum:                     Text = riff::FormatText("MIDI Key: %d (keynum)", amount); break;
        case GeneratorOperator::velocity:                   Text = riff::FormatText("MIDI Velocity: %d (velocity)", amount); break;

        case GeneratorOperator::coarseTune:                 Text = riff::FormatText("Coarse Tune: %d semitones (coarseTune)", (int16_t) amount); break;
        case GeneratorOperator::fineTune:                   Text = riff::FormatText("Fine Tune: %d cents (fineTune)", (int16_t) amount); break;

        case GeneratorOperator::scaleTuning:                Text = riff::FormatText("Scale Tuning: %d (scaleTuning)", (int16_t) amount); break;

        case 14: Text = "Unused"; break;
        case 18: Text = "Unused"; break;
        case 19: Text = "Unused"; break;
        case 20: Text = "Unused"; break;
        case 59: Text = "Unused"; break;
        case 60: Text = "Unused"; break;

        case 42: Text = "Reserved"; break;
        case 49: Text = "Reserved"; break;
        case 55: Text = "Reserved"; break;

        default: Text = "Unknown"; break;
    }

    return Text;
}

/// <summary>
/// Describes a SoundFont Modulator Source (8.2).
/// </summary>
std::string bank_t::DescribeModulatorSource(uint16_t modulator) const noexcept
{
    std::string Text;

    // 8.2.1. Is the MIDI Continuous Controller Flag set?
    if (modulator & 0x0080)
    {
        char t[64];

        // Use the MIDI Controller palette of controllers. The ‘index’ field value corresponds to one of the 128 MIDI Continuous Controller messages as defined in the MIDI specification.
        ::snprintf(t, _countof(t), "MIDI Controller %d", modulator & 0x007F);

        Text = t;
    }
    else
    {
        // Use the General Controller palette of controllers.
        switch (modulator & 0x007F)
        {
            case   0: Text = "No controller"; return Text;      // No controller is to be used. The output of this controller module should be treated as if its value were set to ‘1’. It should not be a means to turn off a modulator.

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

    // Source Direction
    Text.append((modulator & 0x0100) ? ", Max to min" : ", Min to max");

    // Source Polarity
    Text.append((modulator & 0x0200) ? ", -1 to 1 (Bipolar)" : ", 0 to 1 (Unipolar)");

    // Source Type
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
/// Describes a SoundFont Modulator Transform (8.3).
/// </summary>
std::string bank_t::DescribeModulatorTransform(uint16_t modulator) const noexcept
{
    std::string Text;

    switch (modulator)
    {
        case 0: Text = "Linear"; break;         // The output value of the multiplier is to be fed directly to the summing node of the given destination.
        case 2: Text = "Absolute Value"; break; // The output value of the multiplier is to be the absolute value of the input value, as defined by the relationship: output = square root ((input value)^2) or alternatively output = output * sgn(output)

        default: Text = "Reserved"; break;
    }

    return Text;
}

/// <summary>
/// Describes a sample type.
/// </summary>
std::string bank_t::DescribeSampleType(uint16_t sampleType) const noexcept
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
