# Téléchargements

Les fichiers officiels 1.0.2 sont transférés à l’identique dans cette collection. Leur nom, leur taille et leur SHA-256 sont conservés. Chaque instrument a sa propre release.

| Instrument | Windows x64 | Linux x86_64 | macOS |
| --- | --- | --- | --- |
| Bass 1.0.2 | [Archive](https://github.com/unicornwhodev/uwdevst-piano-bass-guitar/releases/download/bass-v1.0.2/synth-bass_1.0.2_Windows_x64.zip) | [Archive](https://github.com/unicornwhodev/uwdevst-piano-bass-guitar/releases/download/bass-v1.0.2/UWdeVST_Bass_1.0.2_Linux_x86_64.tar.gz) | [Builder](https://github.com/unicornwhodev/uwdevst-piano-bass-guitar/releases/download/bass-v1.0.2/UWdeVST_Bass_1.0.2_macOS_Builder.tar.gz) |
| Guitar 1.0.2 | [Archive](https://github.com/unicornwhodev/uwdevst-piano-bass-guitar/releases/download/guitar-v1.0.2/synth-guitar_1.0.2_Windows_x64.zip) | [Archive](https://github.com/unicornwhodev/uwdevst-piano-bass-guitar/releases/download/guitar-v1.0.2/UWdeVST_Guitar_1.0.2_Linux_x86_64.tar.gz) | [Builder](https://github.com/unicornwhodev/uwdevst-piano-bass-guitar/releases/download/guitar-v1.0.2/UWdeVST_Guitar_1.0.2_macOS_Builder.tar.gz) |
| Piano 1.0.2 | [Archive](https://github.com/unicornwhodev/uwdevst-piano-bass-guitar/releases/download/piano-v1.0.2/synth-piano_1.0.2_Windows_x64.zip) | [Archive](https://github.com/unicornwhodev/uwdevst-piano-bass-guitar/releases/download/piano-v1.0.2/UWdeVST_Piano_1.0.2_Linux_x86_64.tar.gz) | [Builder](https://github.com/unicornwhodev/uwdevst-piano-bass-guitar/releases/download/piano-v1.0.2/UWdeVST_Piano_1.0.2_macOS_Builder.tar.gz) |

## Piano corrigé

Les sources Piano intègrent les corrections auditionnées du candidat v3. Les anciens fichiers Piano 1.0.2 ci-dessus ne contiennent pas ces corrections. Aucune nouvelle version binaire n’est annoncée par ce transfert.

## Vérifier un fichier

Les [sommes SHA-256](SHA256SUMS.txt) permettent de contrôler le téléchargement.

```powershell
Get-FileHash -Algorithm SHA256 '.\nom-du-fichier.zip'
```

Les archives macOS contiennent leurs sources et leur commande de compilation. Un builder macOS est à exécuter sur un Mac ; ce n’est pas une application déjà compilée.

[Installation](INSTALLATION.md) · [Accueil](../README.md)
