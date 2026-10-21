# Bullet aim punch and damage feedback

Implemented from the owner's 21 October 2026 attachment
`RESPAWN_AND_RAGE_AIM_PUNCH.md`, plus the requested red directional damage arc
and red screen-edge flash. Current source already has real worksheet firing
recoil; the attachment's description of that recoil as cosmetic was outdated.
This change supersedes only the old cosmetic-only **bullet damage** rule.

## Damage and effective aim

`AERCharacter::TakeDamage` uses the existing authoritative weapon, point-damage
bone, range/zone modifiers and `UERHealthComponent::ReceiveDamage`. A surviving
victim receives an impulse only after HP decreases. Armor mitigation is detected
from actual armor consumption in that same call. Head status comes from the real
point-hit bone, including the existing helmet rules. Friendly fire follows the
same accepted damage path. Protected, rejected, zero, lethal and post-death hits
cannot add gameplay punch. Melee, blast, fall and stun keep their prior gameplay.

All gameplay defaults live in `FERAimPunchTuning`, editable on the character:

| Setting | Default |
| --- | ---: |
| Base pitch / actual HP lost | .40° + .012°/HP |
| Head multiplier | 1.5 |
| Armor mitigation multiplier | .6 |
| Lateral proportion | .35 |
| Pitch / yaw caps | 2.5° / 1.5° |
| Recovery half-life | .20s |

The incoming server-authorized travel direction is negated to find the attacker's
side relative to current effective facing. Nearly frontal shots use a stable
server event-sequence fallback. No client can supply a punch or shot vector.

The first pellet from an accepted weapon shot adds the base term; further
accepted pellets from that shot add only their actual-HP term, with each pellet's
own bone/armor facts. Every notification contains an absolute accumulated state,
so owner presentation never adds a pellet's disturbance twice.

`GetEffectiveAimRotation` combines ordinary base input, existing firing recoil
and the independently decaying damage pitch/yaw. `GetViewRotation` delegates to
it for the camera and attached first-person rig. `TryFire` explicitly uses it
before the unchanged muzzle convergence, obstruction and spread paths, including
all six firearms, optical modes and the launcher. Hand throws use the same
effective facing while retaining the existing launch velocity/overhead arc formula. Bots use that same final
accessor even when AI updates base facing each frame. The capsule, hitboxes,
ControlRotation and spread parameters are unchanged. The old cosmetic bullet
pitch is removed; translation and roll remain cosmetic. Other damage retains
its prior pitch response.

## Recovery, ownership and network latency

Authority and owner mirror are separate pawn states. Each sample evaluates
`angle * pow(.5, elapsed / halfLife)` against server time; sampling at fire time
avoids tick-order and frame-rate-dependent decay. Hits add to the recovered state
and clamp both axes. Very small offsets snap to zero. Mouse input still controls
the base rotation, allowing immediate counter-steering.

The existing reliable owner-only `ClientDamageFeedback` now carries the absolute
post-hit state, sequence, timestamp and half-life. New snapshots replace the
mirror; duplicates/older sequences are ignored, including sequence wrap. A delayed
snapshot is aged on receipt, rather than replaying a fresh impulse. Presentation
uses the newest replicated GameState time sample, anchored at its receipt, rather
than the engine average that can lag behind loading hitches. It adds half the
measured player RTT to that delayed time sample as an
approximation of one-way transit, without changing server state. An owner
receipt-time floor and a last-sample watermark keep recovery monotonic if the initial GameState clock
is temporarily behind after loading; that fallback cannot infer unknown transit
time and starts no earlier than the received authoritative timestamp. Other viewers
receive no locally owned first-person disturbance. Death and possession reset
state and visuals; destruction/respawn/round restart create a fresh pawn. Death
camera ownership retains its existing implementation and suppresses the overlay;
HUD feedback is drawn only when the controlled pawn is the current view target.

There is an unavoidable notification interval: the server can apply damage before
the owner's hit RPC arrives. Shots in that interval use the authoritative punch,
while the owner has not yet seen that hit. This implementation never delays
accepted damage or trusts a client-selected offset to conceal that interval.
The dedicated newest-sample clock leaves all existing recoil/match clocks unchanged.
Timestamped absolute snapshots minimize stale recovery after arrival; they cannot
provide perfect synchronization before arrival. See measured results in
[TEST_RESULTS.md](TEST_RESULTS.md). The local two-process tests are not a physical
two-computer LAN test.

## Red feedback

`ERDamageHUD.cpp` draws a small two-line red arc around the center in the attacker's
bearing: front above, right to the right, rear below, left to the left. It follows
view yaw as the player turns, with up to four recent directions and a .65s fade.
A separate .45s feathered red edge ring leaves the center clear. Both are triggered
by actual owner damage feedback; no persistent low-HP tint or new art assets.
Falls have an edge flash but no arbitrary compass direction. HUD/UI, menu and
scope behavior remain in the existing Canvas path.

## Verification

- `python3 scripts/test-aim-punch.py --automation`
- `python3 scripts/test-aim-punch.py`
- `python3 scripts/test-aim-punch.py --lag 100`
- `python3 scripts/test-aim-punch.py --render`
- Existing `test-network.py`, `--accuracy`, `--recoil`, `test-arsenal.py` and
  reference, movement and grenade regressions as recorded in TEST_RESULTS.

The opt-in `ERAimPunchProbe` is not spawned in ordinary matches and its mutation
logic is excluded from Shipping builds. Screenshots and raw test logs are under
`Saved/Testing`; no binary content migration is required. The probe uses a clear
replicated transient floor and an owned-controller readiness ACK after stable rendered frames,
so initial skeletal/material loading cannot silently skip its observation phases.
