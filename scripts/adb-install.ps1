param([Parameter(Mandatory=$true)][string]$Serial, [string]$Adb='adb', [string]$Apk)
$ErrorActionPreference='Stop'
if (!$Apk) { $Apk=Join-Path $PSScriptRoot '../app/build/outputs/apk/debug/app-debug.apk' }
if (!(Test-Path -LiteralPath $Apk)) { throw "APK missing: $Apk" }
& $Adb -s $Serial install -r $Apk
if ($LASTEXITCODE -ne 0) { throw 'ADB install failed; no app data was cleared' }
& $Adb -s $Serial shell am start -n org.retrotv.lite/.RetroActivity
if ($LASTEXITCODE -ne 0) { throw 'ADB launch failed' }
