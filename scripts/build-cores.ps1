param(
    [string]$Sdk = $env:ANDROID_HOME,
    [ValidateSet('pcsx_rearmed','snes9x2005','picodrive','fbneo')][string[]]$Core = @('pcsx_rearmed'),
    [switch]$PrivateCores,
    [ValidateSet('armeabi-v7a','x86')][string]$Abi = 'armeabi-v7a',
    [switch]$SnesLagFix
)
$ErrorActionPreference = 'Stop'
if ($SnesLagFix -and $Core -notcontains 'snes9x2005') { throw '-SnesLagFix requires -Core snes9x2005' }
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if (!$Sdk) { throw 'Set ANDROID_HOME or pass -Sdk' }
$ndk = Join-Path $Sdk 'ndk/26.3.11579264'
$ndkBuild = Join-Path $ndk 'ndk-build.cmd'
if (!(Test-Path -LiteralPath $ndkBuild)) { $ndkBuild = Join-Path $ndk 'ndk-build' }
if (!(Test-Path -LiteralPath $ndkBuild)) { throw 'Install NDK 26.3.11579264' }
$registry = Get-Content -Raw -LiteralPath (Join-Path $projectRoot 'cores/registry.json') | ConvertFrom-Json
foreach ($name in $Core) {
    $restricted = $name -ne 'pcsx_rearmed'
    if ($restricted -and !$PrivateCores) { throw "$name has noncommercial/mixed license restrictions. Read docs/licenses.md; private build requires -PrivateCores." }
    $source = (Join-Path $projectRoot "third_party/$name").Replace('\', '/')
    $entry = $registry.cores | Where-Object id -EQ $name
    if (!(Test-Path -LiteralPath $source)) { throw "Missing source $source; git submodule update --init --recursive" }
    if (Test-Path -LiteralPath (Join-Path $source '.git')) {
        $revision = (& git -C $source rev-parse HEAD).Trim()
        if ($revision -ne $entry.revision) { throw "Revision mismatch for $name" }
    }
    if ($name -eq 'fbneo') {
        $mk = Join-Path $source 'src/burner/libretro/jni/Android.mk'
        $text = Get-Content -Raw -LiteralPath $mk
        # Build-only patch: CPS1/CPS2 subset and relative objects avoid Windows MAX_PATH.
        $text = $text.Replace('include $(LOCAL_PATH)/../Makefile.all', "SUBSET := cps12`n" + 'include $(LOCAL_PATH)/../Makefile.cps12')
        $text = $text.Replace('LOCAL_SRC_FILES    := $(SOURCES_C) $(SOURCES_S) $(SOURCES_CXX)', 'LOCAL_SRC_FILES    := $(patsubst $(LOCAL_PATH)/%,%,$(SOURCES_C) $(SOURCES_S) $(SOURCES_CXX))')
        [IO.File]::WriteAllText($mk, $text)
        $perl = (Get-Command perl -ErrorAction SilentlyContinue).Source
        if (!$perl -and (Test-Path -LiteralPath 'C:/Program Files/Git/usr/bin/perl.exe')) { $perl = 'C:/Program Files/Git/usr/bin/perl.exe' }
        if (!$perl) { throw 'FBNeo subset needs Perl (Git for Windows includes it)' }
        Push-Location $source
        try {
            & $perl 'src/dep/scripts/gamelist.pl' '-o' 'src/dep/generated/driverlist.h' 'src/burn/drv/capcom/d_cps1.cpp' 'src/burn/drv/capcom/d_cps2.cpp'
            if ($LASTEXITCODE -ne 0) { throw 'Driver list generation failed' }
        } finally { Pop-Location }
        $jni = Join-Path $source 'src/burner/libretro/jni'
    } else { $jni = Join-Path $source 'jni' }
    $jni = $jni.Replace('\', '/')
    $makefile = Join-Path $jni 'Android.mk'
    if ($name -eq 'snes9x2005') {
        # Own wrapper: no edits to the pinned upstream checkout.
        $makefile = (Join-Path $projectRoot 'cores/snes9x2005/Android.mk').Replace('\', '/')
    } else {
        $makeText = Get-Content -Raw -LiteralPath $makefile
        $relativeObjects = 'LOCAL_SRC_FILES := $(patsubst $(LOCAL_PATH)/%,%,$(LOCAL_SRC_FILES))'
        if (!$makeText.Contains($relativeObjects)) {
            $makeText = $makeText.Replace('include $(BUILD_SHARED_LIBRARY)', $relativeObjects + "`n" + 'include $(BUILD_SHARED_LIBRARY)')
            [IO.File]::WriteAllText($makefile, $makeText)
        }
    }
    $variant = if ($name -eq 'snes9x2005' -and $SnesLagFix) { "${name}_lowlatency" } else { $name }
    $outRoot = (Join-Path $projectRoot "build/cores/$variant/$Abi").Replace('\', '/')
    $buildArgs = @("NDK_PROJECT_PATH=$source", "APP_BUILD_SCRIPT=$makefile", "NDK_APPLICATION_MK=$jni/Application.mk", "APP_ABI=$Abi", 'APP_PLATFORM=android-26', 'APP_STL=c++_shared', "NDK_OUT=$outRoot/obj", "NDK_LIBS_OUT=$outRoot/lib", '-j4')
    if ($name -eq 'snes9x2005') {
        $lagFix = if ($SnesLagFix) { '1' } else { '0' }
        $buildArgs += @("RETROTV_SNES_SOURCE=$source", "RETROTV_CORE_REVISION=$($entry.revision.Substring(0,7))", "RETROTV_SNES_LAGFIX=$lagFix")
    }
    if ($name -eq 'pcsx_rearmed') { $buildArgs += @('USE_ASYNC_CDROM=0','USE_ASYNC_GPU=0','USE_ASYNC_SPU=0','NDRC_THREAD=0') }
    if ($name -eq 'fbneo') { $buildArgs += @('GIT_DATE=261023', 'GIT_VERSION=2b0c1c0') }
    Push-Location $source
    try { & $ndkBuild @buildArgs; if ($LASTEXITCODE -ne 0) { throw "$name build failed: $LASTEXITCODE" } }
    finally { Pop-Location }
    $category = if ($restricted) { 'private' } else { 'packaged' }
    $destination = Join-Path $projectRoot "cores/$category/$Abi"
    New-Item -ItemType Directory -Force -Path $destination | Out-Null
    Copy-Item -LiteralPath (Join-Path $outRoot "lib/$Abi/libretro.so") -Destination (Join-Path $destination "lib${variant}_libretro.so")
}
