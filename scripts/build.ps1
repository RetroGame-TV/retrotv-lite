param([string]$Sdk = $env:ANDROID_HOME, [switch]$PrivateCores, [switch]$Release)
$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if (!$Sdk) { throw 'Set ANDROID_HOME or pass -Sdk' }
$env:ANDROID_HOME = $Sdk
if (!(Test-Path -LiteralPath (Join-Path $projectRoot 'third_party/SDL/CMakeLists.txt'))) { throw 'git submodule update --init --recursive' }
& (Join-Path $PSScriptRoot 'build-cores.ps1') -Sdk $Sdk -Core pcsx_rearmed
if ($PrivateCores) { & (Join-Path $PSScriptRoot 'build-cores.ps1') -Sdk $Sdk -Core snes9x2005,picodrive,fbneo -PrivateCores }
$tasks = @(':app:testDebugUnitTest', ':app:lintDebug', ':app:assembleDebug')
if ($Release) { $tasks += ':app:assembleRelease' }
if ($PrivateCores) { $tasks += '-PprivateCores=true' }
Push-Location $projectRoot
try { & ./gradlew.bat @tasks --console=plain; if ($LASTEXITCODE -ne 0) { throw "Gradle failed: $LASTEXITCODE" } }
finally { Pop-Location }
