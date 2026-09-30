#define MyAppName      "uwdevst_piano"
#ifndef AppVersion
#define AppVersion     "1.0.2"
#endif
#define MyAppVersion   AppVersion
#define MyAppPublisher "unicorn who dev / Charli Billabert"
#define MyAppExeName   "uwdevst_piano.exe"
#define ArtifactsDir   "..\build\UWdeVST_piano_artefacts\Release"

[Setup]
AppId={{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\unicorn who dev\{#MyAppName}
DefaultGroupName={#MyAppName}
OutputDir=output
OutputBaseFilename=uwdevst_piano_{#MyAppVersion}_Windows_x64_Setup
Compression=lzma2/ultra64
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
DisableProgramGroupPage=no
UninstallDisplayName={#MyAppName}
UninstallDisplayIcon={app}\{#MyAppExeName}

[Languages]
Name: "french";   MessagesFile: "compiler:Languages\French.isl"
Name: "english";  MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
; ── Standalone ──────────────────────────────────────────────────────────
Source: "{#ArtifactsDir}\Standalone\{#MyAppExeName}"; \
  DestDir: "{app}"; Flags: ignoreversion

; ── VST3 bundle (folder + all subfolders) ───────────────────────────────
Source: "{#ArtifactsDir}\VST3\uwdevst_piano.vst3\*"; \
  DestDir: "{commoncf}\VST3\uwdevst_piano.vst3"; \
  Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#MyAppName}";          Filename: "{app}\{#MyAppExeName}"
Name: "{group}\Désinstaller {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}";    Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Lancer {#MyAppName}"; \
  Flags: nowait postinstall skipifsilent

[UninstallDelete]
; Retire le dossier VST3 s'il est vide après désinstallation
Type: dirifempty; Name: "{commoncf}\VST3\uwdevst_piano.vst3"
