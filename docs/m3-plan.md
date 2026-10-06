# M3 implementation plan
Baseline: dacde1e335a9bdcf3439209cd937a92dcf42f9ca. User-approved design: M3_SPEC.md.
Execute inline using TDD and verification-before-completion. Preserve all M0–M2 tests.

- [ ] Resolver and palette tests → observe RED in Ubuntu CI → implement.
  SelectionResolver owns no history; lazy callbacks choose AT-SPI, PRIMARY, Clipboard in order.
  Valid means non-whitespace and <=100000 UTF-16 units. ManualActionPalette owns value snapshot,
  displays source and first1000 characters, clears on hide and never reads providers or calls AI.
- [ ] Application manual route, fake shortcut/privacy integration → RED → implement.
  All explicit entrypoints call triggerManualActions; resolve before showing/activating.
  Busy shows informational empty palette; repeated trigger replaces snapshot; only feature click calls runSelection.
- [ ] X11 backend: dedicated worker/display, XGrabKey on roots with Caps/Num modifiers,
  poll X connection plus wake pipe (blocking, not timed polling), XUngrabKey on stop.
  Integration uses test-only XTest: activation, lock variants, collision and re-registration.
- [ ] Portal v1 asynchronous transport/state machine; runtime Properties.Get(version),
  CreateSession / BindShortcuts, request token subscription before call, exact session/id filtering.
  Explicit enable only, no startup permission UI even when preference persisted.
  Handle rejection, timeout, close, service loss, late replies and stop; test fake transport/state.
- [ ] QSettings shortcut/enabled defaultfalse; tray/settings sync; metadata diagnostics.
  xcb selects X11, wayland* selects portal; unsupported backends remain usable.
- [ ] InstanceCoordinator uses user-only runtime directory, QLockFile held for primary lifetime,
  QLocalServer/Socket bounded trigger/open protocol and acknowledgements; no text over IPC.
  Connect before lock, never unlink endpoint while another primary owns lock; fail closed on timeout.
  Tests cover subprocess trigger, normal second launch, startup race, stale socket, malformed command.
- [ ] Extend main --trigger and factory; keep smoke isolated; run Release/CTest and all existing integrations.
- [ ] Document three modes, GNOME manual custom shortcut, exact evidence / NOT TESTED matrix.
  Commit/push completed M3 and STOP; M4 not implemented.
