param([string]$Cmake='cmake', [string]$Ctest='ctest')
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
Push-Location $root
try {
    & $Cmake -S native -B build/native '-DRETROTV_TESTS=ON'
    if ($LASTEXITCODE -ne 0) { throw 'Configure failed. Install a C++17 compiler.' }
    & $Cmake --build build/native --parallel 2
    if ($LASTEXITCODE -ne 0) { throw 'Native build failed' }
    & $Ctest --test-dir build/native --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw 'Native tests failed' }
} finally { Pop-Location }
