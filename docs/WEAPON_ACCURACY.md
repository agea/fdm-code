# Weapon accuracy from the supplied CS2 workbook

The 21 October request replaces custom accuracy tuning on Pistol/SMG/Shotgun/
Assault/Carbine/Sniper with their deagle/mp9/xm1014/ak47/scar20/awp rows in the
owner-supplied March18,2026 XLSX. The complete original remains in private weapons
SourceAssets/References. Public values, exact source cells and cached comparison
results are in `references/cs2-weapon-ballistics-2026-03-18.json`.

## Source values and units

`FERWeaponAccuracyMode` stores the original worksheet numbers for Spread,
InaccuracyStand/Crouch/Move/Ladder/Jump/JumpInitial/JumpApex/Land/Fire, initial and
final stand/crouch recovery times and MaxPlayerSpeed. A second profile applies
the worksheet's Alt values when an optical sight is active; missing Alt values
inherit the corresponding hip value. Both optical zoom levels share that Alt
profile, retain their existing40/10 FOV and retain the authored scope controls.
AK iron sights have no alternative accuracy row and keep the hip profile.

Raw offsets are tangent-plane magnitudes scaled by1000. The enclosing half-angle
is `degrees(atan((Spread + Inaccuracy) / 1000))`; Analysis AO reports twice this
value. Add offsets before converting, rather than adding individually converted
angles or interpreting raw numbers as degrees. The server samples independent
weapon-spread and player-inaccuracy offsets; the HUD projects the same enclosing
angle. Every shotgun pellet uses these components; the former3.2-degree radial
rosette is replaced. No current firearm uses the old10-degree cone/bloom caps.

## Posture, motion and recovery

- The capsule's62.5..92.5cm half-height interpolates crouch and standing values.
  Prone uses crouch without an additional accuracy bonus, because the worksheet
  supplies no prone row.
- Horizontal velocity is measured in cm/s. One Source unit is2.54cm; motion
  interpolates linearly from zero to InaccuracyMove at the active profile's
  MaxPlayerSpeed, bounded at that endpoint. The spreadsheet specifies standing
  and full-running values, not intermediate movement curves. Existing player
  walk/run/sprint speeds are preserved; current firearms use this source-scaled
  motion coefficient in place of the separate custom sprint penalty.
- Ladder base is `2 * InaccuracyLadder`, as in Simple Y, including stationary
  rungs. Existing equipment stowing and ladder movement remain authoritative.
- Airborne inaccuracy adds Jump and interpolates JumpApex..JumpInitial with
  `clamp(abs(vertical_cm_s) / project_jump_velocity, 0, 1)`. This matches the
  worksheet apex and uses the existing project jump for the intermediate motion;
  the workbook does not supply a time curve for this project's jump physics.
- Landing captures the actual downward velocity before movement clears it,
  converted to Source units/s. Its impulse is `Jump + Land * downward_speed`,
  decaying with the source recovery time. This matches the sheet's Simple AA
  sample at302 units/s and333ms, including scoped AWP's separate Land value.
  All inventory guns receive the event, so switching cannot erase its history.
- Each authorized trigger shot adds InaccuracyFire once, including one impulse
  per shotgun shell. Recovery is `error * 0.1^(elapsed / RecoveryTime)`, without
  an artificial delay or maximum bloom. Crouch/standing choose their own times.
  AK interpolates initial/final recovery by1/3 and2/3 before its fourth/fifth
  burst bullets, reaching final recovery for the sixth, as in Analysis L/M and
  the cached firing tables. A fresh trigger starts the initial burst schedule
  while retaining any error that has not recovered. Rejected shots add nothing.
- Server snapshots preserve the current error before changing its recovery
  coefficient during posture transitions. The owner extrapolates those snapshots
  on the synchronized server clock. Changing stance cannot resurrect recovered
  error; no client-selected aim/accuracy RPC was added.

The source cycle times also apply (.225s deagle and1.455s awp rather than rounded
CSV values). Damage, armor, hit zones, cover HP subtraction, ammunition, models,
animations and existing aim controls remain on their established paths. Knife
and explosive utility have no matching accuracy rows and retain their project
profiles. The workbook defines values/envelopes, not CS2's exact seed sequence;
the server retains its own deterministic random stream.

## Rebuild and verification

Subsequent recoil alignment is documented in [WEAPON_RECOIL.md](WEAPON_RECOIL.md).
Recoil now affects the shared view/authoritative aim separately from these
accuracy offsets; its decay is not the sheet's inaccuracy recovery curve.

Read-only `extract-weapon-ballistics.py <original.xlsx>` extracts and checks
cached worksheet values without evaluating workbook code. Through Unreal,
`setup_weapon_accuracy.py` changes only six existing data assets, accuracy fields
and source firing intervals. The full art importer also assigns these profiles
for future rebuilds; numeric updates do not require reimporting artwork.

`EclipseRecoil.Weapons.WorksheetAccuracy` compares saved data and calculations to
108 cached burst,36 tapping and36 movement/posture results, checks unit conversion
against Analysis full angles and samples4096 deterministic rays inside the HUD
envelope. `test-network.py --accuracy` checks real movement/jump/landing, all six
firearms and both optical profiles, continuous recovery across stance changes,
actual sampled rays, remote firing and owner clock recovery. `test-scopes.py`
checks nonzero scoped minima and source-based moving/posture accuracy through
real owner inputs. `test-reference-weapons.py` retains actual hits/headshot,
six-pellet, damage, armor and private message coverage. Executed results and any
limitations belong in `TEST_RESULTS.md`.
