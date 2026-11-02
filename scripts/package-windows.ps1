[CmdletBinding()]
param(
    [string]$BuildDirectory = 'build',
    [string]$ArtifactDirectory = 'build/packages',
    [ValidatePattern('^\d+\.\d+\.\d+(-[A-Za-z0-9.]+)?$')]
    [string]$Version = '0.7.0-rc1',
    [string]$InnoCompiler = '',
    [string]$MsvcRedistributableDirectory = ''
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if ($env:OS -ne 'Windows_NT') { throw 'Windows packaging requires Windows x64 and an MSVC developer environment.' }
$repo = Split-Path -Parent $PSScriptRoot
$build = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($BuildDirectory)
$artifacts = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($ArtifactDirectory)
if (Test-Path -LiteralPath $artifacts) {
    if (Get-ChildItem -LiteralPath $artifacts -Force) { throw 'ArtifactDirectory must be empty; stale release files are not accepted.' }
} else { New-Item -ItemType Directory -Path $artifacts -Force | Out-Null }
$deploy = (Get-Command windeployqt.exe -ErrorAction Stop).Source
$qtBin = Split-Path -Parent $deploy
$qtVersion = (Get-Item -LiteralPath (Join-Path $qtBin 'Qt6Core.dll')).VersionInfo.ProductVersion
if ($qtVersion -notmatch '^6\.8\.3(\.|$)') { throw "Expected Qt 6.8.3 beside windeployqt, got: $qtVersion" }
if (!$InnoCompiler) {
    $installed = Get-Command ISCC.exe -ErrorAction SilentlyContinue
    $InnoCompiler = if ($installed) { $installed.Source } else { Join-Path ${env:ProgramFiles(x86)} 'Inno Setup 6/ISCC.exe' }
}
if (!(Test-Path -LiteralPath $InnoCompiler -PathType Leaf)) { throw 'Install Inno Setup 6 first or supply -InnoCompiler; this script never downloads installers.' }
$inno = (Get-Item -LiteralPath $InnoCompiler).VersionInfo
Write-Output "Inno Setup compiler: $InnoCompiler; ProductVersion=$($inno.ProductVersion); FileVersion=$($inno.FileVersion)"
$innoProbeOutput = (& $InnoCompiler (Join-Path $repo 'packaging/windows/CompilerProbe.iss') 2>&1 | Out-String)
$innoProbeExit = $LASTEXITCODE
Write-Output "ISCC compiler preprocessor probe exit code: $innoProbeExit; first 4096 output characters:"
Write-Output $innoProbeOutput.Substring(0, [Math]::Min(4096, $innoProbeOutput.Length))
# The probe has a compile-time Ver/EncodeVer guard and emits DecodeVer(Ver).
# It deliberately creates no installer or output directory.
$innoVersionMatch = [regex]::Match($innoProbeOutput, '(?m)^\s*WORKSIDEKICK_INNO_VERSION=(?<version>\d+\.\d+\.\d+)\s*$')
if ($innoProbeExit -ne 0 -or !$innoVersionMatch.Success -or $innoVersionMatch.Groups['version'].Value -ne '6.7.0') {
    throw 'Pinned Inno Setup 6.7.0 is required; compiler preprocessor probe failed.'
}
$innoVersion = $innoVersionMatch.Groups['version'].Value
Write-Output "Inno Setup compiler version: $innoVersion (verified with preprocessor Ver/DecodeVer)"
if (!$MsvcRedistributableDirectory) {
    if (!$env:VCToolsRedistDir) { throw 'VCToolsRedistDir is missing; run from an MSVC developer shell or supply -MsvcRedistributableDirectory.' }
    $MsvcRedistributableDirectory = Join-Path $env:VCToolsRedistDir 'x64/Microsoft.VC143.CRT'
}
$crt = (Resolve-Path -LiteralPath $MsvcRedistributableDirectory).Path
if ((Split-Path -Leaf $crt) -ne 'Microsoft.VC143.CRT' -or (Split-Path -Leaf (Split-Path -Parent $crt)) -ne 'x64') {
    throw 'MSVC DLLs must come from the Visual Studio x64/Microsoft.VC143.CRT redistributable directory, never System32 or compiler bin.'
}
foreach ($name in @('msvcp140.dll','vcruntime140.dll','vcruntime140_1.dll')) {
    if (!(Test-Path -LiteralPath (Join-Path $crt $name))) { throw "Required redistributable missing: $name" }
}
# Reconfigure and rebuild the current checkout. Never label an arbitrary old
# executable with the current Git SHA or assume a developer's cache is Release.
& cmake -S $repo -B $build -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) { throw 'Release package configuration failed' }
$cache = Get-Content -LiteralPath (Join-Path $build 'CMakeCache.txt') -Raw
if ($cache -notmatch '(?m)^CMAKE_BUILD_TYPE:STRING=Release\r?$') { throw 'Package cache must be configured as Release.' }
if ($cache -notmatch '(?m)^CMAKE_CXX_COMPILER:(?:FILEPATH|STRING)=(.+?)\r?$') { throw 'Cannot determine actual compiler from CMake cache.' }
$compilerPath = $Matches[1]
$compilerConfiguration = Get-ChildItem -LiteralPath (Join-Path $build 'CMakeFiles') -Filter CMakeCXXCompiler.cmake -Recurse -File | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
if (!$compilerConfiguration) { throw 'CMake compiler configuration missing.' }
$compilerInfo = Get-Content -LiteralPath $compilerConfiguration.FullName -Raw
if ($compilerInfo -notmatch 'set\(CMAKE_CXX_COMPILER_ID "MSVC"\)' -or
    $compilerInfo -notmatch 'set\(CMAKE_CXX_SIZEOF_DATA_PTR "?8"?\)' -or
    $compilerInfo -notmatch 'set\(CMAKE_CXX_COMPILER_ARCHITECTURE_ID "?x64"?\)') {
    throw 'Package requires an MSVC x64 compiler configuration.'
}
& cmake --build $build --config Release --target worksidekick worksidekick_uia_worker --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'Release product rebuild failed' }
$binaryDirectory = if ($cache -match '(?m)^CMAKE_CONFIGURATION_TYPES:STRING=') { Join-Path $build 'Release' } else { $build }
# A new, dedicated stage is populated by an explicit product allowlist. The
# native developer artifact may contain tests, but is never copied into here.
$prefix = "WorkSidekick-$Version-windows-x64"
$stageParent = Join-Path $build ('package-stage-' + [guid]::NewGuid().ToString('N'))
$stage = Join-Path $stageParent $prefix
New-Item -ItemType Directory -Path $stage -Force | Out-Null
foreach ($name in @('worksidekick.exe','worksidekick_uia_worker.exe')) {
    $source = Join-Path $binaryDirectory $name
    if (!(Test-Path -LiteralPath $source -PathType Leaf)) { throw "Build the Release product target first: $name" }
    $binary = [System.IO.File]::OpenRead($source)
    $reader = [System.IO.BinaryReader]::new($binary)
    try {
        if ($reader.ReadUInt16() -ne 0x5A4D) { throw "Invalid executable: $name" }
        $binary.Position = 0x3C; $peOffset = $reader.ReadUInt32()
        $binary.Position = $peOffset
        if ($reader.ReadUInt32() -ne 0x00004550 -or $reader.ReadUInt16() -ne 0x8664) { throw "Product executable must be PE x64: $name" }
    } finally { $reader.Dispose() }
    Copy-Item -LiteralPath $source -Destination $stage
    & $deploy --release --no-translations --no-compiler-runtime (Join-Path $stage $name)
    if ($LASTEXITCODE -ne 0) { throw "windeployqt failed for $name" }
}
Copy-Item -Path (Join-Path $crt '*.dll') -Destination $stage
@'
[Paths]
Prefix=.
Plugins=.
Translations=translations
'@ | Set-Content -LiteralPath (Join-Path $stage 'qt.conf') -Encoding ascii
foreach ($name in @('Qt6Core.dll','Qt6Gui.dll','Qt6Widgets.dll','Qt6Network.dll','Qt6Concurrent.dll','platforms/qwindows.dll')) {
    if (!(Test-Path -LiteralPath (Join-Path $stage $name))) { throw "Required deployed runtime missing: $name" }
}
$licenseDir = Join-Path $stage 'licenses'
New-Item -ItemType Directory -Path $licenseDir | Out-Null
Copy-Item -Path (Join-Path $repo 'packaging/windows/licenses/*.txt') -Destination $licenseDir
Copy-Item -LiteralPath (Join-Path $repo 'packaging/THIRD_PARTY.md') -Destination $licenseDir
Copy-Item -LiteralPath (Join-Path $repo 'packaging/windows/README.md') -Destination (Join-Path $stage 'WINDOWS-README.md')
Copy-Item -LiteralPath (Join-Path $repo 'resources/worksidekick.svg') -Destination $stage
$runtimeInventory = @(Get-ChildItem -LiteralPath $stage -Recurse -File | Where-Object Extension -eq '.dll' | Sort-Object FullName | ForEach-Object {
    [ordered]@{ path=[System.IO.Path]::GetRelativePath($stage,$_.FullName).Replace('\','/'); version=$_.VersionInfo.FileVersion; sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant() }
})
[ordered]@{
    application=$Version
    qt=$qtVersion
    compiler_runtime='MSVC v143 x64 app-local, from Visual Studio redistributable directory; Windows UCRT is an OS component'
    installer_tool="Inno Setup $innoVersion (ISCC preprocessor Ver/DecodeVer)"
    qt_license_source='https://github.com/qt/qtbase/tree/v6.8.3/LICENSES'
    native_os_dependencies=@('Windows UI Automation','COM','Credential Manager','user32','advapi32','Windows TLS/Schannel')
    signing='NOT CONFIGURED - unsigned candidate'
    dlls=$runtimeInventory
} | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $stage 'dependency-inventory.json') -Encoding utf8
$commit = (& git -C $repo rev-parse HEAD | Out-String).Trim()
if ($LASTEXITCODE -ne 0) { throw 'Cannot determine source commit' }
$metadata = [ordered]@{
    commit=$commit; version=$Version; configuration='Release'; architecture='x64'
    source_dirty=[bool](& git -C $repo status --porcelain)
    compiler=(Get-Item -LiteralPath $compilerPath).VersionInfo.ProductVersion
    compiler_path=$compilerPath; compiler_provenance='CMakeCache.txt, verified MSVC 64-bit compiler configuration and x64 PE payload'
    qt=$qtVersion; cmake=(& cmake --version | Select-Object -First 1)
    runner=$env:RUNNER_OS; runner_image=$env:ImageOS; runner_image_version=$env:ImageVersion
    os=[Environment]::OSVersion.VersionString; inno_setup=$innoVersion
    inno_version_source='ISCC preprocessor Ver/DecodeVer compiled probe'; inno_pe_product_version=$inno.ProductVersion
    runtime_strategy='App-local Visual Studio redistributable CRT DLLs, no elevated redistributable installer'
    byte_reproducibility='NOT TESTED'; signing='NOT CONFIGURED'
}
$metadata | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $stage 'build-metadata.json') -Encoding utf8
Copy-Item -LiteralPath (Join-Path $stage 'build-metadata.json') -Destination $artifacts
Copy-Item -LiteralPath (Join-Path $stage 'dependency-inventory.json') -Destination $artifacts
& python (Join-Path $repo 'scripts/audit-package.py') $stage --manifest (Join-Path $artifacts 'windows-package-manifest.json')
if ($LASTEXITCODE -ne 0) { throw 'Product stage security audit failed' }
Compress-Archive -LiteralPath $stage -DestinationPath (Join-Path $artifacts "$prefix.zip") -CompressionLevel Optimal
& $InnoCompiler "/DStageDir=$stage" "/DArtifactDir=$artifacts" "/DPackageVersion=$Version" (Join-Path $repo 'packaging/windows/WorkSidekick.iss')
if ($LASTEXITCODE -ne 0) { throw 'Inno Setup compilation failed' }
foreach ($name in @("$prefix.zip","$prefix-setup.exe")) {
    if (!(Test-Path -LiteralPath (Join-Path $artifacts $name) -PathType Leaf)) { throw "Expected artifact not created: $name" }
}
$checksums = Get-ChildItem -LiteralPath $artifacts -File | Sort-Object Name | ForEach-Object {
    '{0}  {1}' -f (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant(), $_.Name
}
$checksums | Set-Content -LiteralPath (Join-Path $artifacts 'SHA256SUMS.txt') -Encoding ascii
Write-Output "Product artifacts: $artifacts"
Get-Content -LiteralPath (Join-Path $artifacts 'SHA256SUMS.txt')
