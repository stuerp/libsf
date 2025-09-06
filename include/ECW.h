
/** $VER: ECW.h (2025.05.01) P. Stuer - ECW data types on disk. Based on ECW.txt and ECW Decompiler. **/

#pragma once

#include "pch.h"

#include "BaseTypes.h"

namespace ecw
{

#pragma pack(push, 1)

struct ecwHeader
{
    char        Id[4];                  // "ECLW"

    uint8_t     Padding1[4];

    uint32_t    OffsAllocation;         // File offset where allocation chunk starts

    uint32_t    Unknown1;               // 16

    char        Copyright[80];          // Copyright info
    char        Name[80];               // Waveset name
    char        FileName[256];
    char        Description[80];
    char        Information[1280];

    uint8_t     Padding2[4];

    uint32_t    BankMapOffs;            // File offset where bank map starts
    uint32_t    BankMapSize;            // Length in bytes of the bank map (always 256)
    uint32_t    BankMapCount;           // Number of bank maps (always 1)

    uint32_t    DrumNoteMapOffs;        // File offset where drum kit map starts
    uint32_t    DrumNoteMapSize;        // Length in bytes of the drum kit map (always 256)
    uint32_t    DrumNoteMapCount;       // Number of drum kit maps (always 1)

    uint32_t    MIDIPatchMapOffs;       // File offset where MIDI patch maps start
    uint32_t    MIDIPatchMapSize;       // Total length in bytes of all MIDI patch maps
    uint32_t    MIDIPatchMapCount;      // Number of MIDI patch maps

    uint32_t    DrumKitMapOffs;         // File offset where the drum kit maps start
    uint32_t    DrumKitMapSize;         // Total length in bytes of all drum kit maps
    uint32_t    DrumKitMapCount;        // Number of drum kit maps

    uint32_t    InstrumentHeaderOffs;   // File offset where instrument headers start
    uint32_t    InstrumentHeaderSize;   // Total length in bytes of all instrument headers (always number of instrument headers multiplied by 23)
    uint32_t    InstrumentHeaderCount;  // Number of instrument headers

    uint32_t    PatchHeaderOffs;        // File offset where patch headers start
    uint32_t    PatchHeaderSize;        // Total length in bytes of all patch headers (always number of patch headers multiplied by 76)
    uint32_t    PatchHeaderCount;       // Number of patch headers

    uint8_t     Padding3[4];

    uint32_t    Array1Offs;             // File offset where array 1 starts
    uint32_t    Array1Size;             // Size of array 1 (in bytes). Each item is 2 bytes.
    uint32_t    Array1Count;            // Number of items in array 1

    uint32_t    Array2Offs;             // File offset where array 2 starts
    uint32_t    Array2Size;             // Size of array 2 (in bytes). Each item is 2 bytes.
    uint32_t    Array2Count;            // Number of items in array 2

    uint32_t    Array3Offs;             // File offset where array 3 starts
    uint32_t    Array3Size;             // Size of array 3 (in bytes). Each item is 2 bytes.
    uint32_t    Array3Count;            // Number of items in array 3

    uint32_t    SampleHeaderOffs;       // File offset where sample headers start
    uint32_t    SampleHeaderSize;       // Size of sample header data (in bytes). Each item is 16 bytes.
    uint32_t    SampleHeaderCount;      // Number of sample headers

    uint8_t     Padding4[4];

    uint32_t    SampleDataOffs;         // File offset where sample waveform area begins
    uint32_t    SampleDataSize;         // Total length of sample waveform area (in bytes)
};

// The bank map sssigns one MIDI patch map to each of the 128 MIDI banks.
struct ecwBankMap
{
    uint16_t MIDIPatchMaps[128];        // MIDI patch map assigned to MIDI bank 0..127.
};

// The drum kit map assigns one drum note map to each of the 128 MIDI drum kits.
struct ecwDrumKitMap
{
    uint16_t DrumNoteMaps[128];         // Drum note map assigned to MIDI drum kit 0..127.
};

// Assigns one ECW instrument to each of the 128 MIDI patches.
// Each MIDI patch map corresponds to one or more MIDI banks. One MIDI patch map can be used by more than one MIDI bank; in fact, all of the MIDI banks can use the
// same MIDI patch map if desired (which would allow a smaller filesize and save system RAM). The number of MIDI patch maps is set in the .ECW file header.
// Each MIDI patch map is 256 bytes long.
// If there are multiple MIDI patch maps (according to the file header), then the MIDI patch maps must follow one another immediately with no gaps between.
struct ecwMIDIPatchMap
{
    uint16_t Instruments[128];          // Instrument assigned to MIDI patch note 0..127 when using a MIDI bank whose entry in the bank map is equal to 0..n-1.
};

// Assigns one ECW instrument to each of the 128 MIDI drum notes.
// Each drum note map corresponds to one or more MIDI drum kits. One drum note map can be used by more than one drum kit; in fact, all of the drum kits can use
// the same drum note map if desired (which would allow a smaller filesize and save system RAM). The number of drum note maps is set in the .ECW file header.
// Each drum note map is 256 bytes long.
// If there are multiple drum kit maps (according to the file header), then the drum kit maps must follow one another immediately with no gaps between.
struct ecwDrumNoteMap
{
    uint16_t Instruments[128];          // Instrument assigned to MIDI drum note 0..127 when using a drum kit whose entry in the drum kit map is equal to 0..n-1.
};

// Describes an instrument and its patches and certain properties such as tuning and pan. (23 bytes)
struct ecwInstrument
{
    uint8_t Type;
    uint8_t Data[22];
};

struct ecwInstrument_v1
{
    uint8_t Type;                       // Value 2

    uint8_t SubType;                    // 0: Use only the first instrument sub-header.
                                        // 1: Use both sub-headers simultaneously for each note played.
                                        // 2: A split point is used to select which of the sub-headers is used for a given note.
                                        // 3: Use only the second sub-header.
    uint8_t NoteThreshold;              // The note number above which only the second instrument sub-header is used; otherwise, only the first instrument sub-header is used.

    struct ecwSubHeader_v1
    {
        uint16_t PatchIndex;            // Patch assigned to this instrument sub-header 
        int8_t Amplitude;               // Amplitude and envelope steepness (signed)
        int8_t Pan;                     // 193 seems to be extreme left and 64 seems to be extreme right
        int8_t CoarseTune;              // In semitones (signed)
        int8_t FineTune;                // In increments of 1/256 of a semitone (signed)
        uint16_t Delay;                 // Delay before onset of note (in milliseconds)
        uint8_t Group;                  // For tremolo strings and most if not all drum kit percussion, has value of either 1 or 2.
        uint8_t Unknown;                // If, in two or more instrument sub-headers, the byte at offset 000ch has the same value, only one of the patches played by those instrument sub-headers can sound simultaneously. Examples of instruments where this is desirable include open/closed hi-hats and open/closed triangles.
    } SubHeaders[2];
};

struct ecwInstrument_v2
{
    uint8_t Type;                       // Value 255

    uint8_t Unknown;

    struct ecwSubHeader_v2
    {
        uint16_t InstrumentIndex;       // The number of another instrument.
        uint8_t NoteThreshold;          // When a MIDI note is played with a note number less than or equal to the value of this byte, use the instrument header specified by the Index.
    } SubHeaders[7];
};

// Describes a patch and certain properties.(76 bytes)
struct ecwPatch
{
    int8_t PitchEnvelopeLevel;          // Magnitude of pitch envelope (signed). Negative values cause pitch to fall rather than rise. A value of 0 effectively disables pitch envelope. (Signed)
    int8_t ModulationSensitivity;       // MIDI controller 1 (modulation) sensitivity
    int8_t Scale;                       // A value of 0 denotes a 12-tones-per-octave (i.e. chromatic) scale. A value of 1causes this patch to ignore the MIDI note number, so that every key on the keyboard is the same pitch. A value of 2 denotes a 24-tones-per-octave (i.e. quarter tone) scale.

    uint8_t Unknown1[8];

    uint16_t Array1Index;               // Slot in array assigned to this patch
    int8_t Detune;                      // Changes tuning slightly

    uint8_t Unknown2[2];

    int8_t SplitPointAdjust;            // Shifts the split points of the samples played by this patch

    uint8_t Unknown3[10];

    /** Pitch Envelope **/

    uint8_t PitchDestination;           // Causes pitch to change more rapidly upon note release when pitch envelope is enabled. May be the destination of the release phase of the pitch envelope.
    uint8_t PitchAttackDelay;           // Delay before pitch envelope enters attack phase
    uint8_t InitialPitch;               // Initial pitch for pitch envelope**

    uint8_t PitchAttackTime;            // Attack time for pitch envelope
    uint8_t PitchAttackLevel;           // Attack level for pitch envelope
    uint8_t PitchDecayTime;             // Decay time for pitch envelope
    uint8_t PitchDecayLevel;            // Decay level for pitch envelope
    uint8_t PitchSustainTime;           // Sustain time for pitch envelope
    uint8_t PitchSustainLevel;          // Sustain level for pitch envelope
    uint8_t PitchReleaseTime;           // Release time for pitch envelope

    uint8_t PitchInfluence1;            // Influence of MIDI note velocity on magnitude of pitch envelope

    uint8_t PitchUnknown1;              // Unknown (may be proportional to attack time of pitch envelope)

    uint8_t PitchInfluence2;            // Influence of MIDI note number on pitch envelope time (0=none; 127=huge)
    uint8_t PitchEnableRelease;         // When set to 0, the pitch envelope enters the release phase when a MIDI note-off command is received. When set to 1, pitch envelope never enters the release phase.

    uint8_t PitchUnknown2;

    /** The "wavetable envelope" transitions from one sample in a sample set to another. This is useful in creating evolving timbres without using long samples.
        This only seems to work with the "ELEC PIANO 2" (simulates decay of the upper partials) and "SAWTOOTH" (simulates a sweeping resonant filter)
        sample sets, however; I'm not sure why it doesn't do the same with any sample set. **/

    uint8_t WaveTableUnused;
    uint8_t WaveTableAttackDelay;       // Delay before wavetable envelope enters attack phase. Seems to play the final wavetable while in this pre-attack phase.***
    uint8_t InitialWaveTable;           // Initial wavetable for wavetable envelope

    uint8_t WaveTableAttackTime;        // Attack time for wavetable envelope
    uint8_t WaveTableAttackLevel;       // Attack level for wavetable envelope
    uint8_t WaveTableDecayTime;         // Decay time for wavetable envelope
    uint8_t WaveTableDecayLevel;        // Decay level for wavetable envelope
    uint8_t WaveTableSustainTime;       // Sustain time for wavetable envelope
    uint8_t WaveTableSusTainLevel;      // Sustain level for wavetable envelope
    uint8_t WaveTableReleaseTime;       // Release time for wavetable envelope

    uint8_t WaveTableInfluence1;        // Influence of MIDI note velocity on magnitude of wavetable envelope (Untested)

    uint8_t WaveTableUnknown1;

    uint8_t WaveTableInfluence2;        // Influence of MIDI note number on wavetable envelope time (Untested)
    uint8_t WaveTableEnableRelease;     // When set to 0, the wavetable envelope enters the release phase when a MIDI note-off command is received; when set to 1, wavetable envelope never enters the release phase (Untested)

    uint8_t WaveTableUnknown2;

    /** Amplitude Envelope **/

    uint8_t AmplitudeDestination;       // Possibly the destination of the release phase of the amplitude envelope.
    uint8_t AmplitudeUnused;
    uint8_t InitialAmplitude;           // Initial amplitude for amplitude envelope**

    uint8_t AmplitudeAttackTime;        // Attack time for amplitude envelope
    uint8_t AmplitudeAttackLevel;       // Attack level for amplitude envelope
    uint8_t AmplitudeDecayTime;         // Decay time for amplitude envelope
    uint8_t AmplitudeDecayLevel;        // Decay level for amplitude envelope
    uint8_t AmplitudeSustainTime;       // Sustain time for amplitude envelope
    uint8_t AmplitudeSustainLevel;      // Sustain level for amplitude envelope
    uint8_t AmplitudeReleaseTime;       // Release time for amplitude envelope

    uint8_t AmplitudeInfluence1;        // Influence of MIDI note velocity on magnitude of amplitude envelope.

    uint8_t AmplitudeUnknown1;          // Seems to affect amplitude attack.

    uint8_t AmplitudeInfluence2;        // Influence of MIDI note number on amplitude envelope time.
    uint8_t AmplitudeEnableRelease;     // When set to 0, the amplitude envelope enters the release phase when a MIDI note-off command is received. When set to 1, amplitude envelope never enters the release phase.

    uint8_t AmplitudeUnknown2;

    /** Pitch LFO **/

    uint8_t LFODepth;                   // Pitch LFO (i.e. vibrato) depth. A value above 0 will add pitch modulation to a patch even when MIDI controller 1 (modulation) is set to 0.
    uint8_t LFOSpeed;                   // Pitch LFO (i.e. vibrato) speed.
    uint8_t LFODelay;                   // Delay before pitch LFO (i.e. vibrato) reaches full depth.

    uint8_t Unknown11;
};

/** Describes a sample set. (22 bytes) **/
struct ecwSampleSet
{
    uint32_t SampleIndex;
    uint16_t Array1Index;               // Index into Array 1
    uint16_t Code;
    char Name[14];                      // Name of the sample set
};

/** Descibes a sample. (16 bytes) **/
struct ecwSample
{
    uint8_t SplitPoint;                 // Highest MIDI note number to use this sample for.
    uint8_t Flags;                      // Determines whether the sample is looped, whether to apply tuning adjustments from patch and instrument headers when the sample is used as part of a drum kit, and whether to shift loop points further into the sample waveform area.
                                        // When set to 0, no tuning adjustments are made when the sample is used in a drum kit.
                                        // 0 .. 1: Disables looping.
                                        // 2 or higher: Enable looping using the loop points specified in the sample header (see below).
                                        // Values of 129 or higher: Shift the loop points forward by an amount proportional to the value minus 128 (i.e. 129 will shift the loop points slightly, while 255 will shift them quite far into the sample waveform area).
                                        // Be careful when experimenting with values over 129 as it is very possible to play past the end of the sample waveform area, which may cause Windows to crash.
    int8_t FineTune;                    // In increments of 1/256 of a semitone (signed)
    int8_t CoarseTune;                  // In semitones (signed)

    // Sample waveform data can be shared by multiple sample headers.
    uint32_t SampleStart;               // Offset in sample waveform area where this sample begins, multiplied by 8.
    uint32_t LoopStart;                 // Offset in sample waveform area of this sample's loop point, multiplied by 8.
    uint32_t LoopEnd;                   // Offset in sample waveform area of this sample's end loop point, multiplied by 8 (fractional loop lengths permitted).
                                        // This is also where the sample ends when looping is disabled. If a sample plays beyond the end of the file, the operating system may lock up.
};

#pragma pack(pop)

}
