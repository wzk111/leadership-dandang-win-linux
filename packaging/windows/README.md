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
WorkSidekick before upgrading; the installer also uses Windows Restart Manager
to detect files in use. Its AppId remains stable across versions. Upgrade keeps
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
**6.8.3 MSVC 2022 x64** on PATH, Python 3, CMake and a preinstalled **Inno Setup 6.3
or later in major version 6**:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
./scripts/package-windows.ps1 -BuildDirectory build -ArtifactDirectory build/packages -Version 0.7.0-rc1
```

`ArtifactDirectory` must be empty. The script does not download or run a runtime
installer. `-InnoCompiler` can point to an existing `ISCC.exe`. CI uses the Inno
Setup already installed in the `windows-2022` runner image and records its actual
version and runner image version in `build-metadata.json`. This is a documented
runner-provided tool dependency, not a claim of a fully pinned binary toolchain.

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
uninstalls. It checks synthetic provider/model, Profile, feature, automatic
toolbar, shortcut preferences and four synthetic vault entries remain intact.
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
