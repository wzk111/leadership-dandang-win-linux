# M0 implementation plan

Goal: explicit clipboard → feature → asynchronous OpenAI Responses → copy result.
The supplied PROJECT_SPEC.md is the approved product and architecture design.

Architecture: Qt Widgets tray and windows use AppController; core owns feature
metadata, prompts and non-secret settings; IAIProvider isolates networking;
ISecretStore isolates Linux libsecret. No unused M1–M5 implementations.

Tech: C++20, CMake, Qt 6 Widgets/Network/Test/Concurrent, libsecret.

1. Save exact original specification, README and ignore rules.
2. Add Ubuntu 22.04 CI and behavioral tests for prompts, settings, response/error
   parsing, cancellation, timeouts, overlap protection and result copying.
3. Run tests against minimal stubs to observe expected assertion failures.
4. Implement core, asynchronous networking and secret backend; rerun tests.
5. Connect tray, settings, clipboard feature picker, result and diagnostics.
6. Exercise UI under Xvfb and an ephemeral D-Bus/keyring session. Never use real
   private clipboard text or credentials in tests.
7. Document exact commands, evidence and untested desktop/live API behavior.

Environment: Windows host; WSL2 cannot start (virtualization unavailable).
Use GitHub Actions Ubuntu 22.04 for build/test evidence. Do not change BIOS,
Windows features, or existing WSL distributions. Real GNOME X11/Wayland and
live paid API checks remain distinct from automated headless validation.
