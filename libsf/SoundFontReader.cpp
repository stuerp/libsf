
/** $VER: SoundFontReader.cpp (2025.03.23) P. Stuer - Reads an SF2/SF3 compliant sound font. **/

#include "pch.h"

#include "libsf.h"

#include <functional>

using namespace sf;

#pragma region Internal

#pragma pack(push, 2)

struct _preset_header_t
{
    char Name[20];
    uint16_t Preset;            // The MIDI preset number which applies to this preset.
    uint16_t Bank;              // The MIDI bank number which applies to this preset.
    uint16_t ZoneIndex;         // Index in the preset’s zone list. 
    uint32_t Library;           // Unused
    uint32_t Genre;             // Unused
    uint32_t Morphology;        // Unused
};

struct _preset_zone_t
{
    uint16_t GeneratorIndex;    // Index to the preset zone's list of generators in the PGEN chunk.
    uint16_t ModulatorIndex;    // Index to the preset zone's list of modulators in the PMOD chunk.
};

struct _preset_zone_modulator_t
{
    uint16_t SrcOperator;       // Source of data for the modulator.
    uint16_t DstOperator;       // Destination of the modulator.
    int16_t Amount;             // The degree to which the source modulates the destination.
    uint16_t AmountSource;      // The degree to which the source modulates the destination is to be controlled by the specficied modulation source.
    uint16_t SourceTransform;   // Transform that will be applied to the modulation source before application to the modulator.
};

struct _preset_zone_generator_t
{
    uint16_t Operator;          // The operator
    uint16_t Amount;            // The value to be assigned to the generator
};

struct _instrument_t
{
    char Name[20];
    uint16_t ZoneIndex;         // Index to the instrument's zone list in the IBAG chunk.
};

struct _instrument_zone_t
{
    uint16_t GeneratorIndex;    // Index to the instrument zone's list of generators in the IGEN chunk.
    uint16_t ModulatorIndex;    // Index to the instrument zone's list of modulators in the IMOD chunk.
};

struct _instrument_zone_modulator_t
{
    uint16_t SrcOperator;       // Source of data for the modulator.
    uint16_t DstOperator;       // Destination of the modulator.
    int16_t Amount;             // The degree to which the source modulates the destination.
    uint16_t AmountSource;      // The degree to which the source modulates the destination is to be controlled by the specficied modulation source.
    uint16_t SourceTransform;   // Transform that will be applied to the modulation source before application to the modulator.
};

struct _instrument_zone_generator_t
{
    uint16_t Operator;          // The operator
    uint16_t Amount;            // The value to be assigned to the generator
};

struct _sample_header_t
{
    char Name[20];
    uint32_t Start;             // Index from the beginning of the sample data to the start of the sample (in sample data points).
    uint32_t End;               // Index from the beginning of the sample data to the end of the sample (in sample data points).
    uint32_t LoopStart;         // Index from the beginning of the sample data to the loop start of the sample (in sample data points).
    uint32_t LoopEnd;           // Index from the beginning of the sample data to the loop end of the sample (in sample data points).
    uint32_t SampleRate;        // Sample rate in Hz at which this sample was acquired.
    uint8_t Pitch;              // MIDI key number of the recorded pitch of the sample.
    int8_t PitchCorrection;     // Pitch correction in cents that should be applied to the sample on playback.
    uint16_t SampleLink;        // Index of the sample header of the associated right or left stereo sample for SampleTypes LeftSample or RightSample respectively.
    uint16_t SampleType;        // Mask 0xC000 indicates a ROM sample.
};

#pragma pack(pop)

#pragma endregion

/// <summary>
/// Processes the complete SoundFont.
/// </summary>
void soundfont_reader_t::Process(const soundfont_reader_options_t & options, soundfont_t & sf)
{
    TRACE_RESET();
    TRACE_INDENT();

    uint32_t FormType;

    ReadHeader(FormType);

    if (FormType != FOURCC_SFBK)
        throw sf::exception("Unexpected header type");

    TRACE_FORM(FormType);
    TRACE_INDENT();

    std::function<bool(const riff::chunk_header_t & ch)> ChunkHandler = [this, &options, &sf, &ChunkHandler](const riff::chunk_header_t & ch) -> bool
    {
        switch (ch.Id)
        {
            case FOURCC_LIST:
            {
                uint32_t ListType;

                if (ch.Size < sizeof(ListType))
                    throw sf::exception("Invalid list chunk");

                Read(&ListType, sizeof(ListType));

                TRACE_LIST(ListType, ch.Size);
                TRACE_INDENT();

                ReadChunks(ch.Id, ch.Size - sizeof(ListType), ChunkHandler);

                TRACE_UNINDENT();
                break;
            }

            // Mandatory sub-chunk identifying the SoundFont specification version level to which the file complies.
            case FOURCC_IFIL:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                Read(&sf.Major, sizeof(sf.Major));
                Read(&sf.Minor, sizeof(sf.Minor));

                #ifdef __TRACE
                ::printf("%*sSoundFont specification version: %d.%d\n", __TRACE_LEVEL * 2, "", sf.Major, sf.Minor);
                #endif

                TRACE_UNINDENT();
                break;
            }

            // Mandatory sub-chunk identifying the wavetable sound engine for which the file was optimized. 
            case FOURCC_ISNG:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                sf.SoundEngine.resize((size_t) ch.Size);

                Read((void *) sf.SoundEngine.c_str(), ch.Size);

                #ifdef __TRACE
                ::printf("%*sSound Engine: \"%s\"\n", __TRACE_LEVEL * 2, "", sf.SoundEngine.c_str());
                #endif

                TRACE_UNINDENT();
                break;
            }

            // Optional sub-chunk identifying a particular wavetable sound data ROM to which any ROM samples refer.
            case FOURCC_IROM:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                sf.ROM.resize((size_t) ch.Size);

                Read((void *) sf.ROM.c_str(), ch.Size);

                #ifdef __TRACE
                ::printf("%*sROM: \"%s\"\n", __TRACE_LEVEL * 2, "", sf.ROM.c_str());
                #endif

                TRACE_UNINDENT();
                break;
            }

            // Optional sub-chunk identifying the particular wavetable sound data ROM revision to which any ROM samples refer.
            case FOURCC_IVER:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                Read(&sf.ROMMajor, sizeof(sf.ROMMajor));
                Read(&sf.ROMMinor, sizeof(sf.ROMMinor));

                #ifdef __TRACE
                ::printf("%*sROM Version: %d.%d\n", __TRACE_LEVEL * 2, "", sf.ROMMajor, sf.ROMMinor);
                #endif

                TRACE_UNINDENT();
                break;
            }

            // Contains one or more samples of digital audio information in the form of linearly coded 16 bit, signed, little endian (least significant byte first) words.
            case FOURCC_SMPL:
            {
                TRACE_CHUNK(ch.Id, ch.Size);

                if (options.ReadSampleData)
                {
                    sf.SampleData.resize(ch.Size);

                    Read(sf.SampleData.data(), ch.Size);
                }
                else    
                    SkipChunk(ch);

                break;
            }

            // Contains the least significant byte counterparts to each sample data point contained in the smpl chunk. Note this means for every two bytes in the [smpl] sub-chunk there is a 1-byte counterpart in [sm24] sub-chunk.
            case FOURCC_SM24:
            {
                TRACE_CHUNK(ch.Id, ch.Size);

                SkipChunk(ch);
                break;
            }

            // Mandatory sub-chunk listing all presets within the SoundFont compatible file.
            case FOURCC_PHDR:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                _preset_header_t PresetHeader = { };
                const size_t PresetHeaderCount = ch.Size / sizeof(PresetHeader);

                for (size_t i = 0; i < PresetHeaderCount; ++i)
                {
                    Read(&PresetHeader, sizeof(PresetHeader));

                    std::string Name; Name.assign(PresetHeader.Name, _countof(PresetHeader.Name));

                    sf.Presets.push_back({ Name, PresetHeader.Bank, PresetHeader.Preset, PresetHeader.ZoneIndex });

                    #ifdef __TRACE
                    ::printf("%*s%5zu. %-20s, Bank %3d, Preset %3d, Zone %6d\n", __TRACE_LEVEL * 2, "", i + 1, Name.c_str(), PresetHeader.Bank, PresetHeader.Preset, PresetHeader.ZoneIndex);
                    #endif
                }

                TRACE_UNINDENT();
                break;
            }

            // Mandatory sub-chunk listing all preset zones within the SoundFont compatible file.
            case FOURCC_PBAG:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                preset_zone_t Zone = { };
                const size_t PresetZoneCount = ch.Size / sizeof(Zone);

                for (size_t i = 0; i < PresetZoneCount; ++i)
                {
                    Read(&Zone, sizeof(Zone));

                    sf.PresetZones.push_back({ Zone.GeneratorIndex, Zone.ModulatorIndex });

                    #ifdef __TRACE
                    ::printf("%*s%5zu. Generator %5d, Modulator %5d\n", __TRACE_LEVEL * 2, "", i + 1, Zone.GeneratorIndex, Zone.ModulatorIndex);
                    #endif
                }

                TRACE_UNINDENT();
                break;
            }

            // Mandatory sub-chunk listing listing all preset zone modulators within the SoundFont compatible file.
            case FOURCC_PMOD:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                _preset_zone_modulator_t Modulator = { };
                const size_t ModulatorCount = ch.Size / sizeof(Modulator);

                for (size_t i = 0; i < ModulatorCount; ++i)
                {
                    Read(&Modulator, sizeof(Modulator));

                    sf.PresetZoneModulators.push_back({ Modulator.SrcOperator, Modulator.DstOperator, Modulator.Amount, Modulator.AmountSource, Modulator.SourceTransform });

                    #ifdef __TRACE
                    ::printf("%*s%5zu. Src Op: 0x%04X, Dst Op: %2d, Amount: %6d, Amount Source: 0x%04X, Source Transform: 0x%04X, %s\n", __TRACE_LEVEL * 2, "", i + 1,
                        Modulator.SrcOperator, Modulator.DstOperator, Modulator.Amount, Modulator.AmountSource, Modulator.SourceTransform,
                        DescribeModulatorController(Modulator.SrcOperator).c_str());
                    #endif
                }

                TRACE_UNINDENT();
                break;
            }

            // Mandatory sub-chunk listing all preset zone generators for each preset zone within the SoundFont compatible file.
            case FOURCC_PGEN:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                _preset_zone_generator_t Generator = { };
                const size_t GeneratorCount = ch.Size / sizeof(Generator);

                for (size_t i = 0; i < GeneratorCount; ++i)
                {
                    Read(&Generator, sizeof(Generator));

                    sf.PresetZoneGenerators.push_back({ Generator.Operator, Generator.Amount });

                    #ifdef __TRACE
                    ::printf("%*s%5zu. Operator: 0x%04X, Amount: 0x%04X, %s\n", __TRACE_LEVEL * 2, "", i + 1,
                        Generator.Operator, Generator.Amount,
                        DescribeGeneratorController(Generator.Operator).c_str());
                    #endif
                }

                TRACE_UNINDENT();
                break;
            }

            // Mandatory sub-chunk listing all instruments within the SoundFont compatible file.
            case FOURCC_INST:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                _instrument_t Instrument = { };
                const size_t InstrumentCount = ch.Size / sizeof(Instrument);

                for (size_t i = 0; i < InstrumentCount; ++i)
                {
                    Read(&Instrument, sizeof(Instrument));

                    std::string Name; Name.assign(Instrument.Name, _countof(Instrument.Name));

                    sf.Instruments.push_back(instrument_t(Name, Instrument.ZoneIndex));

                    #ifdef __TRACE
                    ::printf("%*s%5zu. \"%-20s\", Zone %5d\n", __TRACE_LEVEL * 2, "", i + 1, Name.c_str(), Instrument.ZoneIndex);
                    #endif
                }

                TRACE_UNINDENT();
                break;
            }

            // Mandatory sub-chunk listing all instrument zones within the SoundFont compatible file.
            case FOURCC_IBAG:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                _instrument_zone_t Zone = { };
                const size_t InstrumentZoneCount = ch.Size / sizeof(Zone);

                for (size_t i = 0; i < InstrumentZoneCount; ++i)
                {
                    Read(&Zone, sizeof(Zone));

                    sf.InstrumentZones.push_back({ Zone.GeneratorIndex, Zone.ModulatorIndex });

                    #ifdef __TRACE
                    ::printf("%*s%5zu. Generator %5d, Modulator %5d\n", __TRACE_LEVEL * 2, "", i + 1, Zone.GeneratorIndex, Zone.ModulatorIndex);
                    #endif
                }

                TRACE_UNINDENT();
                break;
            }

            // Mandatory sub-chunk listing all instrument zone modulators within the SoundFont compatible file.
            case FOURCC_IMOD:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                _instrument_zone_modulator_t Modulator = { };
                const size_t ModulatorCount = ch.Size / sizeof(Modulator);

                for (size_t i = 0; i < ModulatorCount; ++i)
                {
                    Read(&Modulator, sizeof(Modulator));

                    sf.InstrumentZoneModulators.push_back({ Modulator.SrcOperator, Modulator.DstOperator, Modulator.Amount, Modulator.AmountSource, Modulator.SourceTransform });

                    #ifdef __TRACE
                    ::printf("%*s%5zu. Src Op: 0x%04X, Dst Op: %2d, Amount: %6d, Amount Source: 0x%04X, Source Transform: 0x%04X, Src: %s, Dst: %s\n", __TRACE_LEVEL * 2, "", i + 1,
                        Modulator.SrcOperator, Modulator.DstOperator, Modulator.Amount, Modulator.AmountSource, Modulator.SourceTransform,
                        DescribeModulatorController(Modulator.SrcOperator).c_str(),
                        DescribeGeneratorController(Modulator.DstOperator).c_str());
                    #endif
                }

                TRACE_UNINDENT();
                break;
            }

            // Mandatory sub-chunk listing all instrument zone generators for each instrument zone within the SoundFont compatible file.
            case FOURCC_IGEN:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                _instrument_zone_generator_t Generator = { };
                const size_t GeneratorCount = ch.Size / sizeof(Generator);

                for (size_t i = 0; i < GeneratorCount; ++i)
                {
                    Read(&Generator, sizeof(Generator));

                    sf.InstrumentZoneGenerators.push_back({ Generator.Operator, Generator.Amount });

                    #ifdef __TRACE
                    ::printf("%*s%5zu. Operator: 0x%04X, Amount: 0x%04X, %s\n", __TRACE_LEVEL * 2, "", i + 1,
                        Generator.Operator, Generator.Amount,
                        DescribeGeneratorController(Generator.Operator).c_str());
                    #endif
                }

                TRACE_UNINDENT();
                break;
            }

            // Mandatory sub-chunk listing all samples within the SMPL sub-chunk and any referenced ROM samples.
            case FOURCC_SHDR:
            {
                TRACE_CHUNK(ch.Id, ch.Size);
                TRACE_INDENT();

                _sample_header_t SampleHeader = { };
                const size_t SampleHeaderCount = ch.Size / sizeof(SampleHeader);

                for (size_t i = 0; i < SampleHeaderCount; ++i)
                {
                    Read(&SampleHeader, sizeof(SampleHeader));

                    std::string Name; Name.assign(SampleHeader.Name, _countof(SampleHeader.Name));

                    sf.Samples.push_back({ Name, SampleHeader.Start, SampleHeader.End, SampleHeader.LoopStart, SampleHeader.LoopEnd,
                        SampleHeader.SampleRate, SampleHeader.Pitch, SampleHeader.PitchCorrection,
                        SampleHeader.SampleType, SampleHeader.SampleLink });

                    #ifdef __TRACE
                    ::printf("%*s%5zu. \"%-20s\", %9d-%9d, Loop: %9d-%9d, %6dHz, Pitch: %3d, Pitch Correction: %3d, Type: 0x%04X, Link: %5d\n", __TRACE_LEVEL * 2, "", i + 1,
                        Name.c_str(), SampleHeader.Start, SampleHeader.End, SampleHeader.LoopStart, SampleHeader.LoopEnd,
                        SampleHeader.SampleRate, SampleHeader.Pitch, SampleHeader.PitchCorrection,
                        SampleHeader.SampleType, SampleHeader.SampleLink);
                    #endif
                }

                TRACE_UNINDENT();
                break;
            }

            default:
            {
                if ((ch.Id & mmioFOURCC(0xFF, 0, 0, 0)) == mmioFOURCC('I', 0, 0, 0))
                {
                    // Information chunks
                    HandleIxxx(ch.Id, ch.Size, sf.Properties);
                }
                else
                {
                    TRACE_CHUNK(ch.Id, ch.Size);

                    SkipChunk(ch);
                }
            }
        }

        return true;
    };

    ReadChunks(ChunkHandler);

    TRACE_UNINDENT(); // FORM

    TRACE_UNINDENT(); // File
}

/// <summary>
/// Describes a SoundFont modulator controller.
/// </summary>
std::string soundfont_reader_t::DescribeModulatorController(uint16_t modulator) noexcept
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
std::string soundfont_reader_t::DescribeGeneratorController(uint16_t generator) noexcept
{
    std::string Text;

    switch (generator & 0x007F)
    {
        case  0: Text = "Start Offset"; break;
        case  1: Text = "End Offset"; break;
        case  2: Text = "Loop Start Offset"; break;
        case  3: Text = "Loop End Offset"; break;

        case  4: Text = "Start Coarse Offset"; break;

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

        case 15: Text = "chorusEffectsSend"; break;
        case 16: Text = "reverbEffectsSend"; break;

        case 17: Text = "Pan"; break;

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
        case 38: Text = "releaseVolEnv"; break;
        case 39: Text = "keynumToVolEnvHold"; break;
        case 40: Text = "keynumToVolEnvDecay"; break;

        case 41: Text = "Instrument"; break;

        case 42: Text = "Reserved"; break;

        case 43: Text = "keyRange"; break;
        case 44: Text = "velRange"; break;
        case 45: Text = "startloopAddrsCoarseOffset"; break;
        case 46: Text = "keynum"; break;
        case 47: Text = "Velocity"; break;
        case 48: Text = "Initial Attenuation"; break;

        case 49: Text = "Reserved"; break;

        case 50: Text = "endloopAddrsCoarseOffset"; break;
        case 51: Text = "coarseTune"; break;
        case 52: Text = "fineTune"; break;
        case 53: Text = "sampleID"; break;
        case 54: Text = "sampleModes"; break;

        case 55: Text = "Reserved"; break;

        case 56: Text = "Scale Tuning"; break;
        case 57: Text = "Exclusive Class"; break;
        case 58: Text = "Overriding Root Key"; break;

        case 59: Text = "Unused"; break;
        case 60: Text = "Unused"; break;

        default: Text = "Unknown"; break;
    }

    return Text;
}
