
/** $VER: BaseTypes.h (2025.09.07) P. Stuer - Base types for soundfont handling **/

#pragma once

#include <map>
#include <string>
#include <vector>
#include <unordered_map>

#include <libriff.h>

namespace sf
{
#pragma warning(disable: 4820) // x bytes padding

// SoundFont 2.04 Technical Specification, 1996, Section 8.1.2 Generator Enumerators defined
enum GeneratorOperator : uint16_t
{
    Invalid = 0xFFFF,                   // Invalid generator

    startAddrsOffset = 0,               // Offset, in sample data points, beyond the Start sample header parameter to the first sample data point to be played for this instrument.
    endAddrsOffset = 1,                 // Offset, in sample data points, beyond the End sample header parameter to the last sample data point to be played for this instrument. 
    startloopAddrsOffset = 2,           // Offset, in sample data points, beyond the Startloop sample header parameter to the first sample data point to be repeated in the loop for this instrument.
    endloopAddrsOffset = 3,             // Offset, in sample data points, beyond the Endloop sample header parameter to the sample data point considered equivalent to the Startloop sample data point for the loop for this instrument.
    startAddrsCoarseOffset = 4,         // Offset, in 32768 sample data point increments beyond the Start sample header parameter and the first sample data point to be played in this instrument.

    modLfoToPitch = 5,                  // Degree, in cents, to which a full scale excursion of the Modulation LFO will influence pitch. A positive value indicates a positive LFO excursion increases pitch; a negative value indicates a positive excursion decreases pitch.
    vibLfoToPitch = 6,                  // Degree, in cents, to which a full scale excursion of the Vibrato LFO will influence pitch. A positive value indicates a positive LFO excursion increases pitch; a negative value indicates a positive excursion decreases pitch.degree, in cents, to which a full scale excursion of the Vibrato LFO will influence pitch. A positive value indicates a positive LFO excursion increases pitch; a negative value indicates a positive excursion decreases pitch.
    modEnvToPitch = 7,                  // Degree, in cents, to which a full scale excursion of the Modulation Envelope will influence pitch. A positive value indicates an increase in pitch; a negative value indicates a decrease in pitch.

    initialFilterFc = 8,                // Cutoff and resonant frequency of the lowpass filter in absolute cent units.
    initialFilterQ = 9,                 // Height above DC gain in centibels which the filter resonance exhibits at the cutoff frequency.

    modLfoToFilterFc = 10,              // filter modulation - modulation lfo lowpass filter cutoff in cents
    modEnvToFilterFc = 11,              // filter modulation - modulation envelope lowpass filter cutoff in cents
    endAddrsCoarseOffset = 12,          // ample control - move sample end point in 32,768 increments
    modLfoToVolume = 13,                // modulation lfo - volume (tremolo), where 100 = 10dB

    unused1 = 14,                       // Unused

    chorusEffectsSend = 15,             // effect send - how much is sent to chorus (0 - 1000)
    reverbEffectsSend = 16,             // effect send - how much is sent to reverb (0 - 1000)
    pan = 17,                           // panning - where -500 = left, 0 = center, 500 = right

    unused2 = 18,                       // Unused
    unused3 = 19,                       // Unused
    unused4 = 20,                       // Unused

    delayModLFO = 21,                   // mod lfo - delay for mod lfo to start from zero
    freqModLFO = 22,                    // mod lfo - frequency of mod lfo, 0 = 8.176 Hz, units = f => 1200log2(f/8.176)
    delayVibLFO = 23,                   // vib lfo - delay for vibrato lfo to start from zero
    freqVibLFO = 24,                    // vib lfo - frequency of vibrato lfo, 0 = 8.176Hz, unit = f => 1200log2(f/8.176)

    delayModEnv = 25,                   // mod env - 0 = 1 s decay till mod env starts
    attackModEnv = 26,                  // mod env - attack of mod env
    holdModEnv = 27,                    // mod env - hold of mod env
    decayModEnv = 28,                   // mod env - decay of mod env
    sustainModEnv = 29,                 // mod env - sustain of mod env
    releaseModEnv = 30,                 // mod env - release of mod env

    keynumToModEnvHold = 31,            // mod env - also modulating mod envelope hold with key number
    keynumToModEnvDecay = 32,           // mod env - also modulating mod envelope decay with key number

    delayVolEnv = 33,                   // vol env - delay of envelope from zero (weird scale)
    attackVolEnv = 34,                  // vol env - attack of envelope
    holdVolEnv = 35,                    // vol env - hold of envelope
    decayVolEnv = 36,                   // vol env - decay of envelope
    sustainVolEnv = 37,                 // vol env - sustain of envelope
    releaseVolEnv = 38,                 // vol env - release of envelope

    keynumToVolEnvHold = 39,            // vol env - key number to volume envelope hold
    keynumToVolEnvDecay = 40,           // vol env - key number to volume envelope decay

    instrument = 41,                    // zone - instrument index to use for preset zone

    reserved1 = 42,                     // Reserved

    keyRange = 43,                      // zone - key range for which preset / instrument zone is active
    velRange = 44,                      // zone - velocity range for which preset / instrument zone is active
    startloopAddrsCoarseOffset = 45,    // sample control - moves sample loop start point in 32,768 increments
    keyNum = 46,                        // zone - instrument only = always use this midi number (ignore what's pressed)
    velocity = 47,                      // zone - instrument only = always use this velocity (ignore what's pressed)
    initialAttenuation = 48,            // Attenuation in centibels. A value of 60 indicates the note will be played at 6 dB below full scale for the note.

    reserved2 = 49,                     // Reserved

    endloopAddrsCoarseOffset = 50,      // sample control - moves sample loop end point in 32,768 increments
    coarseTune = 51,                    // tune - pitch offset in semitones
    fineTune = 52,                      // tune - pitch offset in cents
    sampleID = 53,                      // sample - instrument zone only = which sample to use
    sampleModes = 54,                   // sample - 0 = no loop, 1 = loop, 2 = reserved, 3 = loop and play till the end in release phase

    reserved3 = 55,                     // Reserved

    scaleTuning = 56,                   // sample - the degree to which MIDI key number influences pitch, 100 = default
    exclusiveClass = 57,                // sample - = cut = choke group
    overridingRootKey = 58,             // sample - can override the sample's original pitch

    unused5 = 59,                       // Unused

    endOper = 60                        // Unused
};

struct generator_limit_t
{
    int Min;
    int Max;
    int Default;
};

extern const std::map<GeneratorOperator, generator_limit_t> GeneratorLimits;

struct property_t
{
    uint32_t Id;
    std::string Value;
};

typedef std::vector<property_t> properties_t;

/// <summary>
/// Gets the value of a property with the specified id.
/// </summary>
inline std::string GetPropertyValue(const properties_t & properties, const uint32_t id)
{
    auto it = std::find_if(properties.begin(), properties.end(), [id](property_t p)
    {
        return p.Id == id;
    });

    return (it != properties.end()) ? it->Value : "";
}

class soundfont_reader_base_t : public riff::reader_t
{
public:
    bool HandleIxxx(uint32_t chunkId, uint32_t chunkSize, properties_t & infoMap);

private:
    properties_t _Properties;
};

class soundfont_writer_base_t : public riff::writer_t
{
};

}
