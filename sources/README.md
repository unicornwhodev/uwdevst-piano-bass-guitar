# Sources des trois instruments

Chaque dossier est autonome et conserve ses identifiants de plugin, ses presets, ses composants et sa licence.

| Instrument | Dossier | Projet CMake |
| --- | --- | --- |
| Piano | [piano](piano/) | `piano/synth-piano` |
| Bass | [bass](bass/) | `bass` |
| Guitar | [guitar](guitar/) | `guitar/synth-guitar` |

Pour compiler sous Windows depuis la racine de la collection :

```powershell
.\build.ps1 -Instrument Bass -JuceDir 'C:\Dev\JUCE'
```

Remplacer Bass par Piano ou Guitar. Prérequis : CMake 3.22+, outils C++ Visual Studio et JUCE 8.0.4 externe. Les scripts d’origine restent dans chaque dossier ; les sorties de build restent locales.

Les sources Piano proviennent du candidat local v3 accepté sur les auditions documentées. Les fichiers de release 1.0.2 gardent leur version et leur licence d’origine.

[Présentation](../README.md)
