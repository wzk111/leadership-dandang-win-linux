# Windows candidate packaging

The supported product target is Windows 11 x64. The ZIP needs no Qt installation.
Extract the entire folder, then run `worksidekick.exe`. Configuration and provider
keys use the current user's QSettings and Windows Credential Manager, including
when running the portable distribution. The ZIP does not keep user data beside
the executable.

Run the installer normally to install into
`%LOCALAPPDATA%\Programs\WorkSidekick`. It creates a Start Menu shortcut and
offers a desktop shortcut (unchecked by default). It requests no administrator
privileges, installs no service or task, and enables no autostart entry. Close
WorkSidekick before upgrading or uninstalling; a runtime mutex safely refuses
those operations while the primary app is running. The installer also uses
Windows Restart Manager to detect files in use. Its AppId remains stable across versions. Upgrade keeps
settings and credentials. No automatic relaunch occurs after setup.

Uninstall from Windows Settings > Apps. Uninstall removes program files,
shortcuts and registration, and keeps settings and provider keys. For complete
manual removal, use WorkSidekick's Settings to delete each saved provider key and
clear Profile before uninstalling. If needed after uninstall, remove only the
`HKEY_CURRENT_USER\Software\WorkSidekick\WorkSidekick` settings key and the
`WorkSidekick/openai`, `WorkSidekick/anthropic`, `WorkSidekick/gemini`, and
`WorkSidekick/openai_compatible` generic entries in Windows Credential Manager.
Never delete unrelated credentials.

## Build

From the repository root in an MSVC 2022 x64 developer PowerShell, with Qt
**6.8.3 MSVC 2022 x64** on PATH, Python 3, CMake and a preinstalled **Inno Setup
6.7.0**:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
./scripts/package-windows.ps1 -BuildDirectory build -ArtifactDirectory build/packages -Version 0.7.0-rc1
```

`ArtifactDirectory` must be empty. Packaging reconfigures the current checkout
as Release and rebuilds only `worksidekick` and `worksidekick_uia_worker` plus
their dependencies. It verifies the cached MSVC x64 configuration and actual
PE architecture, and records the compiler selected by CMake. The script does not download or run a runtime
installer. `-InnoCompiler` can point to an existing 6.7.0 `ISCC.exe`. CI downloads
the [official Inno Setup 6.7.0 release](https://github.com/jrsoftware/issrc/releases/tag/is-6_7_0)
to the runner temporary directory and checks SHA-256
`f45c7d68d1e660cf13877ec36738a5179ce72a33414f9959d35e99b68c52a697`
before executing the installer. Installation is per-user, bounded and hidden;
CI passes the exact installed compiler path. The package script verifies the
compiler version by compiling `CompilerProbe.iss`: its preprocessor compares
`Ver` with `EncodeVer(6, 7, 0)` and emits `DecodeVer(Ver)`, with output disabled.
PE version resources and help text are not used to infer a version. Metadata
records the actual compiler version and runner image
version in `build-metadata.json`. The package script never downloads installers
itself. Other runner build tools are recorded, not claimed fully pinned.

`windeployqt` runs only against the app and UIA worker in a new product stage.
`--no-compiler-runtime` prevents bundling a redistributable EXE. App-local DLLs
come exclusively from Visual Studio's `VCToolsRedistDir` under
`x64\Microsoft.VC143.CRT`. `-MsvcRedistributableDirectory` may specify that exact
redistributable folder. Windows supplies UCRT. App-local CRT security updates
require rebuilding and redistributing WorkSidekick; check Microsoft's current
redistribution terms before public release.

Outputs in `build/packages`:

- `WorkSidekick-0.7.0-rc1-windows-x64.zip`
- `WorkSidekick-0.7.0-rc1-windows-x64-setup.exe`
- `windows-package-manifest.json`, `dependency-inventory.json`,
  `build-metadata.json`, and `SHA256SUMS.txt`

Qt license texts in `licenses/` are unmodified files from
[Qt qtbase v6.8.3 LICENSES](https://github.com/qt/qtbase/tree/v6.8.3/LICENSES).
The dependency inventory lists every deployed DLL, file version and SHA-256.
See `licenses/THIRD_PARTY.md` for dependency/source and redistribution review.
The candidate is unsigned; code signing is NOT CONFIGURED. No byte-for-byte
reproducibility claim is made.

## Runner delivery smoke

Only on a disposable GitHub-hosted Windows runner:

```powershell
./scripts/test-windows-package.ps1 -ArtifactDirectory build/packages -Version 0.7.0-rc1
```

This checks hashes, audits the extracted ZIP, removes development paths and Qt
environment overrides for launch, then installs, reinstalls, launches and
uninstalls. It verifies the helper's no-argument JSON response, launches the
installed normal primary, forwards 20 `--trigger` invocations and a second
normal launch, and verifies running-app uninstall is safely refused. After
stopping only its own primary, it verifies every manifest file is removed.
It checks synthetic provider/model, Profile, feature, automatic toolbar,
shortcut preferences and four distinct synthetic provider vault values remain
intact. Vault retention is checked through Windows APIs; a packaged-app
credential read is NOT TESTED. Provider access isolation is covered by the
separate native unit suite, not inferred from package retention checks.
It refuses to start if existing product settings, credentials, registration or
processes are found. It removes only its synthetic data after the test.

To test a real previous-version upgrade, additionally supply
`-PreviousInstallerPath C:\path\to\previous-setup.exe`. Without that input,
the report explicitly identifies same-version reinstall coverage and marks
previous-version upgrade NOT TESTED. `windows-package-smoke.json` records the
results. These checks do not prove clean-machine deployment or real desktop
application compatibility; those remain separate acceptance gates.

References: [Qt deployment](https://doc.qt.io/qt-6.8/windows-deployment.html),
[Microsoft local deployment](https://learn.microsoft.com/en-us/cpp/windows/deployment-in-visual-cpp?view=msvc-170),
[Microsoft redistribution terms](https://learn.microsoft.com/en-us/cpp/windows/redistributing-visual-cpp-files?view=msvc-170),
[Inno per-user privileges](https://jrsoftware.org/ishelp/topic_setup_privilegesrequired.htm).
