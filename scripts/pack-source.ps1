param([Parameter(Mandatory=$true)][string]$Destination)
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
$projectRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$zipPath=[IO.Path]::GetFullPath($Destination)
if (Test-Path -LiteralPath $zipPath) { throw 'Destination exists; choose a new ZIP path' }
$zip=[IO.Compression.ZipFile]::Open($zipPath,[IO.Compression.ZipArchiveMode]::Create)
$script:count=0
function Add-TrackedRepository([string]$Repo,[string]$Prefix) {
    $files=& git -c core.quotepath=false -C $Repo ls-files
    if ($LASTEXITCODE -ne 0) { throw "Cannot enumerate tracked source: $Repo" }
    foreach($relative in $files) {
        $path=Join-Path $Repo $relative
        $name=($Prefix+$relative).Replace('\','/')
        if (Test-Path -LiteralPath $path -PathType Container) { Add-TrackedRepository $path ($name+'/'); continue }
        if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing tracked source: $path" }
        if ($name -match '(?i)(\.jks$|\.keystore$|(^|/)local\.properties$)') { throw "Unexpected sensitive file: $name" }
        [IO.Compression.ZipFileExtensions]::CreateEntryFromFile($zip,$path,$name,[IO.Compression.CompressionLevel]::Optimal) | Out-Null
        $script:count++
    }
}
try { Add-TrackedRepository $projectRoot 'retro-tv-lite/' } finally { $zip.Dispose() }
Write-Output "Source archive: $zipPath ($script:count tracked files; dependency source included, .git and build output excluded)"
