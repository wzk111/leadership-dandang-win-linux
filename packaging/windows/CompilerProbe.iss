; Verify the compiler through its preprocessor, without creating a package or
; touching an output directory. PE version resources and /? are not version APIs.
#if Ver != EncodeVer(6, 7, 0)
  #error WorkSidekick packaging requires exactly Inno Setup 6.7.0
#endif
#pragma message "WORKSIDEKICK_INNO_VERSION=" + DecodeVer(Ver)
[Setup]
AppName=WorkSidekick compiler probe
AppVersion=0.7.0
CreateAppDir=no
Uninstallable=no
PrivilegesRequired=lowest
Output=no
OutputDir=
OutputManifestFile=
