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
$build = (Resolve-Path -LiteralPath $BuildDirectory).Path
$artifacts = [System.IO.Path]::GetFullPath($ArtifactDirectory)
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
if ($inno.FileMajorPart -ne 6 -or $inno.FileMinorPart -lt 3) { throw 'Inno Setup 6.3 or later in major version 6 is required.' }
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
# A new, dedicated stage is populated by an explicit product allowlist. The
# native developer artifact may contain tests, but is never copied into here.
$prefix = "WorkSidekick-$Version-windows-x64"
$stageParent = Join-Path $build ('package-stage-' + [guid]::NewGuid().ToString('N'))
$stage = Join-Path $stageParent $prefix
New-Item -ItemType Directory -Path $stage -Force | Out-Null
foreach ($name in @('worksidekick.exe','worksidekick_uia_worker.exe')) {
    $source = Join-Path $build $name
    if (!(Test-Path -LiteralPath $source)) { $source = Join-Path $build "Release/$name" }
    if (!(Test-Path -LiteralPath $source -PathType Leaf)) { throw "Build the Release product target first: $name" }
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
    installer_tool=$inno.ProductVersion
    qt_license_source='https://github.com/qt/qtbase/tree/v6.8.3/LICENSES'
    native_os_dependencies=@('Windows UI Automation','COM','Credential Manager','user32','advapi32','Windows TLS/Schannel')
    signing='NOT CONFIGURED - unsigned candidate'
    dlls=$runtimeInventory
} | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $stage 'dependency-inventory.json') -Encoding utf8
$commit = (& git -C $repo rev-parse HEAD | Out-String).Trim()
if ($LASTEXITCODE -ne 0) { throw 'Cannot determine source commit' }
$compiler = Get-Command cl.exe -ErrorAction Stop
$metadata = [ordered]@{
    commit=$commit; version=$Version; configuration='Release'; architecture='x64'
    source_dirty=[bool](& git -C $repo status --porcelain)
    compiler=(Get-Item -LiteralPath $compiler.Source).VersionInfo.ProductVersion
    qt=$qtVersion; cmake=(& cmake --version | Select-Object -First 1)
    runner=$env:RUNNER_OS; runner_image=$env:ImageOS; runner_image_version=$env:ImageVersion
    os=[Environment]::OSVersion.VersionString; inno_setup=$inno.ProductVersion
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
