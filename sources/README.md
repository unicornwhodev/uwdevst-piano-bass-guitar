# Instrument sources

Each folder is independent and retains its plug-in identifiers, presets, components and licence.

| Instrument | Folder | CMake project |
| --- | --- | --- |
| Piano | [piano](piano/) | `piano/synth-piano` |
| Bass | [bass](bass/) | `bass` |
| Guitar | [guitar](guitar/) | `guitar/synth-guitar` |

To build on Windows from the collection root:

```powershell
.\build.ps1 -Instrument Bass -JuceDir 'C:\Dev\JUCE'
```

Replace Bass with Piano or Guitar. Requirements: CMake 3.22 or newer, a Visual Studio C++ toolchain and an external JUCE 8.0.4 checkout. Original scripts remain in each folder; build output stays local.

Piano sources include the local v3 candidate accepted on the documented auditions. The 1.0.2 release files retain their original version and licence. [Versions and availability](../docs/VERSIONS.md).

[Collection home](../README.md)
