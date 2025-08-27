
/** $VER: SoundFont.cpp (2025.08.27) P. Stuer **/

#include "pch.h"

#include "libsf.h"

#include "SF2.h"

using namespace sf;

static void ApplyKeyNumToCorrection(std::vector<sf::generator_t> & generators, int16_t value, GeneratorOperator keynumToOperator, GeneratorOperator realOperator);

/// <summary>
/// Initializes a SoundFont bank from a DLS collection.
/// </summary>
void bank_t::ConvertFrom(const dls::collection_t & collection)
{
    Major       = 2;
    Minor       = 4;
    SoundEngine = "E-mu 10K1";
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
                    const std::string PresetName = !Instrument.Name.empty() ? Instrument.Name : FormatText("Preset %d-%d", Bank, Instrument.Program);

                    Presets.push_back(sf::preset_t(PresetName, Instrument.Program, Bank, (uint16_t) PresetZones.size()));

                    {
                        // Add a global preset zone. FIXME: Is this really necessary? We're not adding any generators.
                        PresetZones.push_back(sf::preset_zone_t((uint16_t) PresetGenerators.size(), (uint16_t) PresetModulators.size()));

                        // Add a local zone.
                        PresetZones.push_back(sf::preset_zone_t((uint16_t) PresetGenerators.size(), (uint16_t) PresetModulators.size()));

                        PresetGenerators.push_back(sf::generator_t(GeneratorOperator::instrument, (uint16_t) Instruments.size()));
                    }
                }

                const std::string InstrumentName = !Instrument.Name.empty() ? Instrument.Name : FormatText("Instrument %d-%d", Bank, Instrument.Program);

                Instruments.push_back(sf::instrument_t(InstrumentName, (uint16_t) InstrumentZones.size()));

                // Convert the instrument articulators, if any.
                if (!Instrument.Articulators.empty())
                {
                    std::vector<generator_t> Generators;
                    std::vector<modulator_t> Modulators;

                    ConvertArticulators(Instrument.Articulators, Generators, Modulators);

                    // Add a global instrument zone.
                    InstrumentZones.push_back(instrument_zone_t((uint16_t) InstrumentGenerators.size(), (uint16_t) InstrumentModulators.size()));

                    InstrumentGenerators.insert(InstrumentGenerators.end(), Generators.begin(), Generators.end());
                    InstrumentModulators.insert(InstrumentModulators.end(), Modulators.begin(), Modulators.end());
                }

                // Conver the regions to zones.
                for (const auto & Region : Instrument.Regions)
                {
                    InstrumentZones.push_back(instrument_zone_t((uint16_t) InstrumentGenerators.size(), (uint16_t) InstrumentModulators.size()));

                    InstrumentGenerators.push_back(sf::generator_t(GeneratorOperator::keyRange, MAKEWORD(Region.LowKey,      Region.HighKey)));         // Must be the first generator.
                    InstrumentGenerators.push_back(sf::generator_t(GeneratorOperator::velRange, MAKEWORD(Region.LowVelocity, Region.HighVelocity)));    // Must only be preceded by keyRange.

                    // Convert the region articulators, if any.
                    if (!Region.Articulators.empty())
                    {
                        std::vector<generator_t> Generators;
                        std::vector<modulator_t> Modulators;

                        ConvertArticulators(Region.Articulators, Generators, Modulators);

                        InstrumentGenerators.insert(InstrumentGenerators.end(), Generators.begin(), Generators.end());
                        InstrumentModulators.insert(InstrumentModulators.end(), Modulators.begin(), Modulators.end());
                    }

                    // dls.Cues[CueIndex] is actually an offset in the wave pool but we can use the cue index as an index into the sample list.
                    const uint16_t SampleID = (uint16_t) Region.WaveLink.CueIndex;

                    const auto & Wave = collection.Waves[SampleID];

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
            
                        InstrumentGenerators.push_back(sf::generator_t(GeneratorOperator::sampleModes, SampleMode));
                    }

                    // Add an Initial Attenuation generator.
                    {
                        // Convert DLS gain from 1/655360 dB units to SF2 centibels with EMU correction (0.4).
                        const double Attenuation = ((Wave.WaveSample.Gain / -655360.) * 10.) / 0.4;

                        InstrumentGenerators.push_back(sf::generator_t(GeneratorOperator::initialAttenuation, (uint16_t) Attenuation));
                    }

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
            // Calculate the size of the sample data buffer.
            {
                size_t Size = 0;

                for (const auto & wave : collection.Waves)
                {
                    // FIXME: Support stereo samples
                    if (wave.Channels != 1)
                        continue;

                    Size += wave.Data.size();
                }

                SampleData.resize(Size);
            }

            size_t Offset = 0;

            for (const auto & wave : collection.Waves)
            {
                // FIXME: Support stereo samples
                if (wave.Channels != 1)
                    continue;

                const auto BytesPerSample = wave.BitsPerSample >> 3;

                sf::sample_t Sample =
                {
                    .Name            = wave.Name,

                    .Start           = (uint32_t) ( Offset                     / BytesPerSample),
                    .End             = (uint32_t) ((Offset + wave.Data.size()) / BytesPerSample),

                    .SampleRate      = wave.SamplesPerSec,
                    .Pitch           = (uint8_t) wave.WaveSample.UnityNote,
                    .PitchCorrection =  (int8_t) wave.WaveSample.FineTune,

                    .SampleType      = sf::SampleTypes::MonoSample
                };

                std::memcpy(SampleData.data() + Offset, wave.Data.data(), wave.Data.size());
                Offset += wave.Data.size();

                if (wave.WaveSample.Loops.size() != 0)
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
                            generators.push_back(sf::generator_t(GeneratorOperator::delayVibLFO, (uint16_t) Amount));
                            break;
                        }

                        // Volume Envelope Generator Destinations
                        case CONN_DST_EG1_ATTACKTIME:
                        {
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
                            generators.push_back(sf::generator_t(GeneratorOperator::delayVolEnv, (uint16_t) Amount));
                            break;
                        }

                        case CONN_DST_EG1_HOLDTIME:
                        {
                            generators.push_back(sf::generator_t(GeneratorOperator::holdVolEnv, (uint16_t) Amount));
                            break;
                        }

                        // Modulation Envelope Generator Destinations
                        case CONN_DST_EG2_ATTACKTIME:
                        {
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
                            generators.push_back(sf::generator_t(GeneratorOperator::delayModEnv, (uint16_t) Amount));
                            break;
                        }

                        case CONN_DST_EG2_HOLDTIME:
                        {
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
    int16_t Value = (int16_t) (connectionBlock.Scale >> 16);
    int16_t Amount = Value;

    ModulatorOperator srcOper = 0xFFFF;

    ModulatorOperator sf2Source = 0xFFFF;
    ModulatorOperator sf2SourceAmt;

    bool SwapSources          = false;
    bool IsSourceNoController = false;

    auto dstOper = GeneratorOperator::Invalid;

    {
        const GeneratorOperator specialDestination = GetSpecialGeneratorOperator(connectionBlock);

        if (specialDestination != GeneratorOperator::Invalid)
        {
            sf2Source = 0;
            dstOper = specialDestination;

            SwapSources          = true;
            IsSourceNoController = true;
        }
        else
        {
            ConvertDLSSourceToModulatorOperator(connectionBlock.Source, sf2Source);

            if (sf2Source == 0xFFFF)
                return;

            auto sf2GenDestination = GeneratorOperator::Invalid;

            ConvertDLSDestinationToGeneratorOperator(connectionBlock, sf2GenDestination, Value);

            if (sf2GenDestination == GeneratorOperator::Invalid)
                return;

            dstOper = sf2GenDestination;

            if (Value != connectionBlock.Scale)
                Amount = Value;
        }
    }

    ConvertDLSSourceToModulatorOperator(connectionBlock.Control, sf2SourceAmt);

    if (sf2SourceAmt == 0xFFFF)
        return;

    if (IsSourceNoController)
    {
        srcOper = 0;
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
        Amount = Clamp(Amount, (int16_t) 0, (int16_t) 1440);

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
    if (connectionBlock.Source == CONN_SRC_VIBRATO && connectionBlock.Destination == CONN_DST_PITCH)
        return GeneratorOperator::vibLfoToPitch;

    if (connectionBlock.Source == CONN_SRC_LFO)
    {
        if (connectionBlock.Destination == CONN_DST_PITCH)
            return GeneratorOperator::modLfoToPitch;

        if (connectionBlock.Destination == CONN_DST_FILTER_CUTOFF)
            return GeneratorOperator::modLfoToFilterFc;

        if (connectionBlock.Destination == CONN_DST_GAIN)
            return GeneratorOperator::modLfoToVolume;
    }

    if (connectionBlock.Source == CONN_SRC_EG2)
    {
        if (connectionBlock.Destination == CONN_DST_FILTER_CUTOFF)
            return GeneratorOperator::modEnvToFilterFc;

        if (connectionBlock.Destination == CONN_DST_PITCH)
            return GeneratorOperator::modEnvToPitch;
    }

    return GeneratorOperator::Invalid;
}

/// <summary>
/// Converts a DLS source to an SF2 Modulator Operator.
/// </summary>
void bank_t::ConvertDLSSourceToModulatorOperator(uint16_t source, ModulatorOperator & modulatorOperator) noexcept
{
    switch (source)
    {
        default:
        case CONN_SRC_LFO:
        case CONN_SRC_EG2:
        case CONN_SRC_VIBRATO:
        case CONN_SRC_RPN1:                                                     // Fine Tune
        case CONN_SRC_RPN2:                                                     // Coarse Tune
            modulatorOperator = 0xFFFF; break;

        case CONN_SRC_NONE:             modulatorOperator =           0; break; // No Controller
        case CONN_SRC_KEYONVELOCITY:    modulatorOperator =           2; break; // Note-On Velocity
        case CONN_SRC_KEYNUMBER:        modulatorOperator =           3; break; // Note-On Key Number
        case CONN_SRC_POLYPRESSURE:     modulatorOperator =          10; break; // Poly Pressure
        case CONN_SRC_CHANNELPRESSURE:  modulatorOperator =          13; break; // Channel Pressure
        case CONN_SRC_PITCHWHEEL:       modulatorOperator =          14; break; // Pitch Wheel
        case CONN_SRC_RPN0:             modulatorOperator =          16; break; // Pitch Wheel Sensitivity

        case CONN_SRC_CC1:              modulatorOperator = 0x0080 |  1; break; // Modulation
        case CONN_SRC_CC7:              modulatorOperator = 0x0080 |  7; break; // Channel Volume
        case CONN_SRC_CC10:             modulatorOperator = 0x0080 | 10; break; // Pan
        case CONN_SRC_CC11:             modulatorOperator = 0x0080 | 11; break; // Expression
        case CONN_SRC_CC91:             modulatorOperator = 0x0080 | 91; break; // Chorus Send
        case CONN_SRC_CC93:             modulatorOperator = 0x0080 | 93; break; // Reverb Send
    }
}

/// <summary>
/// Converts a DLS destination to an SF2 Generator Operator.
/// </summary>
void bank_t::ConvertDLSDestinationToGeneratorOperator(const dls::connection_block_t & connectionBlock, GeneratorOperator & dstOperator, int16_t dstAmount) noexcept
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
