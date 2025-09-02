
# libsf

[libsf](https://github.com/stuerp/libsf) is a library for reading and writing soundfonts.

* Read and write SoundFont 1.0 and 2.x banks.
* Read DLS collections.
* Read ECW wavesets. (work in progress)
* Convert DLS collections into SF2 banks.

WARNING: This is very tightly coupled with foo_midi. The API will change several times before it becomes stable.

## ECW

".ECW file" or "waveset" refers to a file with an extension of ".ECW" which stores articulation and sample data used
by the MIDI synthesizer of sound cards equipped with an ES1370, ES1371, or ES1373 chip.

These sound cards include the Ensoniq AudioPCI, the Sound Blaster! AudioPCI, the Sound Blaster! PCI64, the Sound Blaster! PCI128, and the Sound Blaster Live!.

## References

* [IFF File Format Summary](https://www.fileformat.info/format/iff/egff.htm)
* [IFF FORM and Chunk Registry](https://wiki.amigaos.net/wiki/IFF_FORM_and_Chunk_Registry)
* [Audio File Format Specifications](https://www.mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html)

* [Microsoft RIFF File Format Summary](https://www.fileformat.info/format/riff/egff.htm)

* SoundFont Technical Specification 2.04, Creative/E-mu Systems, 10 September 2002
* SoundFont 2.1 Application Note, Creative/E-mu Systems, 12 August 1998
* Downloadable Sounds Level 2.2 v1.0, MIDI Manufacturers Association, April 2006

## Acknowledgements

* John N. Engelmann for reverse engineering the [.ECW file format](http://www.johnnengelmann.com/technology/ecw/index.php).
* [Greg Kennedy](https://greg-kennedy.com/) for [ecw2sfz](https://sourceforge.net/projects/ecw2sfz/).
* [Spessasus](https://github.com/spessasus) for advice, testing, help, [SpessaSynth](https://spessasus.github.io/SpessaSynth/) and [SpessaFont](https://spessasus.github.io/SpessaFont/).
