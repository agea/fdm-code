# Server gameplay settings

The host configures the existing gameplay through **Start Server → Gameplay
settings** (also available from Play Offline). Categories have basic and advanced
fields, units, numeric limits, a reset for each value/category, a Standard reset,
and named local presets. Authored weapon/kit defaults are shown until overridden.
An asterisk identifies an edited value. Preferences are saved for the next hosted
session; an active lobby or match exposes its server snapshot read-only through
**Server rules**. Clients cannot apply their own saved preferences to that server.

The settings cover match duration/score/READY threshold/intermission, locomotion,
stamina, jumping/traversal/posture times, health/armor/helmet/spawn protection,
respawn delay, friendly/self/fall damage, aim punch, firearm damage/precision/recoil,
weapon-specific numeric fields, existing kit quantities, HE and launcher blasts,
smoke, Claymores, knife damage/reach/cooldown, recon stock/range/replacement,
material resistance/blast transmission, production map rotation and generation seed.
Only utility already present in a kit is editable: configuration does not add
new equipment slots or new combat mechanics. No health regeneration, automatic
respawn, bot difficulty system or mine-count restriction was introduced.

## Runtime and saved data

`ERServerSettings.h/.cpp` provide a whitelist of typed numeric descriptors and
sparse `FERServerRules` overrides. Missing ordinary values use the Standard
preset; missing weapon/kit values use the actual authored asset/recipe. Unknown
keys, nonfinite numbers and out-of-range values are rejected or clamped. Counts
are integral. Validation orders health/traversal/fall thresholds, keeps the
Claymore sensor inside its damage cone and retains at least one production map.
Weapon tuning also clamps full-damage radii to the corresponding outer radii.

The host's saved draft and named presets live in the existing GameUserSettings
ini, under `RespawnAndRage.ServerRules`, `RespawnAndRage.ServerPresets` and
`RespawnAndRage.ServerPreset.<name>`. Starting an offline/LAN/Internet session
copies the draft to an immutable session snapshot. It survives lobby/map travel
and subsequent rounds. Joining another server never replaces the local draft.

Direct/dedicated launches can configure `[RespawnAndRage.ServerRules]` in Game.ini.
For example:

```ini
[RespawnAndRage.ServerRules]
Move.SprintDuration=12
Move.Recovery=18
Move.Run=15
Move.Sprint=27
Smoke.Duration=18
Smoke.Radius=6
Smoke.Height=4
Weapon.HE.Damage=120
Weapon.HE.ExplosionInnerRadius=2
Weapon.HE.ExplosionRadius=6
Claymore.DamageRadius=8
Claymore.TriggerPercent=25
Claymore.Damage=140
Kit.rifleman.offense.HE=3
```

Keys and accepted ranges are defined by `ERServer::Settings()`; the menu and
runtime use that same schema. Speeds use km/h, cloud/blast/reach sizes use metres,
and time values use seconds. Weapon radius overrides convert to Unreal cm.
Kit values are actual carried rounds/items, after the historical recipe doubling;
there is no additional hidden multiplier on a configured quantity. Magazine
overflow moves to reserve without losing ammunition.

`AERGameState::ServerRules` replicates the authoritative snapshot to initial and
late clients. Weapon actors retain the replicated original Data asset and derive
private effective copies through `Tuned()`, so authored assets and other worlds
remain untouched. Damage, fire/reload timing, projectiles, accuracy and owner HUD
use those effective values. Movement prediction and stamina HUD use the same
server durations/speeds. The resolved kit catalog carries configured quantities
and starting protection equipment.

Material catalogs are copied per map. Cell network protocol 10 includes the
material-rule snapshot inside the layout descriptor, so client reconstruction
uses matching HP/transmission before digest validation. The cell calibration,
support system and geometry remain unchanged. Depot stays excluded from server
voting and rotation; configured production-map filtering applies to buttons,
accepted votes, tie/no-vote selection and subsequent rounds.

## Claymore damage

The 28 October request supersedes the earlier guaranteed-kill rule. Every victim
uses the ordinary quadratic distance falloff, forward cone, cover checks and
armor pipeline. There is no health/armor-dependent damage uplift.

`trigger radius = damage radius × TriggerPercent / 100`.

Standard retains an 8 m damage radius and a 25% sensor (2 m), with a 60° sensor
inside the 110° damage cone. Trigger checks additionally require the target to
be inside the actual 3D blast radius. Changing damage/armor/radii may let a
full-health enemy survive activation; this is expected. Laser boundaries use the
replicated effective sensor, including on late clients. Placement phases scale
together with placement duration. Existing ownership, expiry, cover, protected
player immunity and cleanup behavior remain.

HE/launcher armor penetration likewise respects its configured value throughout
0..2; the previous hidden 1.6 minimum is removed. Standard authored 1.6 profiles
retain their previous damage. Player damage and structural blast remain separate.

## Validation

- `python3 scripts/test-server-settings.py`: schema/ranges/normalization, isolated
  preset round-trip, real asset-field overrides, intact source assets, kit
  capacity/supply, material calibration and movement/stamina prediction.
- `python3 scripts/test-server-settings.py --regressions`: the 21 focused native
  settings/loadout/ballistics/menu/player/recon/cell checks.
- `python3 scripts/test-server-settings.py --network`: separate listen host/client,
  a client with conflicting local ini values, configured movement/weapon/kit/smoke,
  a late initial cell-map snapshot with modified material resistance, replicated
  mine sensor and survival of a low-damage mine activation.
- `python3 scripts/test-server-settings.py --render`: actual Metal menu capture at
  `Saved/Testing/ServerRules-Menu.png`, plus Claymore and weapon pages.

Retain the existing loadout/mine, weapon ballistics, player movement and cell/network
regressions. The posthumous mine fixture uses a wounded armored victim so it tests
kill attribution without relying on the removed guaranteed-lethality behavior.
