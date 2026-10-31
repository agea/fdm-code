# Online

**Code:** `EROnline`, `ERNetDriverEOS`, `scripts/prepare-eos-guest.py`, `scripts/eos-guest/`.

## Sessions

Offline, LAN and Internet share the same lobby, teams, voting and gameplay. The host keeps
the game open; there are no dedicated matchmaking servers.

- **LAN:** UDP discovery or direct IP, port 7777, bound to `0.0.0.0` through MULTIHOME. Needs
  no EOS configuration. Exercised on one Mac only: a physical two-PC LAN needs its own test.
- **Internet:** Start Server → Connection: Internet (Epic Online Services), or Play Online →
  Search Servers.
- **Capacity:** Start Server saves a positive human capacity (default 16, host included), with
  no fixed cap. GameSession, admission, LAN availability, lobby status and travel all use it.
  Test bots are additional. Occupied starts overflow to safe positions near the base, waiting
  when no space exists.

All peers must match protocol and build. `RR_VERSION` filters incompatible results.

## EOS guest identity

Internet play signs in with **EOS Connect Device ID**: `bUseEAS=false`, no Epic account,
launcher, browser or password. The credential is stored per OS user, and the same process
reuses the same hashed Product User ID. Never delete or reset a Device ID for a test.

Configuration: non-secret IDs in `Config/EOS.example.ini`; private `Config/EOS.local.ini` holds the
**untrusted Peer2Peer client** secret (ignored, mode 0600, never printed). It is staged as NonUFS
when present at compile time, so rebuild after adding or removing it. Builds without it keep LAN
and explain that Internet is not configured. Never ship a trusted server credential. The portal
policy `p2p` includes `matchmaking:managePlayers`.

## Plugin override

UE 5.8.3's stock OSS EOS cannot create a Device ID or supply desktop `UserLoginInfo`, and its public
slot mapping hides results from the browser. `scripts/prepare-eos-guest.py` generates an ignored
project plugin from the **locally licensed** engine, with version and source-hash guards, plus
extensions from `scripts/eos-guest`. The installed engine stays untouched. Never commit
`Plugins/OnlineSubsystemEOS`. Build, open and packaging helpers run it automatically.

## Networking

`UERNetDriverEOS` extends `NetDriverEOS`: IP passthrough for ordinary launches, P2P only with
`EROnline=1`, including lobby travel. Do not restore `bIsUsingP2PSockets`. EOS **Sessions** (not
Lobbies) provide the public browser, with a shared `RespawnAndRage` bucket, so capacity follows the
setting. Server names are trimmed to 48 characters. Exit Game waits up to 5s for session destruction.
`EROnline` releases its world-specific OSS instance while EOSShared is still loaded, after net
drivers shut down. Late async completions after cancel destroy their session.

## Not verified

Real distinct-device `JoinSession`, two-network NAT or relay, and natural token expiry. Two
processes on one OS profile share a Device ID and do not replace remote players. Keep
same-Mac service results separate from cross-network claims.

## Verify

`test-online-service.py` (real EOS, two launches, must exit 0), `test-online-unconfigured.py` in an isolated checkout,
`test-lobby.py`, `test-capacity.py`.
