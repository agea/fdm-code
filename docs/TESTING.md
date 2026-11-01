# Testing

Tests are Python launchers in `scripts/` around native automation (`EclipseRecoil.*`) and opt-in
probes that never spawn in normal play. Logs and captures go to `Saved/Testing`. Compile first
with `scripts/build-editor.command`. Report what really ran, on which platform and RHI.

## Evidence levels

| Level | Proves | Does not prove |
| --- | --- | --- |
| Compile on Mac | Mac Editor build | Linux or Windows: see [AGENTS.md](../AGENTS.md#cross-platform-c-rules) |
| Native automation, `nullrhi` | Logic, geometry, determinism | Rendering, audio, feel |
| Two-process probes | Replication, real inputs on one machine | Physical LAN, NAT |
| `--render` Metal captures | Visuals, after you inspect them | Other GPUs, FPS |
| Packaged runtime | Cooked boot and maps | Gameplay quality |
| Human playtest | Balance | |

Any process that drives a server must exit 0 as well as print PASS: shutdown crashes are real.
Two-process launchers call `settle_clients` from `client_shutdown.py` before terminating: SIGTERM
during the menu load that follows host exit crashes the client and leaves `CrashReportClient`
holding its sockets.

## Where commands live

Each topic doc ends with its own **Verify** section: run that one, not everything. Repo tooling:
`python3 -m unittest discover -s scripts/tests`, `verify-public-tree.py`.

Run native tests directly with `-ExecCmds="Automation RunTests EclipseRecoil.<Group>;Quit"`.
Saved-asset verifiers (`verify_*.py`) run through Unreal Python.

## Useful flags

`-ERInputAudit`, `-ERShotAudit`, `-ERAccuracyAudit`, `-ERAudioAudit`, `-ERPerformanceAudit` add
opt-in logs. Galleries such as `-ERHitscanGallery`, `-ERPostureGallery`, `-ERWoundGallery`,
`-ERTeamGallery`, `-ERReconGallery` are art fixtures, never enabled in ordinary play.
Audit and probe code stays out of Shipping.
