# Windows compatibility

Native Windows is deferred to M5. M0 shares core, AI, controller and UI sources;
CMake may build these libraries/tests on Windows but does not build the application
until a native secure-store backend is available.

| Application | UIA selection | Rectangle | Automatic detection | Clipboard fallback | Focus preservation |
|---|---|---|---|---|---|
| Notepad | NOT TESTED | NOT TESTED | Not implemented | Not implemented | NOT TESTED |
| Edge | NOT TESTED | NOT TESTED | Not implemented | Not implemented | NOT TESTED |
| Chrome | NOT TESTED | NOT TESTED | Not implemented | Not implemented | NOT TESTED |
| VS Code | NOT TESTED | NOT TESTED | Not implemented | Not implemented | NOT TESTED |
| Teams | NOT TESTED | NOT TESTED | Not implemented | Not implemented | NOT TESTED |
| Outlook | NOT TESTED | NOT TESTED | Not implemented | Not implemented | NOT TESTED |

Do not treat the development host being Windows as Windows product verification.
