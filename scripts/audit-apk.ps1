param([Parameter(Mandatory=$true)][string]$Apk)
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip=[IO.Compression.ZipFile]::OpenRead([IO.Path]::GetFullPath($Apk))
try {
    $libraries=@($zip.Entries | Where-Object { $_.FullName -match '^lib/.+\.so$' })
    if (!$libraries.Count) { throw 'No native libraries' }
    foreach($entry in $libraries) {
        if (!$entry.FullName.StartsWith('lib/armeabi-v7a/')) { throw "Unexpected ABI: $($entry.FullName)" }
        $stream=$entry.Open();$header=New-Object byte[] 20
        try { $read=$stream.Read($header,0,20) } finally { $stream.Dispose() }
        if($read -ne 20 -or $header[0] -ne 127 -or $header[1] -ne 69 -or $header[2] -ne 76 -or $header[3] -ne 70 -or $header[4] -ne 1 -or $header[18] -ne 40 -or $header[19] -ne 0){throw "Not ELF32 ARM: $($entry.FullName)"}
        Write-Output "$($entry.FullName): ELF32 ARM"
    }
    foreach($required in @('lib/armeabi-v7a/libSDL2.so','lib/armeabi-v7a/libmain.so','lib/armeabi-v7a/libquicknes_libretro.so','assets/demo/retrotv-pad-test.nes','assets/licenses/NOTICES.md')) {
        if(!$zip.GetEntry($required)){throw "Missing package entry: $required"}
    }
    Write-Output "APK audit passed. Only ARMv7 native libraries; demo and notices included."
} finally { $zip.Dispose() }
