# Dependency and redistribution inventory

This is a basic inventory, not a complete SPDX SBOM. Exact build versions and
commit appear in each package's build-info file and the CI logs. Files and hashes
are inventoried beside the artifacts. Candidate artifacts are unsigned.

| Component | Distribution | License / source |
|---|---|---|
| Qt Core, GUI, Widgets, Network, Concurrent, platform/TLS/image plugins | Windows shared DLLs; Ubuntu distribution dependencies | LGPL-3.0 / GPL alternatives, module-specific notices; https://code.qt.io/cgit/qt/qtbase.git/ and https://www.qt.io/licensing/open-source-lgpl-obligations |
| Qt SVG plugin, if selected by windeployqt | Windows shared DLL | Qt module notices; https://code.qt.io/cgit/qt/qtsvg.git/ |
| MSVC 2022 C/C++ runtime | Windows app-local redistributable DLLs | Microsoft Visual Studio redistribution terms; https://learn.microsoft.com/cpp/windows/redistributing-visual-cpp-files |
| Inno Setup 6 | Installer tooling; not a runtime service | Inno Setup License; https://jrsoftware.org/files/is/license.txt |
| GCC / libstdc++, glibc | Ubuntu linked system dependencies | Distribution copyright files, GCC runtime exception / LGPL |
| libsecret, GLib, AT-SPI, D-Bus, X11, Qt Wayland | Ubuntu package dependencies | Distribution copyright and source packages; respective LGPL/GPL/MIT licenses |
| CMake / CPack, Ninja | Build/package tooling only | BSD-3-Clause / Apache-2.0 |

Qt is dynamically linked. Users may replace compatible Qt DLLs, debug changes
to those libraries and obtain the corresponding Qt source from the links above
(use the exact version in build-info). Bundled Qt license texts accompany Windows
packages. Do not remove upstream notices. Ubuntu uses distribution packages and
their installed copyright/source information rather than bundling system libraries.

Before public production redistribution, review the actual bundled plugin
manifest, Qt module third-party notices, corresponding source availability and
Microsoft redistributable rights under the distributor's license. This candidate
does not claim legal certification or byte-for-byte reproducibility. Code signing
is NOT CONFIGURED. No certificate is purchased or generated automatically.
