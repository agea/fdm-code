# Internet multiplayer with Epic Online Services

21 October 2026. The game now has an EOS Internet path alongside the existing
LAN path. The one-Mac service probe covers authentication, session discovery and P2P
listen/travel/lifecycle. An actual two-computer Internet match is still outside
the verified scope; the user requested testing on this Mac for now. See
TEST_RESULTS.md.

## Player flow

- **Start Server** → **Connection: Internet (Epic Online Services)** → choose
  human capacity and optional test bots → **Start Server & Enter Lobby**.
- **Play Online** → **Connection: Internet (Epic Online Services)** →
  **Search Servers** → select a compatible public server.
- Sign in with an Epic Games account when the browser opens. The game delegates
  authentication and token refresh to Epic's OSS; it does not collect passwords.
- Teams, cosmetics, equipment, map voting, strict-majority READY and authoritative
  gameplay use the existing lobby/gameplay code. The host must keep the game open.
- Choose **LAN** in either screen for the existing local UDP discovery/IP path.
  LAN and offline play need no EOS configuration or Epic login.

## Private build configuration

The observed portal product has a Live sandbox/deployment and a Peer2Peer client.
The non-secret product/sandbox/deployment IDs are in `Config/EOS.example.ini`.
Populate `Config/EOS.local.ini` with the IDs and **existing untrusted Peer2Peer
client secret**. This file is deliberately ignored by Git. Never put a trusted
server credential in the game or public history. The user explicitly authorized saving the existing secret at this exact path
and EOS testing. The file is now populated with mode0600. No secret is printed
in the probe summaries or included in Git-eligible files; do not repeat the
resolved approval question. The temporary non-secret document used while opening
the file was discarded, and the secret was replaced in the clipboard by the
non-secret client ID.

The runtime reads the private file only when the player requests Internet play.
It updates EOS settings in memory, selects the EOS subsystem explicitly and
leaves the default NULL subsystem available for local play. EOS IDs are accepted
by the engine's compatibility list; the host explicitly registers/unregisters
players with the EOS session. Client local-player IDs use the authenticated EOS
identity during Internet travel. The portal's Clients page lists the existing
Peer2Peer policy with Sessions,
and Epic Account Services lists the application as **Published**, permissions
**Configured**, and the existing RespawnAndRage client selected under linked
clients. These read-only checks establish the portal setup, not a successful
SDK login or match. No portal permissions were changed.

When the file exists **during compilation**, `fdm.Build.cs` stages it as a
NonUFS runtime dependency. Existing isolated desktop build helpers copy Config.
Rebuild after adding/removing the file so the staging manifest is current.
The distributed client necessarily contains its untrusted client credential;
Git exclusion protects the public source history, not the installed game.
Builds without the file explain that Internet play is not configured and keep
LAN functional. No new package or upload is included in this change.

## Network behavior

The implementation targets the installed UE 5.8.3 sources. `UERNetDriverEOS`
extends the current `SocketSubsystemEOS.NetDriverEOS`. Its listen path adds
`bUseIPSockets` for ordinary launches; only the explicit server travel option
`EROnline=1` enables P2P. Connections to IP addresses retain Epic's IP passthrough.
The obsolete `bIsUsingP2PSockets` config is intentionally absent. This preserves
direct-IP test launches as well as the normal LAN menu.

EOS **Sessions**, with join-in-progress and public advertisement, supply the
server browser; this is separate from the game's replicated READY lobby.
Sessions avoid imposing EOS Lobbies' smaller membership limit on the existing
configurable capacity. The requested capacity is sent unchanged to EOS; backend
validation/quotas still apply, and a rejected create operation reports failure.
The Exit Game button allows up to five seconds for session destruction before
requesting process exit; LAN discovery is suppressed during that interval.
Game/build-version filters exclude incompatible search results. Only the P2P
endpoint resolved from a successfully joined session is used for Internet travel.
The lobby-to-match travel preserves `EROnline=1`, and Internet hosts do not
advertise through the LAN discovery beacon. Exit, cancellation and failed travel
clean up the online session. Late asynchronous create/join completions after
cancellation destroy their session rather than travelling to it.

## Verification

```sh
./scripts/build-editor.command
python3 scripts/test-online-unconfigured.py # use an isolated unconfigured checkout
python3 scripts/test-online-service.py      # real EOS; browser login may be needed
python3 scripts/test-lobby.py
python3 scripts/test-network.py
python3 scripts/test-lobby.py --gallery
```

The missing-configuration probe boots the real game, attempts online hosting and
search, checks cancellation, then opens a real IP listen server with EOS plugins
enabled. The LAN lobby probe uses a host/two remotes and checks READY, voting and
loadout travel. Neither test certifies EOS service access or NAT traversal.

The service probe uses one Epic account to create a real public session, find
it through EOS, resolve its P2P endpoint, register the listen host, leave/destroy,
cancel an in-flight create, rehost and READY-travel to Depot. It requires both
PASS markers, confirmed session cleanup from the actual Exit Game path while
hosting, and process exit code0. A release-order failure found during the
first run is addressed by releasing the owned EOS instance in GameInstance
subsystem teardown, while EOSShared is still loaded; it does not suppress crash
checking. The probe declares its dependency on EROnline so its interface refs
are released before the owned EOS instance.

Next, verify host/search/join with distinct Epic accounts and two computers on
different Internet connections. Check account
portal consent, player counts/full admission, cancellation/retry, lobby travel,
late join, host loss, recon/destruction replication and leave/rehost. Until this
passes, do not call the Internet feature ready for distribution. EAS applications
in development may restrict access to authorized accounts; inspect app status
and permitted testers in the portal before distributing a build.

References: [Epic OSS EOS guide](https://dev.epicgames.com/documentation/en-us/unreal-engine/online-subsystem-eos-plugin-in-unreal-engine)
and the installed UE 5.8.3 `OnlineSubsystemEOS`/`SocketSubsystemEOS` source.
