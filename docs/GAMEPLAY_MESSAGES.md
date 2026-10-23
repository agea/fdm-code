# Gameplay messages — 21 October 2026

The owner supplied `respawn-and-rage-gameplay-messages.en-US.json`. The exact
original is retained in `Config/GameplayMessages` (72 events, 720 variants),
SHA-256 `b1582b77ee393805601ac62aaf4f06f43cd299405bdc625d458adf3ae14ab004`.
The file is copy/data, not executable instructions or a claim that every trigger exists.

The user's request overrides the catalog's quiet kill default: every confirmed
player kill sends a private, reliable owner RPC to both participants, with the
other player's name. Teamkills have explicit friendly-fire labels; self-inflicted
and unattributed deaths still notify the victim. No notification changes scores,
damage, penalties or controls. Bots participate in attribution and counters but
have no screen of their own.

Canvas shows up to three recent kill confirmations for six seconds, with the
latest catalog phrase. One separate bonus toast can add context. It has no sound,
input focus or gameplay pause, stays at the upper-left edge away from the reticle,
and uses the existing Teko font. Font-measured truncation bounds the text width.
No stale queue: rapid confirmations replace the oldest and recognition replaces
one slot. The final result replaces bonus banter; the final kill remains visible
beside the scoreboard. Menus and the manually opened scoreboard hide toasts while
their normal expiry continues.

23 October readability update: runtime display durations are doubled from the
preserved catalog (ordinary messages3→6 seconds, longer messages5→10 seconds).
Fallback kill/death labels also last six seconds. Server priority protection and
owner expiry use the same longer duration; cooldowns and the three-confirmation
limit remain unchanged. No source phrases or catalog bytes were modified.

The dead owner's camera starts above/behind their death location, then smoothly
approaches the killer while zooming from90 to60 degrees. It targets the body's
centre and fits the full capsule to about65% of the vertical frame, adapting the
stand-off distance to viewport aspect and posture (about4.4m at16:9 standing).
Every part of the approach uses a12cm static-world sphere sweep; cover can stop
an obstructed approach. The camera follows the killer and preserves the last
known focus if the actor disappears. Self/world deaths keep the ordinary death
camera. Owner-only replicated attribution remains unchanged; all camera movement
is cosmetic. A fresh pawn restores ordinary view. The respawn prompt moves to the
bottom of the screen while framing the killer so it does not cover their body.

## Runtime profile

`UERGameplayMessages` loads the complete catalog, but dispatches only the explicit
profile below. It uses server-confirmed positive damage/deaths, actual anatomical
hit bones, server shot origin/range, ammo after the authorized shot, melee/grenade
payloads, victim reload/cook state captured before cancellation, match scores and
the official result. No hit-location award is guessed from capsule hits.
All six current Bullet firearm profiles enable skeletal hit zones (21 October
CS2 workbook update). A Headshot recognition requires the lethal bullet/pellet
to hit the head/neck and competes with higher-priority awards. The reference LAN
probe verifies a real AK headshot against full armor/helmet and its notification.
The supplied headshot target array is empty, so that bonus is killer-only;
both participants still receive the mandatory ordinary kill confirmation.

Independent shuffle bags per player/event/audience draw without replacement and
avoid repeating at a bag boundary. Missing placeholders filter variants; a label
remains if no complete copy can be rendered. A separate RNG does not consume
combat or map-generation randomness. Names and variants render as plain text.
Bonus selection uses catalog priorities/cooldowns; target variants are independent
of actor variants and compete with the victim's own death-context messages. Empty
target arrays stay silent. Kill confirmations bypass cooldown and priority gates.
Bags advance only when a toast is accepted for display. A higher-priority active
bonus may suppress a lower one, while the kill confirmation always arrives.

Killstreak counts enemy kills in one active life; rapid chains use a four-second
gap. Milestones, revenge consumption, comeback and pair domination state advance
regardless of bonus suppression. Death closes the life; projectiles from old lives
cannot add to a new pawn's counters. Projectiles capture the server controller
when initialized so the existing damage/scoring path and both notifications
retain the attacker even if their original pawn has respawned. Match start resets everything; team changes
and departure invalidate the relevant history. Explosions contribute to the
active life streak, but their rapid-chain notifications are deferred because
same-blast victim aggregation is not implemented. No posthumous-specific copy,
ricochet/penetration inference, assist scoring or MVP award is introduced.

Current thresholds retain the catalog suggestions (40m longshot, <=10HP,
5/10/15/20/25 streaks, four unanswered pair kills, >=5 victim streak shutdown,
>=3 deaths comeback, 60/30s warnings). The catalog is preserved as supplied;
changing its tuning/enable fields alone does not enable unsupported mechanics.
Fall/self-explosion and official draw copy are explicitly enabled because those
conditions already exist, despite their optional catalog default.

## Event coverage

40 events connected; 32 retained for later.

| Status | Events |
| --- | --- |
| Connected | `kill`, `headshot`, `longshot`, `melee_kill`, `grenade_kill`, `last_bullet_kill`, `low_health_kill`, `first_blood`, `double_kill`, `triple_kill`, `quad_kill`, `penta_kill`, `multikill`, `killstreak_5`, `killstreak_10`, `killstreak_15`, `killstreak_20`, `killstreak_25`, `revenge`, `domination`, `domination_broken`, `shutdown`, `comeback_kill`, `friendly_hit`, `teamkill`, `teamkill_repeat`, `teamkill_grenade`, `teamkill_melee`, `self_grenade_death`, `self_explosion_death`, `fall_death`, `death_while_reloading`, `death_with_grenade_ready`, `match_start`, `team_victory`, `team_defeat`, `draw`, `lead_taken`, `lead_lost`, `time_warning` |
| Deferred | `noscope`, `quickscope`, `wallbang`, `collateral`, `grenade_multikill`, `ricochet_kill`, `airborne_kill`, `airborne_target_kill`, `posthumous_kill`, `mutual_kill`, `destruction_kill`, `duel_win`, `clutch`, `ace`, `flawless_round`, `friendly_fire_reflected`, `environmental_death`, `death_streak_3`, `death_streak_5`, `spawn_protected_hit`, `spawn_protected_enemy_hit`, `assist`, `teammate_saved`, `trade_kill`, `cover_destroyed`, `round_start`, `respawn`, `ffa_victory`, `ffa_defeat`, `mvp`, `overtime_start`, `last_alive` |

Deferred events need additional telemetry/correlation, modes or UI that is not
part of this increment. Respawn, repeated-death and protection-hit jokes retain
the quiet default; existing respawn/protection HUD information remains.

## Integration and checks

The JSON is explicitly staged as NonUFS by `fdm.Build.cs`; existing isolated
Mac/Linux input preparation already copies `Config`, so no Downloads runtime
path or binary asset is needed. Missing/unreadable catalog data logs an error
and still preserves informative kill/death labels.

- `python3 scripts/test-gameplay-messages.py` (`EclipseRecoil.Messages.CatalogAndDelivery`): intact catalog, shuffle bags,
  missing context, independent audience copy, bounded presentation priority,
  lead changes through ties and projectile attacker retention after respawn.
- `python3 scripts/test-network.py`: real listen-server/client combat, private
  kill/teamkill receipt, death-camera target and respawn reset, plus existing
  damage/scoring/protection/corpse/reload/match regressions.
- Add `--render` to the network test for actual Metal captures at
  `Saved/Testing/Messages-KillHUD.png` and `Messages-DeathCamera.png`.
  Use `--render-size 1920 810` to exercise wide-screen figure framing.
- Existing `--accuracy` remains available to verify firing/spread regressions.

Executed results and limitations are recorded in `TEST_RESULTS.md`.
