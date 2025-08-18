
/** $VER: ECWReader.cpp (2025.05.01) P. Stuer - Implements a reader for a ECW wave set. **/

#include "pch.h"

//#define __TRACE

#include "libsf.h"

#include "ECW.h"

using namespace ecw;

/// <summary>
/// Reads the complete waveset.
/// </summary>
void reader_t::Process(waveset_t & ws)
{
    ecwHeader Header = { };

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

    ws.Instruments.resize(Header.InstrumentHeaderCount);
    Offset(Header.InstrumentHeaderOffs);
    Read(ws.Instruments.data(), Header.InstrumentHeaderSize);

    ws.Patches.resize(Header.PatchHeaderCount);
    Offset(Header.PatchHeaderOffs);
    Read(ws.Patches.data(), Header.PatchHeaderSize);

    // Read the sample data.
    ws.SampleData.resize(Header.SampleDataSize);

    Offset(Header.SampleDataOffs);
    Read(ws.SampleData.data(), Header.SampleDataSize);

    // Read the sample sets.
    {
        Offset(Header.SampleDataOffs + 22);

        uint16_t SampleSetCount = 0;
        Read(SampleSetCount);

        std::vector<ecwSampleSet> SampleSets(SampleSetCount);

        Offset(Header.SampleDataOffs + 40);
        Read(SampleSets.data(), SampleSetCount * sizeof(ecwSampleSet));

        ws.SampleSets.reserve(SampleSets.size());

        for (auto & SampleSet : SampleSets)
        {
            std::string Name(SampleSet.Name, sizeof(SampleSet.Name));

            // Remove trailing spaces.
            for (auto it = Name.rbegin(); it != Name.rend(); ++it)
            {
                if ((*it != ' ') && (*it != '\0'))
                    break;

                *it = '\0';
            }

            ws.SampleSets.push_back(sample_set_t
            (
                Name,
                SampleSet.SampleIndex,
                SampleSet.Array1Index,
                SampleSet.Code
            ));
        }

        // Read and process array 1.
        {
            std::vector<uint16_t> Array1(Header.Array1Count);

            Offset(Header.Array1Offs);
            Read(Array1.data(), Header.Array1Size);

            ws.Array1.reserve(Array1.size());

            uint16_t i = 0;

            for (const auto & Item : Array1)
            {
                if (Item != 0xFFFF)
                {
                    std::string Name;

                    for (const auto & ss : ws.SampleSets)
                    {
                        if (ss.Array1Index == i)
                        {
                            Name = ss.Name;
                            break;
                        }
                    }

                    ws.Array1.push_back(slot_t(Item, Name));
                }
                else
                    ws.Array1.push_back(slot_t(Item, ""));

                ++i;
            }
        }

        // Read and process array 2.
        {
            std::vector<uint16_t> Array2(Header.Array2Count);

            Offset(Header.Array2Offs);
            Read(Array2.data(), Header.Array2Size);

            ws.Array2.reserve(Array2.size());

            std::vector<size_t> SampleSetArray3Index(Array2.size());

            {
                uint16_t i = 0;

                for (const auto & Item : Array2)
                {
                    if (Item != 0)
                    {
                        std::string Name;

                        // Get the name of the sample set.
                        size_t j = 0;

                        for (const auto & ss : ws.SampleSets)
                        {
                            if (Item == ss.Code)
                            {
                                Name = ss.Name;
                                SampleSetArray3Index[i] = j;
                                break;
                            }

                            ++j;
                        }

                        ws.Array2.push_back(slot_t(Item, Name));
                    }
                    else
                        ws.Array2.push_back(slot_t(Item, ""));

                    ++i;
                }
            }

            // Read and process array 3.
            {
                std::vector<uint16_t> Array3(Header.Array3Count);

                Offset(Header.Array3Offs);
                Read(Array3.data(), Header.Array3Size);

                ws.Array3.reserve(Array3.size());

                std::vector<size_t> SampleIndexInSampleSet(ws.Array1.size(), ~0u);

                {
                    size_t i = 0;

                    for (const auto & Item : Array3)
                    {
                        if (Item != 0)
                            ws.Array3.push_back(slot_t(Item, ws.SampleSets[SampleSetArray3Index[i]].Name));
                        else
                            ws.Array3.push_back(slot_t(Item, ""));

                        SampleIndexInSampleSet[Item] = SampleSetArray3Index[i];

                        ++i;
                    }
                }

                // Read the samples.
                {
                    std::vector<ecwSample> Samples(Header.SampleHeaderCount);

                    Offset(Header.SampleHeaderOffs);
                    Read(Samples.data(), Header.SampleHeaderSize);

                    ws.Samples.reserve(Samples.size());

                    size_t i = 0;

                    uint32_t SampleStart = ~0u;
                    std::string SampleSetName;
                    uint8_t HighKey = 0;

                    for (auto & Sample : Samples)
                    {
                        uint8_t LowKey = (HighKey == 127) ? 0 : HighKey + 1;
                        HighKey = Sample.SplitPoint;

                        if (SampleIndexInSampleSet[i] != -1)
                            SampleSetName = ws.SampleSets[SampleIndexInSampleSet[i]].Name;

                        ws.Samples.push_back(sample_t
                        (
                            SampleSetName,
                            LowKey,
                            HighKey,
                            Sample.Flags,
                            Sample.FineTune,
                            Sample.CoarseTune,
                            Sample.SampleStart / 8,
                            Sample.LoopStart / 8,
                            Sample.LoopEnd /  8
                        ));

                        if (Sample.SampleStart < SampleStart)
                            SampleStart = Sample.SampleStart;

                        ++i;
                    }
                }
            }
        }
    }
}
