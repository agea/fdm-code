# Internet multiplayer with Epic Online Services

22 October 2026. The game now has an EOS Internet path alongside the existing
LAN path. The one-Mac service probe covers authentication, session discovery and P2P
listen/travel/lifecycle. An actual two-computer Internet match is still outside
the verified scope; the user requested testing on this Mac for now. See
TEST_RESULTS.md.

## Player flow

- **Start Server** → **Connection: Internet (Epic Online Services)** → choose
  a server name, human capacity and optional test bots → **Start Server & Enter Lobby**.
- **Play Online** → **Connection: Internet (Epic Online Services)** →
  **Search Servers** → select a compatible public server.
- Internet play signs in automatically with EOS Connect Device ID. No Epic
  account, launcher login, browser consent or password is required. EOS stores
  the guest credential locally for the operating-system user and reuses it on
  subsequent launches. This identity is specific to the device/OS profile;
  moving it to another computer is outside this implementation.
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
P2P and the existing deployment. Earlier EAS portal checks are historical: the
guest path sets bUseEAS=false and uses Connect only. No portal permissions
were changed for this extension.

When the file exists **during compilation**, `fdm.Build.cs` stages it as a
NonUFS runtime dependency. Existing isolated desktop build helpers copy Config.
Rebuild after adding/removing the file so the staging manifest is current.
The distributed client necessarily contains its untrusted client credential;
Git exclusion protects the public source history, not the installed game.
Builds without the file explain that Internet play is not configured and keep
LAN functional. No new package or upload is included in this change.

## UE plugin extension

UE 5.8.3's stock OSS EOS accepts DeviceIdAccessToken as a credential type but
does not create the local Device ID or supply the required desktop
UserLoginInfo. Its Connect-only automatic refresh also asks the platform OSS
for an external token, which is unsuitable for Device ID.

`scripts/prepare-eos-guest.py` generates a project override under the ignored
`Plugins/OnlineSubsystemEOS/` from the **locally licensed UE 5.8.3 installation**.
It checks the exact engine version and SHA-256 of both patched engine files,
applies the reviewed public-slot mapping correction and adds the project extension snippets from `scripts/eos-guest`, and leaves
the installed engine untouched. Epic's copied sources and build outputs must
never be added to the public repository. Preparation is repeatable and preserves
unchanged file timestamps and compiled outputs. Build/editor entry points and
isolated Mac/Windows/Linux build preparation invoke it automatically. For
manual UBT/IDE builds, run it first (use `--engine` or `UE_ROOT`).

The extension creates or reuses the Device ID, provides the sanitized in-game
nickname, uses a null external token, and lets OSS perform normal Connect
login/CreateUser and maintain its user mappings for Sessions/P2P. Auth expiration
repeats Device ID Connect login without consulting NULL or EAS. It never deletes
the Device ID. A development-only probe entry point exercises that same handler.
The game requires the generated module definition at compilation, preventing a
silent fallback to the incomplete stock login path.

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
The browser advertises the chosen server name (trimmed, at most 48 characters,
with a default for empty input) and persists it with server settings. EOS create
and search explicitly share the RespawnAndRage bucket rather than inheriting
Unreal's engine changelist bucket; RR_VERSION still excludes incompatible game
versions. Update both host and client: older builds use the implicit bucket.
Game/build-version filters exclude incompatible search results. Only the P2P
endpoint resolved from a successfully joined session is used for Internet travel.
The lobby-to-match travel preserves `EROnline=1`, and Internet hosts do not
advertise through the LAN discovery beacon. Exit, cancellation and failed travel
clean up the online session. Late asynchronous create/join completions after
cancellation destroy their session rather than travelling to it.

## UE 5.8.3 discovery correction (26 October)

A real simultaneous host/browser reproduced the empty list: EOS search returned
one result while the game's filtered results contained zero. In stock
`FOnlineSessionEOS::AddSearchResult`, EOS public capacity and open public slots
are assigned to Unreal's private fields. `NumOpenPublicConnections` stays zero,
so the browser's full-server filter drops every session. The preparation script
checks the session source SHA-256 and corrects those two assignments in the
ignored project plugin. The installed engine stays unchanged.

The same service run also returned HTTP 403 / `policy_missing_action` for
`matchmaking:managePlayers`. Local RegisteredPlayers does not prove that backend
registration succeeded. With explicit user approval, the portal policy `p2p`
was changed from the read-only Peer2Peer template to Custom, preserving its
existing features/actions and User required condition and adding only
`matchmaking:managePlayers`. Guest Device ID still satisfies authentication;
no Epic account or EAS is required. The immediate first registration still
received the old policy error, but subsequent hosts and a fresh full lifecycle
launch completed without that denial. The first log therefore still makes the
script's aggregate policy warning INCOMPLETE; the clean relaunch is separate
evidence. Do not claim successful remote admission from this same-device test.

## Verification

```sh
./scripts/build-editor.command
python3 scripts/test-online-unconfigured.py # use an isolated unconfigured checkout
python3 scripts/test-online-service.py      # real EOS; no account/browser; two launches
python3 scripts/test-lobby.py
python3 scripts/test-network.py
python3 scripts/test-lobby.py --gallery
```

The missing-configuration probe boots the real game, attempts online hosting and
search, checks cancellation, then opens a real IP listen server with EOS plugins
enabled. The LAN lobby probe uses a host/two remotes and checks READY, voting and
loadout travel. Neither test certifies EOS service access or NAT traversal.

The service probe runs the real EOS lifecycle twice with the same OS user.
It checks that the identity has a Product User ID and **no Epic Account ID**,
creates a public session, finds its own advertised session, resolves its P2P
endpoint, registers the host, leaves/destroys, cancels an in-flight create,
rehosts and READY-travels to Central Building. It also invokes the same refresh handler
used by EOS auth expiration and requires a successful real Connect response.
This tests refresh directly; it does not wait for natural token expiration.
The second process must reuse the Device ID and the same hashed Product User ID.
No Device ID is deleted or reset, and existing Epic persistent auth is untouched.

Both launches require PASS, cleanup from Exit Game while hosting and process
exit0. The probe releases interface refs before the owned EOS instance, which
is destroyed while EOSShared is still loaded. A previous real SDK shutdown
crash is covered by this process-exit requirement.

The service script also starts a simultaneous second game process using the
normal browser search and a deliberately different Unreal BuildIdOverride. It
must find the named host, proving discovery does not depend on the engine build
ID. Both processes retain the existing same-device guest identity; this checks
server visibility but intentionally does not attempt JoinSession as that user.

The current test scope remains this Mac. Verify host/search/join with distinct
Device IDs on two computers on different Internet connections before claiming
cross-network multiplayer validation. Check player counts/full admission,
cancellation/retry, lobby travel, late join, host loss, recon/destruction
replication and leave/rehost. Two processes under the same OS profile reuse one
Device ID and are not a substitute for distinct remote players.

References: [Epic OSS EOS guide](https://dev.epicgames.com/documentation/en-us/unreal-engine/online-subsystem-eos-plugin-in-unreal-engine)
and the installed UE 5.8.3 `OnlineSubsystemEOS`/`SocketSubsystemEOS` source.
