# M6 stability evidence

Date: 2026-11-02. Short repeatable stress, not a multi-day soak or leak certification.

## Repeatable scenarios

`test_stability` runs the shared controller/UI with four real provider protocol
implementations and loopback-only synthetic HTTP; native selection/shortcut
signals in that scenario are simulated. Per provider: 1,000 selection signals,
500 ActionBar cycles, 200 shortcut/palette cycles, 100 monitor restarts and local
Reply opening, with zero HTTP and zero credential reads before an explicit action.

Across four providers: 100 measured HTTP cancellations (25 each), four warmups,
and four successful recovery requests. Actual QNetworkReply QPointers are observed
and must clear; provider descendant counts are not used. Eight shutdown-signal
cases cover pending credentials/HTTP, cancellation, bounded cleanup and late secret
suppression. This is the production aboutToQuit connection, not full OS process exit.

IPC: 100 measured simultaneous subprocess startup races and crash-recovery
restarts, 20 rapid forwarded triggers, malformed commands and incomplete-client
teardown. Cross-user/session isolation and forwarding exactly during primary exit
remain NOT TESTED; original protocol/ACL/deadline tests remain separate gates.

Windows: 100 actual hook install/remove cycles; 100 native RegisterHotKey cycles
with 200 synthetic WM_HOTKEY messages; ten helper timeout/cancel/recovery cycles
and pending-helper shutdown. GUI heartbeat and child-process reaping are asserted.
This does not establish all third-party providers' hung-COM behavior.

Linux: 1,000 simulated AT-SPI events coalesce to 100 extractions, 100 monitor
restarts, stale-reply rejection and pending-stop checks. Native X11 registration
cycles and 100 actual AT-SPI listener register/deregister cycles run separately on
a controlled accessibility bus. Xvfb is not GNOME. Bus disappearance and target
exit mid-extraction remain NOT TESTED; failed initialization requires restart.

## Recorded resources and measurement limits

The `dc9b614` Windows CI artifact records the following representative batches
(raw logs: `.local/m6-native-current/test-results/`):

| Scenario | Warm baseline → end | Interpretation |
|---|---|---|
| Native hooks, 100 cycles | private 2,064,384 → 2,064,384 B; handles 182 → 182; threads 5 → 5 | Stable across sampled batches |
| Native hotkeys, 100 cycles | private 2,064,384 → 2,064,384 B; handles 182 → 182; threads 5 → 5 | Stable across sampled batches |
| Helper timeout/cancel/recovery | handles 190 → 192 by batch 2, then 192 through 10; threads 7 → 7 | Small initial cache growth, no continued sampled growth |
| IPC race/restart, 100 cycles | private 9,129,984 → 9,158,656 B, plateau from 60; handles 673 → 673; threads 11 → 9 | No sustained growth in this bounded scenario |
| OpenAI synthetic ActionBar, 500 cycles | private 8,241,152 → 8,404,992 B; handles 652 → 653; threads 11 → 11 | Small allocation/cache change, not a zero-leak claim |

CI controlled suites report zero failures/crashes/hangs in the passing runs;
forced-primary termination is an intentional test, not a crash defect. Dedicated
COM allocation, every D-Bus subscription and multi-hour process accounting remain
NOT TESTED. Inspect the complete batch logs rather than inferring leak freedom
from a before/after pair.

Local Windows 11 idle sample: 10.034 seconds, 12 logical processors; machine CPU
0.182% (one-core equivalent 2.024%). RSS 52,199,424 → 51,912,704 B; private memory
37,408,768 B, handles 1,164 and threads 9 unchanged. The app displayed a local
preview. This short sample is not an idle-CPU guarantee.

One synthetic OpenAI row: 500 samples selection-to-visible P95 0.589 ms;
200 samples shortcut-to-palette P95 13.182 ms. Boundaries exclude native input,
accessibility provider and window manager. Real-desktop native P95 targets remain
NOT TESTED and must not be inferred from these numbers.

Local opt-in native policy/runtime verification and exact final CI evidence are
recorded in the completion report. No paid API calls, private text or real keys
are needed by these scenarios.
