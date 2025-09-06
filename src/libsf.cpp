
/** $VER: SoundFont.cpp (2025.09.03) P. Stuer **/

#include "pch.h"

#include "libsf.h"

#include "SF2.h"

#include <mmreg.h>

using namespace sf;

static void ApplyKeyNumToCorrection(std::vector<sf::generator_t> & generators, int16_t value, GeneratorOperator keynumToOperator, GeneratorOperator realOperator);

/// <summary>
/// Initializes a SoundFont bank from a DLS collection.
/// </summary>
void bank_t::ConvertFrom(const dls::collection_t & collection)
{
    Major       = 2;
    Minor       = 4;
    SoundEngine = "E-mu 10K2"; // https://en.wikipedia.org/wiki/E-mu_20K
    Name        = GetPropertyValue(collection.Properties, FOURCC_INAM);

    {
         SYSTEMTIME st = {};

        ::GetLocalTime(&st);

        char Date[32] = { };
        char Time[32] = { };

        ::GetDateFormatA(LOCALE_USER_DEFAULT, 0, &st, "yyyy-MM-dd", Date, sizeof(Date));
        ::GetTimeFormatA(LOCALE_USER_DEFAULT, 0, &st, "HH:mm:ss", Time, sizeof(Time));

        Properties.push_back(sf::property_t(FOURCC_ICRD, std::string(Date) + " " + std::string(Time)));
    }

    for (const auto & Property : collection.Properties)
    {
        if (Property.Id == FOURCC_INAM)
            continue;

        Properties.push_back(sf::property_t(Property.Id, Property.Value));
    }

    // Write the Hydra.
    {
        // Add the instruments.
        {
            for (const auto & Instrument : collection.Instruments)
            {
                const uint16_t Bank = !Instrument.IsPercussion ? ((Instrument.BankMSB != 0) ? Instrument.BankMSB : Instrument.BankLSB) : 128;

                // Add a preset.
                {
                    const std::string PresetName = !Instrument.Name.empty() ? Instrument.Name : msc::FormatText("Preset %d-%d", Bank, Instrument.Program);

                    if (PresetZones.size() >= 65536)
                        throw sf::exception(msc::FormatText("Maximum number of preset zones exceeded when creating preset \"%s\"", PresetName.c_str()));

                    Presets.push_back(sf::preset_t(PresetName, Instrument.Program, Bank, (uint16_t) PresetZones.size()));

                    {
                        if (PresetGenerators.size() >= 65536)
                            throw sf::exception(msc::FormatText("Maximum number of preset generators exceeded when creating preset \"%s\"", PresetName.c_str()));

                        if (PresetModulators.size() >= 65536)
                            throw sf::exception(msc::FormatText("Maximum number of preset modulators exceeded when creating preset \"%s\"", PresetName.c_str()));

                        // Add a global preset zone. FIXME: Is this really necessary? We're not adding any generators.
                        PresetZones.push_back(sf::preset_zone_t((uint16_t) PresetGenerators.size(), (uint16_t) PresetModulators.size()));

                        // Add a local zone.
                        PresetZones.push_back(sf::preset_zone_t((uint16_t) PresetGenerators.size(), (uint16_t) PresetModulators.size()));

                        PresetGenerators.push_back(sf::generator_t(GeneratorOperator::instrument, (uint16_t) Instruments.size()));
                    }
                }

                const std::string InstrumentName = !Instrument.Name.empty() ? Instrument.Name : msc::FormatText("Instrument %d-%d", Bank, Instrument.Program);

                if (InstrumentZones.size() >= 65536)
                    throw sf::exception(msc::FormatText("Maximum number of instrument zones exceeded when creating instrument \"%s\"", InstrumentName.c_str()));

                Instruments.push_back(sf::instrument_t(InstrumentName, (uint16_t) InstrumentZones.size()));

                if (InstrumentGenerators.size() >= 65536)
                    throw sf::exception(msc::FormatText("Maximum number of instrument generators exceeded when creating preset \"%s\"", InstrumentName.c_str()));

                if (InstrumentModulators.size() >= 65536)
                    throw sf::exception(msc::FormatText("Maximum number of instrument modulators exceeded when creating preset \"%s\"", InstrumentName.c_str()));

                // Add a global instrument zone.
                InstrumentZones.push_back(instrument_zone_t((uint16_t) InstrumentGenerators.size(), (uint16_t) InstrumentModulators.size()));

                {
                    std::vector<generator_t> Generators;
                    std::vector<modulator_t> Modulators;

                    // Convert the instrument articulators, if any.
                    if (!Instrument.Articulators.empty())
                        ConvertArticulators(Instrument.Articulators, Generators, Modulators);

                    // Add a default reverb modulator unless one already exists. (Not defined in the SF2 spec.)
                    {
                        auto it = std::find_if(Modulators.begin(), Modulators.end(), [](modulator_t m) { return (m.DstOper == GeneratorOperator::reverbEffectsSend); });

                        if (it == Modulators.end())
                            Modulators.push_back(modulator_t(MIDIControllerReverb, GeneratorOperator::reverbEffectsSend, 1000, 0, 0));
                    }

                    // Add a default chorus modulator unless one already exists. (Not defined in the SF2 spec.)
                    {
                        auto it = std::find_if(Modulators.begin(), Modulators.end(), [](modulator_t m) { return (m.DstOper == GeneratorOperator::chorusEffectsSend); });

                        if (it == Modulators.end())
                            Modulators.push_back(modulator_t(MIDIControllerChorus, GeneratorOperator::chorusEffectsSend, 1000, 0, 0));
                    }

                    InstrumentGenerators.insert(InstrumentGenerators.end(), Generators.begin(), Generators.end());
                    InstrumentModulators.insert(InstrumentModulators.end(), Modulators.begin(), Modulators.end());
                }

                // Convert the regions to instrument zones.
                for (const auto & Region : Instrument.Regions)
                {
                    // Add a local instrument zone.
                    InstrumentZones.push_back(instrument_zone_t((uint16_t) InstrumentGenerators.size(), (uint16_t) InstrumentModulators.size()));

                    InstrumentGenerators.push_back(sf::generator_t(GeneratorOperator::keyRange, MAKEWORD(Region.LowKey,      Region.HighKey)));         // Must be the first generator.
                    InstrumentGenerators.push_back(sf::generator_t(GeneratorOperator::velRange, MAKEWORD(Region.LowVelocity, Region.HighVelocity)));    // Must only be preceded by keyRange.

                    if (Region.KeyGroup != 0)
                        InstrumentGenerators.push_back(sf::generator_t(GeneratorOperator::exclusiveClass, Region.KeyGroup));

                    // Convert the region articulators, if any.
                    if (!Region.Articulators.empty())
                    {
                        std::vector<generator_t> Generators;
                        std::vector<modulator_t> Modulators;

                        ConvertArticulators(Region.Articulators, Generators, Modulators);

                        InstrumentGenerators.insert(InstrumentGenerators.end(), Generators.begin(), Generators.end());
                        InstrumentModulators.insert(InstrumentModulators.end(), Modulators.begin(), Modulators.end());
                    }

                    // dls.Cues[CueIndex] is actually an offset in the wave pool but we can use the cue index as an index into the wave list.
                    const uint16_t SampleID = (uint16_t) Region.WaveLink.CueIndex;

                    const auto & Wave = collection.Waves[SampleID];

                    // Add an Initial Attenuation generator.
                    {
                        // Decide which gain value to use. (See DLS Spec 3.1 Coding Requirements and Recommendations)
                        int32_t Gain = 0;

                        if (Region.WaveSample.IsInitialized())
                            Gain = Region.WaveSample.Gain;
                        else
                        if (Wave.WaveSample.IsInitialized())
                            Gain = Wave.WaveSample.Gain;

                        const double EmuCorrection = 0.4;

                        // Convert DLS gain from 1/655360 dB units to SF2 attenuation in centibels. (See DLS Spec 1.14.4 Gain)
                        const double Attenuation = ((double) Gain / -65536.) / EmuCorrection;

                        InstrumentGenerators.push_back(sf::generator_t(GeneratorOperator::initialAttenuation, (uint16_t) Attenuation));
                    }

                    // Add a Sample Mode generator.
                    {
                        uint16_t SampleMode = 0; // No loop

                        if (!Wave.WaveSample.Loops.empty())
                        {
                            if (Wave.WaveSample.Loops[0].Type == dls::wave_sample_loop_t::WLOOP_TYPE_FORWARD)
                                SampleMode = 1; // Loop
                            else
                            if (Wave.WaveSample.Loops[0].Type == dls::wave_sample_loop_t::WLOOP_TYPE_RELEASE)
                                SampleMode = 3; // Loop and play till the end in release phase
                        }

                        if (SampleMode != 0)
                        {
                            InstrumentGenerators.push_back(sf::generator_t(GeneratorOperator::sampleModes, SampleMode));

                            // Adjust the loop if necessary.
                            if (!Wave.WaveSample.Loops.empty() && !Region.WaveSample.Loops.empty())
                            {
                                const auto & SrcLoop = Wave  .WaveSample.Loops[0];
                                const auto & DstLoop = Region.WaveSample.Loops[0];

                                const int32_t StartDiff = DstLoop.Start - SrcLoop.Start;
                                const int32_t EndDiff   = (DstLoop.Start + DstLoop.Length) - (SrcLoop.Start + SrcLoop.Length);

                                if (StartDiff != 0)
                                {
                                    const int32_t CoarseOffset = StartDiff / 32768;

                                    if (CoarseOffset != 0)
                                        InstrumentGenerators.push_back(sf::generator_t(GeneratorOperator::startloopAddrsCoarseOffset, (uint16_t) CoarseOffset));

                                    const int32_t FineOffset = StartDiff % 32768;

                                    InstrumentGenerators.push_back(sf::generator_t(GeneratorOperator::startloopAddrsOffset, (uint16_t) FineOffset));
                                }

                                if (EndDiff != 0)
                                {
                                    const int32_t CoarseOffset = EndDiff / 32768;

                                    if (CoarseOffset != 0)
                                        InstrumentGenerators.push_back(sf::generator_t(GeneratorOperator::endloopAddrsCoarseOffset, (uint16_t) CoarseOffset));

                                    const int32_t FineOffset = EndDiff % 32768;

                                    InstrumentGenerators.push_back(sf::generator_t(GeneratorOperator::endloopAddrsOffset, (uint16_t) FineOffset));
                                }
                            }
                        }
                    }

                    // Adjust the tuning if necessary.
                    {
                        int16_t FineTune = Region.WaveSample.FineTune - Wave.WaveSample.FineTune;

                        int16_t CoarseTune = FineTune / 100;

                        if (CoarseTune != 0)
                            InstrumentGenerators.push_back(sf::generator_t(GeneratorOperator::coarseTune, (uint16_t) CoarseTune));

                        FineTune = FineTune % 100;

                        if (FineTune != 0)
                            InstrumentGenerators.push_back(sf::generator_t(GeneratorOperator::fineTune, FineTune));
                    }

                    // Adjust the root key if necessary.
                    if (Region.WaveSample.UnityNote != Wave.WaveSample.UnityNote)
                        InstrumentGenerators.push_back(sf::generator_t(GeneratorOperator::overridingRootKey, Region.WaveSample.UnityNote));

                    InstrumentGenerators.push_back(sf::generator_t(GeneratorOperator::sampleID, SampleID));                                             // Must be the last generator.
                }
            }

            // Add the instrument list terminator.
            Instruments.push_back(sf::instrument_t("EOI", (uint16_t) InstrumentZones.size()));
            InstrumentZones.push_back(instrument_zone_t((uint16_t) InstrumentGenerators.size(), (uint16_t) InstrumentModulators.size()));

            // Add the instrument zone modulator.
            InstrumentModulators.push_back(sf::modulator_t());

            // Add the preset list terminator.
            Presets.push_back(sf::preset_t("EOP", 0, 0,  (uint16_t) PresetZones.size()));
            PresetZones.push_back(sf::preset_zone_t((uint16_t) PresetGenerators.size(), (uint16_t) PresetModulators.size()));

            // Add the preset zone modulator.
            PresetModulators.push_back(sf::modulator_t());
        }

        // Add the samples.
        {
            a_law_codec_t ALawCodec;

            // Calculate the size of the sample data buffer.
            {
                size_t Size = 0;

                for (const auto & wave : collection.Waves)
                {
                    if (wave.Channels != 1)
                        throw sf::exception(msc::FormatText("Unsupported number of channels (%d channels) in wave \"%s\"", wave.Channels, wave.Name.c_str()));

                    if ((wave.BitsPerSample != 8) && (wave.BitsPerSample != 16))
                        throw sf::exception(msc::FormatText("Unsupported sample size (%d bit) in wave \"%s\"", wave.BitsPerSample, wave.Name.c_str()));

                    if (wave.FormatTag == WAVE_FORMAT_PCM)
                    {
                        if (wave.BitsPerSample == 16)
                            Size += wave.Data.size();
                        else
                            Size += wave.Data.size() * 2;   // Allow for 8-bit to 16-bit conversion. 
                    }
                    else
                    if (wave.FormatTag == WAVE_FORMAT_ALAW)
                        Size += wave.Data.size() * 2;       // Allow for 8-bit A-Law to 16-bit PCM conversion.
                    else
                        throw sf::exception(msc::FormatText("Unsupported sample format 0x%04X in wave \"%s\"", wave.FormatTag, wave.Name.c_str()));
                }

                SampleData.resize(Size);
            }

            size_t Offset = 0;

            for (const auto & wave : collection.Waves)
            {
                const size_t DataSize = (wave.BitsPerSample == 16) ? wave.Data.size() : wave.Data.size() * 2;

                const auto BytesPerSample = 2;                                      // SF2 samples are always 16-bit (SoundFont 2.01 Technical Specification, 6 The sdta-list Chunk)

                // Pitch correction: convert 1/100 to note units.
                const  int16_t Semitones = wave.WaveSample.FineTune / 100;          // FineTune in Relative Pitch units (1/65536 cents) (See 1.14.2 Relative Pitch)

                const uint16_t UnityNote = wave.WaveSample.UnityNote + Semitones;
                const  int16_t FineTune  = wave.WaveSample.FineTune % 100;          // FineTune in cents

                sf::sample_t Sample =
                {
                    .Name            = wave.Name,

                    .Start           = (uint32_t) ( Offset             / BytesPerSample),
                    .End             = (uint32_t) ((Offset + DataSize) / BytesPerSample),

                    .SampleRate      = wave.SamplesPerSec,
                    .Pitch           = (uint8_t) UnityNote,
                    .PitchCorrection =  (int8_t) FineTune,

                    .SampleType      = sf::SampleTypes::MonoSample
                };

                if (wave.FormatTag == WAVE_FORMAT_PCM)
                {
                    if (wave.BitsPerSample == 16)
                    {
                        std::memcpy(SampleData.data() + Offset, wave.Data.data(), wave.Data.size());
                    }
                    else
                    {
                        // Convert 8-bit samples to 16-bit (Downloadable Sounds Level 2.2, 2.16.8 Data Format of the WAVE_FORMAT_PCM Samples).
                        auto PCM = std::span<int16_t>((int16_t *)(SampleData.data() + Offset), wave.Data.size());
                        size_t i = 0;

                        for (const auto Byte : wave.Data)
                            PCM[i++] = msc::Map(Byte, (uint8_t) 0, (uint8_t) 255, (int16_t) -32768, (int16_t) 32767);
                    }
                }
                else
                if (wave.FormatTag == WAVE_FORMAT_ALAW)
                {
                    auto PCM = std::span<int16_t>((int16_t *)(SampleData.data() + Offset), wave.Data.size());

                    ALawCodec.ToPCM(wave.Data, PCM);
                }
                else
                    assert(1 == 2); // Should not get here.

                Offset += DataSize;

                if (!wave.WaveSample.Loops.empty())
                {
                    const auto & Loop = wave.WaveSample.Loops[0];

                    Sample.LoopStart = Sample.Start     + Loop.Start;
                    Sample.LoopEnd   = Sample.LoopStart + Loop.Length;
                }
                else
                {
                    Sample.LoopStart = Sample.Start;
                    Sample.LoopEnd   = Sample.End - 1;
                }

                Samples.push_back(Sample);
            }

            Samples.push_back(sf::sample_t("EOS"));
        }
    }
}

/// <summary>
/// Converts the articulators. Based on [spesssynth_core/read_articulation.js](https://github.com/spessasus/spessasynth_core).
/// </summary>
void bank_t::ConvertArticulators(const std::vector<dls::articulator_t> & articulators, std::vector<sf::generator_t> & generators, std::vector<sf::modulator_t> & modulators)
{
    for (const auto & Articulator : articulators)
    {
        for (const auto & ConnectionBlock : Articulator.ConnectionBlocks)
        {
            const int16_t Value = ConnectionBlock.Scale >> 16;

            int16_t Amount = Value;

            if (ConnectionBlock.Control == 0)
            {
                if ((ConnectionBlock.Source == CONN_SRC_NONE) && (ConnectionBlock.Transform == CONN_TRN_NONE))
                {
                    switch (ConnectionBlock.Destination)
                    {
                        // Generic Destinations (Level 1)
                        case CONN_DST_ATTENUATION: // CONN_DST_GAIN
                        {
                            // Convert DLS gain from 1/655360 dB units to SF2 centibels with EMU correction (0.4).
                            const double Attenuation = ((ConnectionBlock.Scale / -655360.) * 10.) / 0.4;

                            generators.push_back(sf::generator_t(GeneratorOperator::initialAttenuation, (uint16_t) Attenuation));
                            break;
                        }

                        case CONN_DST_PITCH:
                        {
                            Amount = (uint16_t) ::floor((double) Value / 100.);

                            generators.push_back(sf::generator_t(GeneratorOperator::coarseTune, (uint16_t) Amount));

                            Amount = (uint16_t) ::floor((double) Value - ((double) Amount * 100.));

                            generators.push_back(sf::generator_t(GeneratorOperator::fineTune, (uint16_t) Amount));
                            break;
                        }

                        case CONN_DST_PAN:
                        {
                            generators.push_back(sf::generator_t(GeneratorOperator::pan, (uint16_t) Amount));
                            break;
                        }

                        // Effect Destinations
                        case CONN_DST_CHORUS:
                        {
                            generators.push_back(sf::generator_t(GeneratorOperator::chorusEffectsSend, (uint16_t) Amount));
                            break;
                        }

                        case CONN_DST_REVERB:
                        {
                            generators.push_back(sf::generator_t(GeneratorOperator::reverbEffectsSend, (uint16_t) Amount));
                            break;
                        }

                        // LFO Destinations (Level 1)
                        case CONN_DST_LFO_FREQUENCY:
                        {
                            generators.push_back(sf::generator_t(GeneratorOperator::freqModLFO, (uint16_t) Amount));
                            break;
                        }

                        case CONN_DST_LFO_STARTDELAY:
                        {
                            // Don't add a generator for amount -32768. It conventionally indicates no delay. (SoundFont 2.04 Technical Specification, 8.1.2 Generator Enumerators Defined)
                            if (Amount != -32768)
                                generators.push_back(sf::generator_t(GeneratorOperator::delayModLFO, (uint16_t) Amount));
                            break;
                        }

                        // Vibrato LFO Destinations (Level 2)
                        case CONN_DST_VIB_FREQUENCY:
                        {
                            generators.push_back(sf::generator_t(GeneratorOperator::freqVibLFO, (uint16_t) Amount));
                            break;
                        }

                        case CONN_DST_VIB_STARTDELAY:
                        {
                            // Don't add a generator for amount -32768. It conventionally indicates no delay. (SoundFont 2.04 Technical Specification, 8.1.2 Generator Enumerators Defined)
                            if (Amount != -32768)
                                generators.push_back(sf::generator_t(GeneratorOperator::delayVibLFO, (uint16_t) Amount));
                            break;
                        }

                        // Volume Envelope Generator Destinations
                        case CONN_DST_EG1_ATTACKTIME:
                        {
                            // Don't add a generator for amount -32768. It conventionally indicates instantaneous attack. (SoundFont 2.04 Technical Specification, 8.1.2 Generator Enumerators Defined)
                            if (Amount != -32768)
                                generators.push_back(sf::generator_t(GeneratorOperator::attackVolEnv, (uint16_t) Amount));
                            break;
                        }

                        case CONN_DST_EG1_DECAYTIME:
                        {
                            generators.push_back(sf::generator_t(GeneratorOperator::decayVolEnv, (uint16_t) Amount));
                            break;
                        }

                        case CONN_DST_EG1_SUSTAINLEVEL:
                        {
                            Amount = 1000 - Value; // Gain seems to be (1000 - value) / 10 = sustain dB (see Spessasynth_core)

                            generators.push_back(sf::generator_t(GeneratorOperator::sustainVolEnv, (uint16_t) Amount));
                            break;
                        }

                        case CONN_DST_EG1_RELEASETIME:
                        {
                            generators.push_back(sf::generator_t(GeneratorOperator::releaseVolEnv, (uint16_t) Amount));
                            break;
                        }

                        case CONN_DST_EG1_DELAYTIME:
                        {
                            // Don't add a generator for amount -32768. It conventionally indicates no delay. (SoundFont 2.04 Technical Specification, 8.1.2 Generator Enumerators Defined)
                            if (Amount != -32768)
                                generators.push_back(sf::generator_t(GeneratorOperator::delayVolEnv, (uint16_t) Amount));
                            break;
                        }

                        case CONN_DST_EG1_HOLDTIME:
                        {
                            // Don't add a generator for amount -32768. It conventionally indicates no hold phase. (SoundFont 2.04 Technical Specification, 8.1.2 Generator Enumerators Defined)
                            if (Amount != -32768)
                                generators.push_back(sf::generator_t(GeneratorOperator::holdVolEnv, (uint16_t) Amount));
                            break;
                        }

                        // Modulation Envelope Generator Destinations
                        case CONN_DST_EG2_ATTACKTIME:
                        {
                            // Don't add a generator for amount -32768. It conventionally indicates instantaneous attack. (SoundFont 2.04 Technical Specification, 8.1.2 Generator Enumerators Defined)
                            if (Amount != -32768)
                                generators.push_back(sf::generator_t(GeneratorOperator::attackModEnv, (uint16_t) Amount));
                            break;
                        }

                        case CONN_DST_EG2_DECAYTIME:
                        {
                            generators.push_back(sf::generator_t(GeneratorOperator::decayModEnv, (uint16_t) Amount));
                            break;
                        }

                        case CONN_DST_EG2_SUSTAINLEVEL:
                        {
                            Amount = 1000 - Value; // DLS uses 1%, SF2 uses 0.1% (see Spessasynth_core)

                            generators.push_back(sf::generator_t(GeneratorOperator::sustainModEnv, (uint16_t) Amount));
                            break;
                        }

                        case CONN_DST_EG2_RELEASETIME:
                        {
                            generators.push_back(sf::generator_t(GeneratorOperator::releaseModEnv, (uint16_t) Amount));
                            break;
                        }

                        case CONN_DST_EG2_DELAYTIME:
                        {
                            // Don't add a generator for amount -32768. It conventionally indicates no delay. (SoundFont 2.04 Technical Specification, 8.1.2 Generator Enumerators Defined)
                            if (Amount != -32768)
                                generators.push_back(sf::generator_t(GeneratorOperator::delayModEnv, (uint16_t) Amount));
                            break;
                        }

                        case CONN_DST_EG2_HOLDTIME:
                        {
                            // Don't add a generator for amount -32768. It conventionally indicates no hold phase. (SoundFont 2.04 Technical Specification, 8.1.2 Generator Enumerators Defined)
                            if (Amount != -32768)
                                generators.push_back(sf::generator_t(GeneratorOperator::holdModEnv, (uint16_t) Amount));
                            break;
                        }

                        // Filter Destinations
                        case CONN_DST_FILTER_CUTOFF:
                        {
                            generators.push_back(sf::generator_t(GeneratorOperator::initialFilterFc, (uint16_t) Amount));
                            break;
                        }

                        case CONN_DST_FILTER_Q:
                        {
                            generators.push_back(sf::generator_t(GeneratorOperator::initialFilterQ, (uint16_t) Amount));
                            break;
                        }
                    }
                }
                else
                if (ConnectionBlock.Source == CONN_SRC_LFO)
                {
                    if (ConnectionBlock.Destination == CONN_DST_PITCH)
                        generators.push_back(sf::generator_t(GeneratorOperator::modLfoToPitch, (uint16_t) Amount));
                    else
                    if (ConnectionBlock.Destination == CONN_DST_ATTENUATION)
                        generators.push_back(sf::generator_t(GeneratorOperator::modLfoToVolume, (uint16_t) Amount));
                    else
                    if (ConnectionBlock.Destination == CONN_DST_FILTER_CUTOFF)
                        generators.push_back(sf::generator_t(GeneratorOperator::modLfoToFilterFc, (uint16_t) Amount));
                }
                else
                if (ConnectionBlock.Source == CONN_SRC_VIBRATO && ConnectionBlock.Destination == CONN_DST_PITCH)
                {
                    generators.push_back(sf::generator_t(GeneratorOperator::vibLfoToPitch, (uint16_t) Amount));
                }
                else
                if (ConnectionBlock.Source == CONN_SRC_EG2)
                {
                    if (ConnectionBlock.Destination == CONN_DST_PITCH)
                        generators.push_back(sf::generator_t(GeneratorOperator::modEnvToPitch, (uint16_t) Amount));
                    else
                    if (ConnectionBlock.Destination == CONN_DST_FILTER_CUTOFF)
                        generators.push_back(sf::generator_t(GeneratorOperator::modEnvToFilterFc, (uint16_t) Amount));
                }
                else
                if (ConnectionBlock.Source == CONN_SRC_KEYNUMBER)
                {
                    if (ConnectionBlock.Destination == CONN_DST_PITCH)
                        generators.push_back(sf::generator_t(GeneratorOperator::scaleTuning, (uint16_t) Amount / 128)); // 12,800 means the regular scale (100)
                    else
                    if (ConnectionBlock.Destination == CONN_DST_EG1_HOLDTIME)
                        ApplyKeyNumToCorrection(generators, Amount, GeneratorOperator::keynumToVolEnvHold, GeneratorOperator::holdVolEnv);
                    else
                    if (ConnectionBlock.Destination == CONN_DST_EG1_DECAYTIME)
                        ApplyKeyNumToCorrection(generators, Amount, GeneratorOperator::keynumToVolEnvDecay, GeneratorOperator::decayVolEnv);
                    else
                    if (ConnectionBlock.Destination == CONN_DST_EG2_HOLDTIME)
                        ApplyKeyNumToCorrection(generators, Amount, GeneratorOperator::keynumToModEnvHold, GeneratorOperator::holdModEnv);
                    else
                    if (ConnectionBlock.Destination == CONN_DST_EG2_DECAYTIME)
                        ApplyKeyNumToCorrection(generators, Amount, GeneratorOperator::keynumToModEnvDecay, GeneratorOperator::decayModEnv);
                }
            }
            else
            {
                ConvertConnectionBlockToModulator(ConnectionBlock, modulators);
            }
        }
    }

    // Remove the generators with a default value.
    std::vector<sf::generator_t> FilteredGenerators;

    std::copy_if(generators.begin(), generators.end(), std::back_inserter(FilteredGenerators), [](const sf::generator_t g)
    {
        return g.Amount != sf::GeneratorLimits.at((GeneratorOperator) g.Operator).Default;
    });


    generators = FilteredGenerators;
}

/// <summary>
/// Applies a correction for the keynumTo- generators. Based on [spesssynth_core/read_articulation.js](https://github.com/spessasus/spessasynth_core).
/// </summary>
static void ApplyKeyNumToCorrection(std::vector<sf::generator_t> & generators, int16_t amount, GeneratorOperator keynumToOperator, GeneratorOperator realOperator)
{
    // According to Viena and another strange (with modulators) conversion of GM.dls to SF2, amount must be divided by -128 and a strange correction needs to be applied to the real value: real + (60 / 128) * scale
    const int16_t keynumToAmount = amount / -128;

    generators.push_back(generator_t(keynumToOperator, keynumToAmount));

    // airfont 340 fix
    if (keynumToAmount <= 120)
    {
        const int16_t Correction = (int16_t) ::round((60. / 128.) * amount);

        for (auto & Generator : generators)
        {
            if (Generator.Operator == realOperator)
                Generator.Amount += Correction;
        };
    }
};

/// <summary>
/// Converts a connection block to a modulator. Based on [spesssynth_core/articulator_convertor.js](https://github.com/spessasus/spessasynth_core).
/// </summary>
void bank_t::ConvertConnectionBlockToModulator(const dls::connection_block_t & connectionBlock, std::vector<modulator_t> & modulators)
{
    const int16_t Value = (int16_t) (connectionBlock.Scale >> 16);

    int16_t Amount = Value;

    ModulatorOperator srcOper      = 0xFFFF;
    ModulatorOperator sf2Source    = 0xFFFF;
    ModulatorOperator sf2SourceAmt = 0xFFFF;

    bool SwapSources = false;

    auto dstOper = GeneratorOperator::Invalid;

    // Determine the modulator source and destination.
    {
        const GeneratorOperator SpecialDestination = GetSpecialGeneratorOperator(connectionBlock);

        if (SpecialDestination != GeneratorOperator::Invalid)
        {
            sf2Source = 0x0000;
            dstOper   = SpecialDestination;

            SwapSources = true;
        }
        else
        {
            sf2Source = ConvertDLSInputToModulatorOperator(connectionBlock.Source);

            if (sf2Source == 0xFFFF)
                return;

            ConvertDLSDestinationToGeneratorOperator(connectionBlock, dstOper, Amount);

            if (dstOper == GeneratorOperator::Invalid)
                return;
        }
    }

    // Determine the modulator source amount.
    {
        sf2SourceAmt = ConvertDLSInputToModulatorOperator(connectionBlock.Control);

        if (sf2SourceAmt == 0xFFFF)
            return;
    }

    if (sf2Source == 0x0000)
    {
        srcOper = 0x0000;
    }
    else
    {
        // The Source Transform in section 2.10, Table 9 and 10 maps to a SF2 curve type. The Output Transform is ignored unless the curve type of the source is linear.
        uint16_t srcOperTransform = connectionBlock.Transform & 0x3C00;;

        if (srcOperTransform == 0)
            srcOperTransform = connectionBlock.Transform & 0x000F;

        uint16_t srcOperIsBipolar  = connectionBlock.Transform & 0x4000;
        uint16_t srcOperIsInverted = connectionBlock.Transform & 0x8000;

        // Special case: for attenuation, invert source. DLS gain is the opposite of SF2 attenuation.
        if (dstOper == GeneratorOperator::initialAttenuation)
        {
            if (Value < 0)
                srcOperIsInverted = 0x8000;
        }

        srcOper = srcOperIsInverted | srcOperIsBipolar | srcOperTransform | sf2Source;
    }

    if (dstOper == GeneratorOperator::initialAttenuation)
        Amount = std::clamp(Amount, (int16_t) 0, (int16_t) 1440);

    uint16_t srcOperAmtTransform  = connectionBlock.Transform & 0x00F0;
    uint16_t srcOperAmtIsBipolar  = connectionBlock.Transform & 0x0100;
    uint16_t srcOperAmtIsInverted = connectionBlock.Transform & 0x0200;

    ModulatorOperator srcOperAmt = srcOperAmtIsInverted | srcOperAmtIsBipolar | srcOperAmtTransform | sf2SourceAmt;

    if (SwapSources)
        std::swap(srcOper, srcOperAmt);

    modulators.push_back(modulator_t(srcOper, dstOper, Amount, srcOperAmt, 0x0000));
}

/// <summary>
/// Gets a Generator Operator for specific DLS source and destination combinations.
/// </summary>
GeneratorOperator bank_t::GetSpecialGeneratorOperator(const dls::connection_block_t & connectionBlock) noexcept
{
    if (connectionBlock.Source == CONN_SRC_LFO)
    {
        if (connectionBlock.Destination == CONN_DST_GAIN)
            return GeneratorOperator::modLfoToVolume;

        if (connectionBlock.Destination == CONN_DST_PITCH)
            return GeneratorOperator::modLfoToPitch;

        if (connectionBlock.Destination == CONN_DST_FILTER_CUTOFF)
            return GeneratorOperator::modLfoToFilterFc;
    }

    if (connectionBlock.Source == CONN_SRC_EG2)
    {
        if (connectionBlock.Destination == CONN_DST_PITCH)
            return GeneratorOperator::modEnvToPitch;

        if (connectionBlock.Destination == CONN_DST_FILTER_CUTOFF)
            return GeneratorOperator::modEnvToFilterFc;
    }

    if (connectionBlock.Source == CONN_SRC_VIBRATO)
    {
        if (connectionBlock.Destination == CONN_DST_PITCH)
            return GeneratorOperator::vibLfoToPitch;
    }

    return GeneratorOperator::Invalid;
}

/// <summary>
/// Converts a DLS input to an SF2 Modulator Operator.
/// </summary>
ModulatorOperator bank_t::ConvertDLSInputToModulatorOperator(uint16_t input) noexcept
{
    switch (input)
    {
        default:
        case CONN_SRC_LFO:
        case CONN_SRC_EG2:
        case CONN_SRC_VIBRATO:
        case CONN_SRC_RPN1:                                        // Fine Tune
        case CONN_SRC_RPN2:                                        // Coarse Tune
            return 0xFFFF; break;

        case CONN_SRC_NONE:             return           0; break; // No Controller
        case CONN_SRC_KEYONVELOCITY:    return           2; break; // Note-On Velocity
        case CONN_SRC_KEYNUMBER:        return           3; break; // Note-On Key Number
        case CONN_SRC_POLYPRESSURE:     return          10; break; // Poly Pressure
        case CONN_SRC_CHANNELPRESSURE:  return          13; break; // Channel Pressure
        case CONN_SRC_PITCHWHEEL:       return          14; break; // Pitch Wheel
        case CONN_SRC_RPN0:             return          16; break; // Pitch Wheel Sensitivity

        case CONN_SRC_CC1:              return 0x0080 |  1; break; // Modulation
        case CONN_SRC_CC7:              return 0x0080 |  7; break; // Channel Volume
        case CONN_SRC_CC10:             return 0x0080 | 10; break; // Pan
        case CONN_SRC_CC11:             return 0x0080 | 11; break; // Expression
        case CONN_SRC_CC91:             return 0x0080 | 91; break; // Chorus Send
        case CONN_SRC_CC93:             return 0x0080 | 93; break; // Reverb Send
    }
}

/// <summary>
/// Converts a DLS destination to an SF2 Generator Operator.
/// </summary>
void bank_t::ConvertDLSDestinationToGeneratorOperator(const dls::connection_block_t & connectionBlock, GeneratorOperator & dstOperator, int16_t & dstAmount) noexcept
{
    dstAmount = (int16_t) (connectionBlock.Scale >> 16);

    switch (connectionBlock.Destination)
    {
        default:

        case CONN_DST_NONE:             dstOperator = GeneratorOperator::Invalid; break;

        case CONN_DST_GAIN:             dstOperator = GeneratorOperator::initialAttenuation; dstAmount = -dstAmount; break;
        case CONN_DST_PITCH:            dstOperator = GeneratorOperator::fineTune; break;
        case CONN_DST_PAN:              dstOperator = GeneratorOperator::pan; break;
        case CONN_DST_KEYNUMBER:        dstOperator = GeneratorOperator::overridingRootKey; break;

        // Volume Envelope
        case CONN_DST_EG1_DELAYTIME:    dstOperator = GeneratorOperator::delayVolEnv; break;
        case CONN_DST_EG1_ATTACKTIME:   dstOperator = GeneratorOperator::attackVolEnv; break;
        case CONN_DST_EG1_HOLDTIME:     dstOperator = GeneratorOperator::holdVolEnv; break;
        case CONN_DST_EG1_DECAYTIME:    dstOperator = GeneratorOperator::decayVolEnv; break;
        case CONN_DST_EG1_SUSTAINLEVEL: dstOperator = GeneratorOperator::sustainVolEnv; dstAmount = 1000 - dstAmount; break;
        case CONN_DST_EG1_RELEASETIME:  dstOperator = GeneratorOperator::releaseVolEnv; break;

        // Modulation Envelope
        case CONN_DST_EG2_DELAYTIME:    dstOperator = GeneratorOperator::delayModEnv; break;
        case CONN_DST_EG2_ATTACKTIME:   dstOperator = GeneratorOperator::attackModEnv; break;
        case CONN_DST_EG2_HOLDTIME:     dstOperator = GeneratorOperator::holdModEnv; break;
        case CONN_DST_EG2_DECAYTIME:    dstOperator = GeneratorOperator::decayModEnv; break;
        case CONN_DST_EG2_SUSTAINLEVEL: dstOperator = GeneratorOperator::sustainModEnv, dstAmount = (1000 - dstAmount) / 10; break;
        case CONN_DST_EG2_RELEASETIME:  dstOperator = GeneratorOperator::releaseModEnv; break;

        case CONN_DST_FILTER_CUTOFF:    dstOperator = GeneratorOperator::initialFilterFc; break;
        case CONN_DST_FILTER_Q:         dstOperator = GeneratorOperator::initialFilterQ; break;

        case CONN_DST_CHORUS:           dstOperator = GeneratorOperator::chorusEffectsSend; break;
        case CONN_DST_REVERB:           dstOperator = GeneratorOperator::reverbEffectsSend; break;

        // LFO
        case CONN_DST_LFO_FREQUENCY:    dstOperator = GeneratorOperator::freqModLFO; break;
        case CONN_DST_LFO_STARTDELAY:   dstOperator = GeneratorOperator::delayModLFO; break;

        case CONN_DST_VIB_FREQUENCY:    dstOperator = GeneratorOperator::freqVibLFO; break;
        case CONN_DST_VIB_STARTDELAY:   dstOperator = GeneratorOperator::delayVibLFO; break;
    }
}
