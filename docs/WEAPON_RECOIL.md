# Worksheet recoil — 21 October 2026

The same complete March18,2026 workbook is preserved privately under weapons
SourceAssets/References. Exact values and source cells are in
`references/cs2-weapon-ballistics-2026-03-18.json`, extracted read-only by
`scripts/extract-weapon-ballistics.py`. Raw Values AI/AJ/AK hold angle variance,
magnitude and magnitude variance; AT/AU/AV hold optical alternatives. Missing
Alt cells inherit the hip value. Simple AE/AF/AG confirm all eighteen hip values;
AH supplies the Random/Set Pattern classification.

| Current slot | Worksheet row | Hip magnitude ± variance | Angle variance | Optical magnitude ± variance | Pattern |
|---|---|---:|---:|---:|---|
| Pistol | deagle | 48.2 ±18 | ±60° | same | Random |
| SMG | mp9 | 21 ±1 | ±70° | same | Set Pattern |
| Shotgun | xm1014 | 80 ±20 | ±20° | same | Set Pattern |
| Assault Rifle | ak47 | 30 ±0 | ±70° | same | Set Pattern |
| Sniper Rifle (Carbine asset) | scar20 | 31 ±4 | ±30° | 31 ±4 | Set Pattern |
| Marksman Rifle (Sniper asset) | awp | 78 ±15 | ±20° | **25 ±2** | Random |

Both optical levels use Alt; iron sights use hip. No extra crouch/prone recoil
multiplier is applied because the workbook supplies none. Movement/posture
accuracy remains independently governed by the worksheet accuracy profiles.

## Explicit project adaptations

The workbook contains recoil parameters, not a CS2 spray coordinate table,
recoil seeds, angle integration units or return/decay settings. Consequently
this implements its exact parameters in Respawn & Rage's response; it does not
claim to reproduce CS2 spray paths or timing exactly.

- Adopt each magnitude unit as one degree/second of angular velocity impulse.
  Select magnitude uniformly in `[Magnitude-Variance, Magnitude+Variance]`
  and angle uniformly within the source angle variance around upward pitch.
  Pitch/yaw impulse is `magnitude * (cos(angle), sin(angle))`.
- Fixed-pattern weapons use repeatable per-shot sampling from a stable project
  seed (CRC32 of the source row name). Random weapons use an independent server
  random stream. These are project patterns, not recovered CS2 seeds. Reset the
  pattern index after a .55s firing gap; retain any physical recoil residual.
- Between shots solve `dAngle/dt = Velocity - 8*Angle`,
  `dVelocity/dt = -4.5*Velocity` analytically. At elapsed `t`,
  `A = A0*exp(-8t) + V0*(exp(-4.5t)-exp(-8t))/3.5`,
  `V = V0*exp(-4.5t)`. These global project constants are not extracted from
  Excel's **inaccuracy** recovery times. The response is independent of frame
  rate, rises after each impulse and returns smoothly without permanent drift.

## Authority and camera

Only accepted server shots add recoil, once per shell (including shotgun),
after the shell's directions were sampled. The following bullet's aim uses
the remaining angular deflection plus player input, before independent spread
and inaccuracy. Empty, cooldown and reload rejection never create an impulse.
AWP selects its recoil profile before automatic unscope.

One character-level snapshot contains angle, velocity and server timestamp,
replicated only to its owner. The owner evaluates the same response on the
synchronized server clock. `GetViewRotation` adds it once for authoritative
eyes and the camera component, so the attached FPS arms/iron sights follow the
same frame. The combined view keeps pitch inside ±89° to prevent a vertical camera flip.
ControlRotation remains player input: moving the mouse compensates
the deflection naturally. Weapon switching cannot erase the residual. A new
respawn starts fresh; dead-player cameras ignore it. Damage flinch retains its
separate cosmetic path. Existing fire animations are preserved.

The old shared .35° cosmetic kick/.65° cap is disabled for worksheet guns;
legacy fixtures and utility without worksheet rows keep their existing feedback.

## Migration and checks

`setup_weapon_recoil.py` runs through Unreal, saving only the six existing owned
firearm data assets and their recoil fields. `import_sketchfab_weapons.py` also
assigns these fields for future imports; routine numeric tuning never reruns
the art import. Backups before migration: `/tmp/rr-weapon-recoil-backup`.

`EclipseRecoil.Weapons.WorksheetRecoil` checks all saved raw values and optical
profiles against the extraction, bounds both variances over thousands of samples,
distinguishes repeatable and random patterns, and verifies recovery at30/60/144Hz.
`test-network.py --recoil` exercises real authorized shots in all six guns and
both optical levels, accumulation, first/next bullet timing, empty rejection,
holstering, mouse compensation, remote burst and owner camera/clock recovery.
See `TEST_RESULTS.md` for executed results.
