
/** $VER: main.cpp (2025.07.27) P. Stuer **/

#include "pch.h"

#include <libsf.h>

#include "Encoding.h"

static void ProcessDirectory(const fs::path & directoryPath);
static void ProcessFile(const fs::path & filePath);
static void ExamineFile(const fs::path & filePath);

static void ProcessDLS(const fs::path & filePath);
static void ProcessSF(const fs::path & filePath);
static void ProcessECW(const fs::path & filePath);

static void ConvertDLS(const sf::dls::collection_t & dls, sf::bank_t & bank);
static void ConvertECW(const ecw::waveset_t & ws, sf::bank_t & bank);

static std::string DescribeModulatorController(uint16_t modulator) noexcept;
static std::string DescribeGeneratorController(uint16_t generator, uint16_t amount) noexcept;
static std::string DescribeSampleType(uint16_t sampleType) noexcept;

static const char * GetChunkName(const uint32_t chunkId) noexcept;

fs::path FilePath;

const std::vector<fs::path> Filters = { /*".dls", L".sbk",*/ ".sf2", ".sf3", /*L".ecw"*/ };

class arguments_t
{
public:
    void Initialize(int argc, char * argv[]) noexcept
    {
        for (int i = 1; i < argc; ++i)
        {
            if (argv[i][0] == '-')
            {
                if (::_stricmp(argv[i], "-presetzones") == 0) Items["presetzones"] = "";
            }

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

    ::printf("\xEF\xBB\xBF"); // UTF-8 BOM

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
    ::printf("\"%s\"\n", directoryPath.string().c_str());

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
    auto FileSize = fs::file_size(filePath);

    ::printf("\n\"%s\", %llu bytes\n", filePath.string().c_str(), (uint64_t) FileSize);

    ExamineFile(filePath);
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
    catch (sf::exception e)
    {
        ::printf("Failed to process soundfont: %s\n\n", e.what());
    }
    catch (riff::exception e)
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
            sr.Process({ true }, Bank);

        ms.Close();
    }

    uint32_t __TRACE_LEVEL = 0;

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

    // Dump the presets.
    {
        ::printf("%*sPresets (%zu)\n", __TRACE_LEVEL * 2, "", Bank.Presets.size() - 1);
        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & Preset : Bank.Presets)
        {
            ::printf("%*s%5zu. \"%-20s\", Bank %5d, Program %3d, Zone %6d\n", __TRACE_LEVEL * 2, "", i++, Preset.Name.c_str(), Preset.MIDIBank, Preset.MIDIProgram, Preset.ZoneIndex);

            if (i == Bank.Presets.size() - 1)
                break;
        }

        __TRACE_LEVEL--;
    }

    // Dump the preset zones.
    if (Arguments.IsSet("presetzones"))
    {
        ::printf("%*sPreset Zones (%zu)\n", __TRACE_LEVEL * 2, "", Bank.PresetZones.size());

        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & pz : Bank.PresetZones)
        {
            ::printf("%*s%5zu. Generator: %5d, Modulator: %5d\n", __TRACE_LEVEL * 2, "", i++, pz.GeneratorIndex, pz.ModulatorIndex);
        }

        __TRACE_LEVEL--;
    }

    // Dump the preset zone modulators.
    if (Arguments.IsSet("presetzonemodulators"))
    {
        ::printf("%*sPreset Zone Modulators (%zu)\n", __TRACE_LEVEL * 2, "", Bank.PresetZoneModulators.size());

        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & pzm : Bank.PresetZoneModulators)
        {
            ::printf("%*s%5zu. Src Op: 0x%04X, Dst Op: %2d, Amount: %6d, Amount Source: 0x%04X, Source Transform: 0x%04X, Src Op: \"%s\"\n", __TRACE_LEVEL * 2, "", i++,
                pzm.SrcOperator, pzm.DstOperator, pzm.Amount, pzm.AmountSource, pzm.SourceTransform, DescribeModulatorController(pzm.SrcOperator).c_str());
        }

        __TRACE_LEVEL--;
    }

    // Dump the preset zone generators.
    if (Arguments.IsSet("presetzonegenerators"))
    {
        ::printf("%*sPreset Zone Generators (%zu)\n", __TRACE_LEVEL * 2, "", Bank.PresetZoneGenerators.size());

        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & pzg : Bank.PresetZoneGenerators)
        {
            ::printf("%*s%5zu. Operator: 0x%04X, Amount: 0x%04X, \"%s\"\n", __TRACE_LEVEL * 2, "", i++,
                pzg.Operator, pzg.Amount, DescribeGeneratorController(pzg.Operator, pzg.Amount).c_str());

            if (pzg.Operator == 41 /* instrument */)
                ::putchar('\n');
        }

        __TRACE_LEVEL--;
    }

    // Dump the instruments.
    if (Arguments.IsSet("instruments"))
    {
        ::printf("%*sInstruments (%zu)\n", __TRACE_LEVEL * 2, "", Bank.Instruments.size() - 1);

        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & Instrument : Bank.Instruments)
        {
            ::printf("%*s%5zu. \"%-20s\", Instrument Zone %6d\n", __TRACE_LEVEL * 2, "", i++, Instrument.Name.c_str(), Instrument.ZoneIndex);

            if (i == Bank.Instruments.size() - 1)
                break;
        }

        __TRACE_LEVEL--;
    }

    // Dump the instrument zones.
    if (Arguments.IsSet("instrumentzones"))
    {
        ::printf("%*sInstrument Zones (%zu)\n", __TRACE_LEVEL * 2, "", Bank.InstrumentZones.size());

        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & iz : Bank.InstrumentZones)
        {
            ::printf("%*s%5zu. Generator %5d, Modulator %5d\n", __TRACE_LEVEL * 2, "", i++, iz.GeneratorIndex, iz.ModulatorIndex);
        }

        __TRACE_LEVEL--;
    }

    // Dump the instrument zone modulators.
    if (Arguments.IsSet("instrumentzonemodulators"))
    {
        ::printf("%*sInstrument Zone Modulators (%zu)\n", __TRACE_LEVEL * 2, "", Bank.InstrumentZoneModulators.size());

        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & izm : Bank.InstrumentZoneModulators)
        {
            ::printf("%*s%5zu. Src Op: 0x%04X, Dst Op: 0x%04X, Amount: %6d, Amount Source: 0x%04X, Source Transform: 0x%04X, Src Op: \"%s\", Dst Op: \"%s\"\n", __TRACE_LEVEL * 2, "", i++,
                izm.SrcOperator, izm.DstOperator, izm.Amount, izm.AmountSource, izm.SourceTransform, DescribeModulatorController(izm.SrcOperator).c_str(), DescribeModulatorController(izm.DstOperator).c_str());
        }

        __TRACE_LEVEL--;
    }

    // Dump the instrument zone generators.
    if (Arguments.IsSet("instrumentzonegenerators"))
    {
        ::printf("%*sInstrument Zone Generators (%zu)\n", __TRACE_LEVEL * 2, "", Bank.InstrumentZoneGenerators.size());

        __TRACE_LEVEL++;

        size_t i = 0;

        for (const auto & izg : Bank.InstrumentZoneGenerators)
        {
            ::printf("%*s%5zu. Operator: 0x%04X, Amount: 0x%04X, \"%s\"\n", __TRACE_LEVEL * 2, "", i++, izg.Operator, izg.Amount, DescribeGeneratorController(izg.Operator, izg.Amount).c_str());

            if (izg.Operator == 53 /* sampleID */)
                ::putchar('\n');
        }

        __TRACE_LEVEL--;
    }

    // Dump the sample names (SoundFont v1.0 only).
    if (Arguments.IsSet("samplenames"))
    {
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
    }

    // Dump the samples.
    if (Arguments.IsSet("samples"))
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
            ::printf("%*sMIDI Key: %3d - %3d, Velocity: %3d - %3d, Options: 0x%04X, Key Group: %d, Zone: %d\n", __TRACE_LEVEL * 2, "",
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

//  ConvertDLS(dls, sf);
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

            ::printf("\n\"%s\"\n", riff::WideToUTF8(FilePath.wstring()).c_str());

            {
                if (fs.Open(FilePath.wstring(), true))
                {
                    sf::writer_t sw;

                    if (sw.Open(&fs, riff::writer_t::option_t::PolyphoneCompatible))
                        sw.Process({ }, Bank);

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

                    bank.InstrumentZones.push_back(sf::instrument_zone_t((uint16_t) bank.InstrumentZoneGenerators.size(), (uint16_t) bank.InstrumentZoneModulators.size()));

                    bank.InstrumentZoneGenerators.push_back(sf::instrument_zone_generator_t(43, MAKEWORD(it->LowKey, it->HighKey))); // Generator "keyRange"
                    bank.InstrumentZoneGenerators.push_back(sf::instrument_zone_generator_t(53, i)); // Generator "sampleID"

                    ++i;
                }
            }

            bank.Instruments.push_back(sf::instrument_t("EOI"));

            // Add the instrument zone modulators.
            bank.InstrumentZoneModulators.push_back(sf::instrument_zone_modulator_t(0, 0, 0, 0, 0));
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
