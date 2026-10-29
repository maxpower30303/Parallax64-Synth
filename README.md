# Parallax64 Synth

by Flix

A polyphonic subtractive synthesizer for Windows, built with [JUCE](https://juce.com). Runs as a **VST3 plug-in** and as a **standalone application** (WASAPI/ASIO).

## Features

- 8 voices, two anti-aliased oscillators (PolyBLEP) plus sub oscillator and noise, optional hard sync
- Unison (detuned, stereo-spread), Poly/Mono voice mode with last-note priority, glide, adjustable pitch-bend range
- Five filter modes: 12 dB lowpass, 24 dB transistor-ladder lowpass, highpass, an aggressive ladder variant (LAD), bandpass
- Two ADSR envelopes, LFO (cutoff / pitch / amp / pulse width), arpeggiator
- Effects: distortion, chorus, delay, reverb
- 48 factory presets (banks A-F, slots 1-8)
- User presets: SAVE stores the current sound under a suggested name (UserPreset1, UserPreset2, ...) and asks before overwriting; click the preset name at the top right to load them
- MIDI learn for knobs, faders and selectors; learned mappings are saved with the session/preset state

## Build (Windows)

Requirements: Visual Studio (C++20 toolchain) and CMake 3.22 or newer. JUCE 9.0.2 is downloaded automatically.

```
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target P64_Standalone
cmake --build build --config Release --target P64_VST3
```

Use the generator name of your installed Visual Studio version. To build against a local JUCE checkout instead of downloading it, pass `-DJUCE_DIR=<path to JUCE>` (or set the `JUCE_DIR` environment variable).

Results are placed in `build/P64_artefacts/Release/` (the plug-in and executable files are named "P64 Synthesizer"). ASIO support uses the headers bundled with JUCE; no separate ASIO SDK is required.

## License

Parallax64 Synth is free software, licensed under the **GNU Affero General Public License v3.0** (`AGPL-3.0-only`), see [LICENSE](LICENSE). This is the licence option under which JUCE 9 is used here.

Third-party components and their licences are listed in [THIRD_PARTY_NOTICES.txt](THIRD_PARTY_NOTICES.txt), which must be shipped together with binary releases. Anyone who distributes this software or a modified version must pass on the source code under the same licence.

The software is provided "as is", without warranty of any kind.

### Name and logo

The code is covered by the licence above. The names "Parallax64" and "Parallax64 Synth" and the logo identify the original project; please do not use them as the name of a modified or derived product.

"VST" is a trademark of Steinberg Media Technologies GmbH, registered in Europe and other countries. "ASIO" is a trademark and software of Steinberg Media Technologies GmbH. All other names are the property of their respective owners; no affiliation with or endorsement by them is implied.

## Contributing

Issues and pull requests are welcome. By submitting a contribution you confirm that you wrote it (or have the right to submit it) and agree that it is licensed under `AGPL-3.0-only` like the rest of the project.

(c) 2026 Flix
