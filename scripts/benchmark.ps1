param([Parameter(Mandatory=$true)][string]$Serial, [string]$Adb='adb', [string]$Out='build/benchmark')
$ErrorActionPreference='Stop'
New-Item -ItemType Directory -Force -Path $Out | Out-Null
& $Adb -s $Serial shell getprop ro.product.cpu.abilist | Set-Content (Join-Path $Out 'abi.txt')
& $Adb -s $Serial shell getprop ro.opengles.version | Set-Content (Join-Path $Out 'gles.txt')
& $Adb -s $Serial shell wm size | Set-Content (Join-Path $Out 'display.txt')
& $Adb -s $Serial shell dumpsys meminfo org.retrotv.lite | Set-Content (Join-Path $Out 'meminfo.txt')
& $Adb -s $Serial shell dumpsys gfxinfo org.retrotv.lite | Set-Content (Join-Path $Out 'gfxinfo.txt')
$appPid=(& $Adb -s $Serial shell pidof org.retrotv.lite).Trim()
if ($appPid -match '^\d+$') {
    & $Adb -s $Serial shell top -b -n 1 -p $appPid | Set-Content (Join-Path $Out 'process.txt')
    & $Adb -s $Serial logcat -d --pid=$appPid -t 1000 | Set-Content (Join-Path $Out 'app-log.txt')
}
Write-Output "Evidence saved in $Out. No FPS/PSS target is inferred by this script."
