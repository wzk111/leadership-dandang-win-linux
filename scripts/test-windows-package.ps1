[CmdletBinding()]
param(
    [string]$ArtifactDirectory = 'build/packages',
    [string]$Version = '0.7.0-rc1',
    [string]$PreviousInstallerPath = ''
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
# This test creates production-format QSettings and credential records. Never run
# it on a user's account: hosted GitHub runners are disposable test accounts.
if ($env:GITHUB_ACTIONS -ne 'true' -or $env:RUNNER_ENVIRONMENT -ne 'github-hosted') {
    throw 'Installer verification requires a disposable GitHub-hosted Windows runner.'
}
$repo = Split-Path -Parent $PSScriptRoot
$artifacts = (Resolve-Path -LiteralPath $ArtifactDirectory).Path
$prefix = "WorkSidekick-$Version-windows-x64"
$zip = Join-Path $artifacts "$prefix.zip"
$installer = Join-Path $artifacts "$prefix-setup.exe"
foreach ($file in @($zip, $installer, (Join-Path $artifacts 'SHA256SUMS.txt'))) {
    if (!(Test-Path -LiteralPath $file -PathType Leaf)) { throw "Missing artifact: $file" }
}
foreach ($line in Get-Content -LiteralPath (Join-Path $artifacts 'SHA256SUMS.txt')) {
    if ($line -notmatch '^([0-9a-fA-F]{64})  ([A-Za-z0-9_.-]+)$') { throw 'Invalid checksum line' }
    $expected = $Matches[1]; $name = $Matches[2]
    if ((Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $artifacts $name)).Hash -ne $expected) {
        throw "Checksum mismatch: $name"
    }
}
$appId = '{5D602BE9-2D81-41B4-B1AF-DBA247631C91}'
$uninstallKey = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\${appId}_is1"
$settingsKey = 'HKCU:\Software\WorkSidekick\WorkSidekick'
if ((Test-Path -LiteralPath $settingsKey) -or (Test-Path -LiteralPath $uninstallKey)) {
    throw 'Existing WorkSidekick user data/installation found; refusing to modify it.'
}
if (Get-Process worksidekick,worksidekick_uia_worker -ErrorAction SilentlyContinue) {
    throw 'A WorkSidekick process is already running.'
}
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class PackageCredentialFixture {
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    private struct Credential {
        public uint Flags, Type; public string TargetName, Comment;
        public System.Runtime.InteropServices.ComTypes.FILETIME LastWritten;
        public uint BlobSize; public IntPtr Blob; public uint Persist, AttributeCount;
        public IntPtr Attributes; public string TargetAlias, UserName;
    }
    [DllImport("advapi32.dll", EntryPoint="CredReadW", CharSet=CharSet.Unicode, SetLastError=true)]
    private static extern bool ReadNative(string name, uint type, uint flags, out IntPtr value);
    [DllImport("advapi32.dll", EntryPoint="CredWriteW", CharSet=CharSet.Unicode, SetLastError=true)]
    private static extern bool WriteNative(ref Credential value, uint flags);
    [DllImport("advapi32.dll", EntryPoint="CredDeleteW", CharSet=CharSet.Unicode, SetLastError=true)]
    private static extern bool DeleteNative(string name, uint type, uint flags);
    [DllImport("advapi32.dll")] private static extern void CredFree(IntPtr value);
    public static string Read(string name) {
        IntPtr pointer;
        if (!ReadNative(name, 1, 0, out pointer)) {
            int code = Marshal.GetLastWin32Error();
            if (code == 1168) return null;
            throw new System.ComponentModel.Win32Exception(code);
        }
        try { var value = Marshal.PtrToStructure<Credential>(pointer);
            return Marshal.PtrToStringUni(value.Blob, (int)value.BlobSize / 2);
        } finally { CredFree(pointer); }
    }
    public static void Write(string name, string text) {
        IntPtr blob = Marshal.StringToCoTaskMemUni(text);
        try { var value = new Credential { Type=1, TargetName=name, BlobSize=(uint)(text.Length*2), Blob=blob, Persist=2 };
            if (!WriteNative(ref value, 0)) throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());
        } finally { Marshal.ZeroFreeCoTaskMemUnicode(blob); }
    }
    public static void Delete(string name) {
        if (!DeleteNative(name, 1, 0) && Marshal.GetLastWin32Error()!=1168)
            throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());
    }
}
'@
$targets = @('openai', 'anthropic', 'gemini', 'openai_compatible') | ForEach-Object { "WorkSidekick/$_" }
foreach ($target in $targets) {
    if ($null -ne [PackageCredentialFixture]::Read($target)) { throw 'Existing provider credential found; refusing to modify it.' }
}
$testRoot = Join-Path $env:RUNNER_TEMP ('worksidekick-package-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $testRoot | Out-Null
$installRoot = Join-Path $testRoot 'installed'
$reportPath = Join-Path $artifacts 'windows-package-smoke.json'
$writtenTargets = [System.Collections.Generic.List[string]]::new()
$ownedProcesses = [System.Collections.Generic.List[System.Diagnostics.Process]]::new()
$fixtures = @{}
foreach ($target in $targets) { $fixtures[$target] = "WorkSidekick M6 synthetic sentinel for $target; not an API key" }
$results = [ordered]@{ environment='GitHub-hosted Windows runner'; clean_machine='NOT TESTED - build runner has developer tools'; desktop='NOT TESTED - no real application acceptance'; portable='NOT TESTED'; helper_runtime='NOT TESTED'; install='NOT TESTED'; update='NOT TESTED'; single_instance='NOT TESTED'; running_app_uninstall='NOT TESTED'; uninstall='NOT TESTED'; settings='NOT TESTED'; credentials='NOT TESTED'; packaged_credential_read='NOT TESTED - vault retention only; native unit suite checks provider separation' }
function Start-OwnedProcess([string]$Executable, [string[]]$Arguments = @(), [string]$StandardOutput = '') {
    $options = @{ FilePath=$Executable; WorkingDirectory=$testRoot; PassThru=$true; WindowStyle='Hidden' }
    if ($Arguments.Count -gt 0) { $options.ArgumentList = $Arguments }
    if ($StandardOutput) { $options.RedirectStandardOutput = $StandardOutput }
    $process = Start-Process @options
    $ownedProcesses.Add($process)
    return $process
}
function Stop-OwnedProcess([System.Diagnostics.Process]$Process) {
    if (!$ownedProcesses.Contains($Process)) { throw 'Refusing to stop a process not created by this test.' }
    if (!$Process.HasExited) { $Process.Kill(); if (!$Process.WaitForExit(10000)) { throw 'Owned process did not terminate.' } }
}
function Invoke-ProcessExitCode([string]$Executable, [string[]]$Arguments = @(), [int]$TimeoutMilliseconds = 60000, [string]$StandardOutput = '') {
    $process = Start-OwnedProcess $Executable $Arguments $StandardOutput
    if (!$process.WaitForExit($TimeoutMilliseconds)) { Stop-OwnedProcess $process; throw "Timed out: $Executable" }
    return $process.ExitCode
}
function Invoke-CheckedProcess([string]$Executable, [string[]]$Arguments = @(), [int]$TimeoutMilliseconds = 60000) {
    $code = Invoke-ProcessExitCode $Executable $Arguments $TimeoutMilliseconds
    if ($code -ne 0) { throw "Process failed (${code}): $Executable" }
}
function Invoke-CleanEnvironment([scriptblock]$Action) {
    $names = @('PATH','QT_PLUGIN_PATH','QT_QPA_PLATFORM_PLUGIN_PATH','QML2_IMPORT_PATH','QML_IMPORT_PATH','QT_QPA_PLATFORM','QTDIR')
    $saved = @{}
    foreach ($name in $names) { $saved[$name] = [Environment]::GetEnvironmentVariable($name, 'Process'); [Environment]::SetEnvironmentVariable($name, $null, 'Process') }
    try {
        $env:PATH = "$env:SystemRoot\System32;$env:SystemRoot"
        & $Action
    } finally { foreach ($name in $names) { [Environment]::SetEnvironmentVariable($name, $saved[$name], 'Process') } }
}
function Invoke-CleanSmoke([string]$Directory) {
    Invoke-CleanEnvironment { Invoke-CheckedProcess (Join-Path $Directory 'worksidekick.exe') @('--smoke-test') }
}
function Assert-PrimaryReady([System.Diagnostics.Process]$Primary) {
    $deadline = [System.Diagnostics.Stopwatch]::StartNew()
    while ($deadline.ElapsedMilliseconds -lt 15000) {
        if ($Primary.HasExited) { throw "Installed primary exited during startup ($($Primary.ExitCode))." }
        try {
            $mutex = [System.Threading.Mutex]::OpenExisting('Local\WorkSidekick.Runtime')
            $mutex.Dispose()
            return
        } catch [System.Threading.WaitHandleCannotBeOpenedException] { Start-Sleep -Milliseconds 100 }
    }
    throw 'Installed primary did not expose its runtime mutex.'
}
function Assert-Fixtures {
    foreach ($entry in $settingFixtures) {
        $value = Get-ItemPropertyValue -LiteralPath "$settingsKey\$($entry[0])" -Name $entry[1]
        if ($value -ne $entry[2]) { throw "Setting did not persist: $($entry[0])/$($entry[1])" }
    }
    foreach ($target in $writtenTargets) {
        if ([PackageCredentialFixture]::Read($target) -ne $fixtures[$target]) { throw 'Provider-specific synthetic vault value did not persist.' }
    }
}
function Assert-InstalledPayload {
    foreach ($entry in (Get-Content -LiteralPath (Join-Path $artifacts 'windows-package-manifest.json') -Raw | ConvertFrom-Json)) {
        $file = Join-Path $installRoot $entry.path
        if (!(Test-Path -LiteralPath $file -PathType Leaf) -or (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash -ne $entry.sha256) {
            throw "Installed payload mismatch: $($entry.path)"
        }
    }
}
$settingFixtures = @(
    @('ai','provider','anthropic'), @('ai\anthropic','model','synthetic-m6-model'),
    @('profile','role','Synthetic QA role'), @('profile','enabled','true'),
    @('features','enabled','summarize'), @('features','quickActions','summarize'),
    @('ui','automaticPopup','false'), @('shortcut','enabled','false')
)
try {
    foreach ($target in $targets) { [PackageCredentialFixture]::Write($target, $fixtures[$target]); $writtenTargets.Add($target) }
    foreach ($entry in $settingFixtures) {
        New-Item -Path "$settingsKey\$($entry[0])" -Force | Out-Null
        New-ItemProperty -LiteralPath "$settingsKey\$($entry[0])" -Name $entry[1] -Value $entry[2] -PropertyType String -Force | Out-Null
    }
    $extract = Join-Path $testRoot 'portable'
    Expand-Archive -LiteralPath $zip -DestinationPath $extract
    $portableRoot = Join-Path $extract $prefix
    & python (Join-Path $repo 'scripts/audit-package.py') $portableRoot
    if ($LASTEXITCODE -ne 0) { throw 'Extracted portable package audit failed' }
    Invoke-CleanSmoke $portableRoot
    $results.portable = 'PASS - extracted ZIP, clean PATH and Qt environment'
    $workerOutput = Join-Path $testRoot 'uia-worker-invalid-arguments.json'
    $workerExit = Invoke-CleanEnvironment { Invoke-ProcessExitCode (Join-Path $portableRoot 'worksidekick_uia_worker.exe') @() 10000 $workerOutput }
    if ($workerExit -ne 0) { throw "Packaged UIA worker runtime failed: $workerExit" }
    $workerReply = Get-Content -LiteralPath $workerOutput -Raw | ConvertFrom-Json
    if ($workerReply.initialized -ne $false -or $workerReply.reason -ne 'invalid arguments') { throw 'Unexpected no-arguments UIA worker response.' }
    $results.helper_runtime = 'PASS - clean-environment helper starts and returns bounded invalid-arguments JSON; real UIA NOT TESTED'
    $initialInstaller = if ($PreviousInstallerPath) { (Resolve-Path -LiteralPath $PreviousInstallerPath).Path } else { $installer }
    Invoke-CheckedProcess $initialInstaller @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/SP-',"/DIR=`"$installRoot`"",'/TASKS=',"/LOG=`"$testRoot\install.log`"")
    if (!(Test-Path -LiteralPath $uninstallKey)) { throw 'Per-user uninstall registration missing' }
    $shortcut = Join-Path $env:APPDATA 'Microsoft\Windows\Start Menu\Programs\WorkSidekick.lnk'
    if (!(Test-Path -LiteralPath $shortcut)) { throw 'Start Menu shortcut missing' }
    Invoke-CleanSmoke $installRoot
    Assert-Fixtures
    $results.install = 'PASS - per-user install, Start Menu shortcut, packaged launch'
    Invoke-CheckedProcess $installer @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/SP-',"/DIR=`"$installRoot`"",'/TASKS=',"/LOG=`"$testRoot\update.log`"")
    Assert-InstalledPayload
    Invoke-CleanSmoke $installRoot
    Assert-Fixtures
    $results.update = if ($PreviousInstallerPath) { 'PASS - supplied previous installer to candidate' } else { 'PASS - same-version reinstall only; previous-version upgrade NOT TESTED' }
    $results.settings = 'PASS - provider/model, Profile, feature and shortcut preferences retained'
    $results.credentials = 'PASS - four distinct provider sentinel values retained in Windows vault; packaged app read NOT TESTED'
    $primary = Invoke-CleanEnvironment { Start-OwnedProcess (Join-Path $installRoot 'worksidekick.exe') }
    Assert-PrimaryReady $primary
    Invoke-CleanEnvironment {
        foreach ($iteration in 1..20) { Invoke-CheckedProcess (Join-Path $installRoot 'worksidekick.exe') @('--trigger') 10000 }
        Invoke-CheckedProcess (Join-Path $installRoot 'worksidekick.exe') @() 10000
    }
    if ($primary.HasExited) { throw 'Primary exited during second-instance forwarding.' }
    $runningApps = @(Get-Process worksidekick -ErrorAction SilentlyContinue)
    if ($runningApps.Count -ne 1 -or $runningApps[0].Id -ne $primary.Id) { throw 'Forwarding left a duplicate app process.' }
    $results.single_instance = 'PASS - installed normal primary, 20 --trigger calls and second normal launch forward and exit; one primary remains'
    $blockedUninstall = Invoke-ProcessExitCode (Join-Path $installRoot 'unins000.exe') @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART',"/LOG=`"$testRoot\uninstall-running.log`"")
    if ($blockedUninstall -eq 0 -or $primary.HasExited -or !(Test-Path -LiteralPath $uninstallKey)) {
        throw 'Running-app uninstall was not safely refused.'
    }
    Assert-InstalledPayload
    $results.running_app_uninstall = 'PASS - AppMutex blocks silent uninstall with nonzero exit, payload and owned primary preserved'
    Stop-OwnedProcess $primary
    Invoke-CheckedProcess (Join-Path $installRoot 'unins000.exe') @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART',"/LOG=`"$testRoot\uninstall.log`"")
    if ((Test-Path -LiteralPath (Join-Path $installRoot 'worksidekick.exe')) -or (Test-Path -LiteralPath $uninstallKey) -or (Test-Path -LiteralPath $shortcut)) {
        throw 'Uninstall left program, registration or Start Menu shortcut behind'
    }
    foreach ($entry in (Get-Content -LiteralPath (Join-Path $artifacts 'windows-package-manifest.json') -Raw | ConvertFrom-Json)) {
        if (Test-Path -LiteralPath (Join-Path $installRoot $entry.path)) { throw "Manifest payload survived uninstall: $($entry.path)" }
    }
    Assert-Fixtures
    $results.uninstall = 'PASS - program/registration/shortcut removed, user settings/credentials retained'
    if (Get-Process worksidekick,worksidekick_uia_worker -ErrorAction SilentlyContinue) { throw 'Packaged launch left an app/helper process behind.' }
} finally {
    foreach ($process in $ownedProcesses) { Stop-OwnedProcess $process }
    $results | ConvertTo-Json | Set-Content -LiteralPath $reportPath -Encoding utf8
    foreach ($target in $writtenTargets) { [PackageCredentialFixture]::Delete($target) }
    # Only this previously absent key was created by this disposable-runner test.
    if (Test-Path -LiteralPath $settingsKey) { Remove-Item -LiteralPath $settingsKey -Recurse -Force }
    Get-Content -LiteralPath $reportPath
}
