; Keep AppId stable across every version so update/uninstall stay per-user.
#ifndef StageDir
  #error StageDir must be supplied by package-windows.ps1
#endif
#ifndef ArtifactDir
  #error ArtifactDir must be supplied by package-windows.ps1
#endif
#ifndef PackageVersion
  #define PackageVersion "0.7.0-rc1"
#endif
[Setup]
AppId={{5D602BE9-2D81-41B4-B1AF-DBA247631C91}
AppName=WorkSidekick
AppVersion={#PackageVersion}
AppPublisher=WorkSidekick
AppPublisherURL=https://github.com/wzk111/leadership-dandang-win-linux
DefaultDirName={localappdata}\Programs\WorkSidekick
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
; Windows Server 2022 CI shares the Windows 10 API baseline. Product support and
; real desktop acceptance remain Windows 11; this does not certify Windows 10.
MinVersion=10.0
OutputDir={#ArtifactDir}
OutputBaseFilename=WorkSidekick-{#PackageVersion}-windows-x64-setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayIcon={app}\worksidekick.exe
AppMutex=Local\WorkSidekick.Runtime
CloseApplications=yes
CloseApplicationsFilter=worksidekick.exe,worksidekick_uia_worker.exe,*.dll
RestartApplications=no
SetupLogging=yes
; No Run, Registry, services, scheduled tasks, or autostart entries.
; No UninstallDelete entries: QSettings and OS vault credentials remain intact.
[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional shortcuts:"; Flags: unchecked
[Files]
Source: "{#StageDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
[Icons]
Name: "{autoprograms}\WorkSidekick"; Filename: "{app}\worksidekick.exe"; WorkingDir: "{app}"
Name: "{autodesktop}\WorkSidekick"; Filename: "{app}\worksidekick.exe"; WorkingDir: "{app}"; Tasks: desktopicon
