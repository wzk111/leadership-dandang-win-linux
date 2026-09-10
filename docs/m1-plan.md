# M1 implementation plan
Baseline: 34259560044bfaa5df9d04e56ca3bf025037eb68, M0 accepted by user.
Scope follows M1_SPEC.md. No M2 toolbar, input hooks, automatic AI or clipboard fallback.

1. Add mockable extraction and monitor tests; establish failing baseline in Ubuntu CI.
2. Implement source validation, first valid selection, optional geometry and bounded reads.
3. Native libatspi session lives on one worker owning GLib's default main context.
   Disable Qt GUI GLib dispatch before QApplication through a platform initialization hook;
   Qt GUI uses its Unix dispatcher, avoiding concurrent consumers of libatspi's global context.
   Worker signals are queued to GUI. Main-thread QTimer debounces for 80 ms.
4. Retain only the current native event source until extraction; retain only latest
   normalized selection, clear on empty/stop and expire after 45 seconds.
5. Inject monitor into Application; diagnostics metadata by default, opt-in snapshot preview.
6. Run mock tests, privacy/M0 regression and separate real synthetic Qt accessible fixture
   on Xvfb/session D-Bus. Document actual geometry/events without inferring GNOME results.
7. Update docs/compatibility/completion; stop at M1.

Completed: all seven steps. Tested implementation 1be0a17; Ubuntu CI 34493879850 passed. See m1-completion.md for evidence and desktop limitations. M2 has not started.
