; ---------------------------------------------------------------------------
;  UWdeVST Guitar — Inno Setup 6 installer script
;  Installs: Standalone application + VST3 plugin bundle (Windows x64)
;
;  Usage:
;    ISCC.exe /DAppVersion=1.0.2 installer\UWdeVST_guitar.iss
; ---------------------------------------------------------------------------

#ifndef AppVersion
  #define AppVersion "1.0.2"
#endif

#define AppName      "uwdevst_guitar"
#define AppPublisher "unicorn who dev / Charli Billabert"
#define AppExeName   "uwdevst_guitar.exe"
#define Vst3Name     "uwdevst_guitar.vst3"
#define BuildDir     "..\build\UWdeVST_guitar_artefacts\Release"

; ---------------------------------------------------------------------------
[Setup]
AppId={{com.uwdevst.guitar}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}

; Installation paths
DefaultDirName={autopf64}\unicorn who dev\{#AppName}
DefaultGroupName=unicorn who dev\{#AppName}

; Installer output
OutputDir=output
OutputBaseFilename=uwdevst_guitar_{#AppVersion}_Windows_x64_Setup

; Compression
Compression=lzma2/ultra64
SolidCompression=yes

; Require 64-bit Windows
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible

; Require admin rights (needed to write to Common Files\VST3)
PrivilegesRequired=admin

; Appearance
WizardStyle=modern
WizardImageFile=..\assets versions png\logo,favic\logo_guitar.png
WizardSmallImageFile=..\assets versions png\logo,favic\logo_guitar.png

; Uninstall
UninstallDisplayName={#AppName} {#AppVersion}
UninstallDisplayIcon={app}\{#AppExeName}

; ---------------------------------------------------------------------------
[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "french";  MessagesFile: "compiler:Languages\French.isl"

; ---------------------------------------------------------------------------
[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

; ---------------------------------------------------------------------------
[Files]
; --- Standalone application ---
Source: "{#BuildDir}\Standalone\{#AppExeName}"; DestDir: "{app}"; Flags: ignoreversion

; --- VST3 plugin bundle (folder: Contents\x86_64-win\*.vst3 inside) ---
Source: "{#BuildDir}\VST3\{#Vst3Name}\*"; DestDir: "{commoncf64}\VST3\{#Vst3Name}"; Flags: ignoreversion recursesubdirs createallsubdirs

; ---------------------------------------------------------------------------
[Icons]
Name: "{group}\{#AppName}";                       Filename: "{app}\{#AppExeName}"
Name: "{group}\{cm:UninstallProgram,{#AppName}}"; Filename: "{uninstallexe}"
Name: "{commondesktop}\{#AppName}";               Filename: "{app}\{#AppExeName}"; Tasks: desktopicon

; ---------------------------------------------------------------------------
[Run]
Filename: "{app}\{#AppExeName}"; Description: "{cm:LaunchProgram,{#AppName}}"; Flags: nowait postinstall skipifsilent

; ---------------------------------------------------------------------------
[UninstallDelete]
; Remove the VST3 bundle folder entirely (Inno Setup only deletes files it installed)
Type: filesandordirs; Name: "{commoncf64}\VST3\{#Vst3Name}"
