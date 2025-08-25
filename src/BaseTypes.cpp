
/** $VER: BaseTypes.cpp (2025.08.24) P. Stuer - Base types for sound font handling **/

#include "pch.h"

#include "libsf.h"

#include <functional>

#include "BaseTypes.h"

using namespace sf;

/// <summary>
/// Handles an Ixxx chunk.
/// </summary>
bool soundfont_reader_base_t::HandleIxxx(uint32_t chunkId, uint32_t chunkSize, properties_t & properties)
{
    std::string Value;

    Value.resize((size_t) chunkSize);

    Read((void *) Value.data(), chunkSize);

    // Remove trailing zeroes.
    Value.erase(std::find_if(Value.rbegin(), Value.rend(), [](char ch) { return ch != '\0'; }).base(), Value.end());

    properties.push_back(property_t(chunkId, Value));

    return true;
}

// SoundFont 2.04 Technical Specification, 1996, Section 8.1.3 Generator Summary
const std::map<GeneratorOperator, generator_limit_t> sf::GeneratorLimits =
{
    { GeneratorOperator::startAddrsOffset,              {      0, 32768,      0 } }, // Samples
    { GeneratorOperator::endAddrsOffset,                { -32768, 32768,      0 } }, // Samples
    { GeneratorOperator::startAddrsCoarseOffset,        {      0, 32768,      0 } }, // 32768 Samples
    { GeneratorOperator::endAddrsCoarseOffset,          { -32768, 32768,      0 } }, // 32768 Samples

    { GeneratorOperator::startloopAddrsOffset,          { -32768, 32768,      0 } }, // Samples
    { GeneratorOperator::endloopAddrsOffset,            { -32768, 32768,      0 } }, // Samples
    { GeneratorOperator::startloopAddrsCoarseOffset,    { -32768, 32768,      0 } }, // 32768 Samples
    { GeneratorOperator::endloopAddrsCoarseOffset,      { -32768, 32768,      0 } }, // 32768 Samples

    // Pitch influence
    { GeneratorOperator::modLfoToPitch,                 { -12000, 12000,      0 } }, // cent fs
    { GeneratorOperator::vibLfoToPitch,                 { -12000, 12000,      0 } }, // cent fs
    { GeneratorOperator::modEnvToPitch,                 { -12000, 12000,      0 } }, // cent fs

    // Lowpass filter
    { GeneratorOperator::initialFilterFc,               {   1500, 13500,  13500 } }, // cent, 20 Hz - 20 kHz
    { GeneratorOperator::initialFilterQ,                {      0,   960,      0 } }, // cB, 0 dB - 96 dB

    { GeneratorOperator::modLfoToFilterFc,              { -12000, 12000,      0 } }, // cent fs, -10 oct - 10 oct
    { GeneratorOperator::modEnvToFilterFc,              { -12000, 12000,      0 } }, // cent fs, -10 oct - 10 oct

    { GeneratorOperator::modLfoToVolume,                {   -960,   960,      0 } }, // cB fs, -96 dB - 96 dB

    { GeneratorOperator::chorusEffectsSend,             {      0,  1000,      0 } }, // 0.1%, 0% - 100%
    { GeneratorOperator::reverbEffectsSend,             {      0,  1000,      0 } }, // 0.1%, 0% - 100%
    { GeneratorOperator::pan,                           {   -500,   500,      0 } }, // 0.1%, Left - Right, 0 = Center

    { GeneratorOperator::delayModLFO,                   { -12000,  5000, -12000 } }, // timecent, 1 ms - 20 s, 0 = 1 s
    { GeneratorOperator::freqModLFO,                    { -16000,  4500,      0 } }, // cent, 1 mHz - 100 Hz, 0 = 8.176 Hz
    { GeneratorOperator::delayVibLFO,                   { -12000,  5000, -12000 } }, // timecent, 1 ms - 20 s, 0 = 1 s
    { GeneratorOperator::freqVibLFO,                    { -16000,  4500,      0 } }, // cent, 1 mHz - 100 Hz, 0 = 8.176 Hz

    { GeneratorOperator::delayModEnv,                   { -12000,  5000, -12000 } }, // timecent, 1 ms - 20 s
    { GeneratorOperator::attackModEnv,                  { -12000,  8000, -12000 } }, // timecent, 1 ms - 100 s
    { GeneratorOperator::holdModEnv,                    { -12000,  5000, -12000 } }, // timecent, 1 ms - 20 s
    { GeneratorOperator::decayModEnv,                   { -12000,  8000, -12000 } }, // timecent, 1 ms - 100 s
    { GeneratorOperator::sustainModEnv,                 {      0,  1000,      0 } }, // -0.1%, 100% - 0%
    { GeneratorOperator::releaseModEnv,                 { -12000,  8000, -12000 } }, // timecent, 1 ms - 100 s

    { GeneratorOperator::keynumToModEnvHold,            {  -1200,  1200,      0 } }, // tcent/key, -oct/ky - oct/key
    { GeneratorOperator::keynumToModEnvDecay,           {  -1200,  1200,      0 } }, // tcent/key, -oct/ky - oct/key

    { GeneratorOperator::delayVolEnv,                   { -12000,  5000, -12000 } }, // timecent, 1 ms - 20 s
    { GeneratorOperator::attackVolEnv,                  { -12000,  8000, -12000 } }, // timecent, 1 ms - 100 s
    { GeneratorOperator::holdVolEnv,                    { -12000,  5000, -12000 } }, // timecent, 1 ms - 20 s
    { GeneratorOperator::decayVolEnv,                   { -12000,  8000, -12000 } }, // timecent, 1 ms - 100 s
    { GeneratorOperator::sustainVolEnv,                 {      0,  1440,      0 } }, // cB attn, 0 db - 144 dB
    { GeneratorOperator::releaseVolEnv,                 { -12000,  8000, -12000 } }, // timecent, 1 ms - 100 s

    { GeneratorOperator::keynumToVolEnvHold,            {  -1200,  1200,      0 } }, // tcent/key, -oct/ky - oct/key
    { GeneratorOperator::keynumToVolEnvDecay,           {  -1200,  1200,      0 } }, // tcent/key, -oct/ky - oct/key

    { GeneratorOperator::keyNum,                        {      0,   127,     -1 } }, // MIDI key
    { GeneratorOperator::velocity,                      {      0,   127,     -1 } }, // MIDI vel

    { GeneratorOperator::initialAttenuation,            {      0,  1440,      0 } }, // cB, 0 dB - 144 dB

    { GeneratorOperator::coarseTune,                    {   -120,   120,      0 } }, // semitone, -10 oct - 10 oct
    { GeneratorOperator::fineTune,                      {    -99,    99,      0 } }, // cent, -99 cent - 99 cent

    { GeneratorOperator::sampleModes,                   {      0,     3,      0 } }, // Flags
    { GeneratorOperator::scaleTuning,                   {      0,  1200,    100 } }, // cent/key, None - oct/key
    { GeneratorOperator::exclusiveClass,                {      1,   127,      0 } },
    { GeneratorOperator::overridingRootKey,             {      0,   127,     -1 } }, // MIDI key
};
