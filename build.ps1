param(
    [Parameter(Mandatory)][ValidateSet('Piano','Bass','Guitar')][string]$Instrument,
    [string]$JuceDir = '',
    [string]$Configuration = 'Release'
)
$ErrorActionPreference = 'Stop'
$instrumentRoot = Join-Path $PSScriptRoot ('sources/' + $Instrument.ToLowerInvariant())
$buildScript = Join-Path $instrumentRoot '_build_all.ps1'
& $buildScript -Configuration $Configuration -JuceDir $JuceDir
