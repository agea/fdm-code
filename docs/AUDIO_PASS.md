# Footsteps, firearm variations and smoke release

21 October 2026. UE 5.8.3; native module and serialized weapon IDs unchanged.
This pass implements the owner's `RESPAWN_AND_RAGE_AUDIO_PASSI_SPARI_FUMOGENA.md`.
Executed checks and remaining listening limits are recorded in TEST_RESULTS.md.

## Recordings and ownership

Original archives, creator-page snapshots, included licenses, source hashes,
exact source-frame cuts and derived hashes live privately in
`Assets/weapons/SourceAssets/AudioPass`. Runtime packages live at
`/Game/EclipseRecoil/Audio/`, mounted from the private weapons repository.
Public core contains code and credits only. `.7z`, `.ogg`, `.wav` and `.uasset`
are LFS-managed in that private repository. Preserve originals on pipeline reruns.

| Weapon label | Stable data key | Actual recordings | Source identity |
|---|---|---:|---|
| Pistol | Pistol | 4 | 1911, two recording positions |
| SMG | SMG | 6 | Carl Gustav M45, two recording positions |
| Shotgun | Shotgun | 4 | Benelli Nova, two recording positions |
| Assault Rifle | Assault | 6 | AK-47, two recording positions |
| Sniper Rifle | Carbine | 4 | Tikka, two recording positions |
| Marksman Rifle | Sniper | 4 | SKS |
| Grenade Launcher | Launcher | 1 existing | Existing Epic launcher sample |

These are timbral stand-ins, not claims to recordings of every displayed model.
The six firearm sets use separate real semi-auto transients from
[The Free Firearm Sound Library](https://opengameart.org/content/the-free-firearm-sound-library),
by Ben Jaszczak, Brian Nelson, Kevin Heras and Matthew Nanney, declared CC0.
No rapid bursts were chopped into artificial independent variations. PCM copies
are mono 48kHz/16bit, with 6ms attack lead and a short end fade. Attack RMS is
matched by attenuating louder takes; within-set spread is about 1.05dB or less
before cue modulation. This avoids hard limiting the transient. Source clipping
fractions and all conversion gains are retained in the private manifest.

The approved libraries contain no grenade-launcher recording. Its single Epic
sample, previous pitch and .35/.6 local/remote playback gains are retained; the
cue supplies only subtle modulation. No fictitious take count is claimed.
Historical LowPoly fixtures keep their existing fallback audio.

Footsteps use eight real variants each for Hard, Wood, Metal, Grass and Gravel
from [congusbongus's pack](https://opengameart.org/content/footsteps-on-different-surfaces).
The included credits identify swuing (concrete/wood/grass, CC BY3.0), Eelke
(aluminum ladder, CC BY3.0) and Ali_6868 (gravel, CC0). All credits/links and edits
are retained privately and in `Branding/Credits.txt`, available in the game menu.
LightHard uses one stone sample from
[TinyWorlds](https://opengameart.org/content/different-steps-on-wood-stone-leaves-gravel-and-mud),
declared CC0. Dirt explicitly falls back to Grass and Snow to Gravel; neither
has a dedicated recorded set. The Thimras metal/robot-foot pack is source-only,
not the universal human boot sound.

Tabasco is not imported: its creator page says CC0, while its included
`creativecommons.txt` says Copyright 2009 Vincent Sevedge, CC BY3.0. Both pieces
of evidence are preserved in its provenance instead of assuming CC0.

## Runtime selection

`UERWeaponData.ShotAudio` references a `UEROneShotAudio` palette (cue + real
SoundWaves). The existing server-authorized `MulticastShot` event also carries
the accepted weapon data reference. This avoids selecting a provisional or newly
holstered weapon when its actor's properties arrive after the cosmetic event.
There is no extra shot event, client shot authority or replicated variant index.
Shotgun still emits one presentation event per cartridge, outside its six rays.

Each emitter maintains a shuffle bag per palette. It draws every recording once
and excludes the preceding bag's final recording at the next boundary. It uses
its own `FRandomStream`, never the game RNG that seeds weapons/ordnance.
The saved cue contains Modulator -> Switch (default Random) -> WavePlayers.
Runtime sets the local switch index before Play, producing exactly one source; Random without
replacement remains the preview/fallback. The cue has finite duration and
virtualization is disabled, so an inaudible one-shot cannot resume as a loop.
All graph editing/saving uses Unreal.

Firearm modulation: pitch .98..1.02, level .891..1.122 (about +/-1dB).
Owner playback stays centered 2D; other players use the existing muzzle and
300cm inner radius + 3700cm falloff. Concurrency is 24 per owner / 96 global,
stopping oldest tails first with 25ms release. A 128-source default quality
budget leaves room for short steps and hit/voice feedback. No sound class gain
or gameplay firing interval was changed.

## Footsteps

`UERFootsteps`, inherited by humans and bots, observes actual horizontal travel
on authority after movement. It emits an unreliable transient multicast per
appoggio; clients only play those events, so owner audio cannot double with
prediction. Clients send no footstep requests. Late join does not replay steps.
The existing camera bob is independent and unchanged.

Distance per event at full posture:

| Speed | Stride | Approximate events/s |
|---|---:|---:|
| 5km/h | 95cm | 1.46 |
| 10km/h | 130cm | 2.14 |
| 18km/h | 150cm | 3.33 |

Low posture uses 80cm, prone 65cm. Relative gains: walk .7, run 1, sprint 1.15,
crouch .4, prone .3; base .18 and owner multiplier .65 remain data-asset editable.
The safety interval is 220ms; hitches cannot emit a backlog. Stationary input,
airborne/dead/traversing/disabled movement and large teleports reset distance.
Returning to ground starts a fresh stride; no landing sound was introduced.

A short WorldStatic trace requests the physical material. `DA_ER_Footsteps`
contains SurfaceType -> family, cell-material -> family, material-asset -> family,
and family -> palette maps. Cell hits use the existing `ResolveCell` ID without
changing collision, catalog properties or HP. Concrete/stone/brick are Hard,
wood is Wood, steel/container is Metal, drywall/glass are LightHard. Ordinary
meshes use physical surface IDs; current authored ground/floor materials also
have identity mappings. Unknown surfaces fall back to Hard. This follows the
[UE data-asset footstep approach](https://dev.epicgames.com/documentation/unreal-engine/artist-11-add-footstep-sounds-to-a-character?lang=en-US).

Footsteps use pitch .94..1.06, slight level modulation, 125cm full radius plus
2075cm falloff (22m total), 4 voices per owner / 48 global and priority .25.
Add a palette and mapping to the data asset when introducing another surface.

## Smoke gain and regeneration

`AERSmokeCloud.ReleaseVolume` is a persistent Blueprint-editable local gain.
The previous playback constant was .65, so the new default is
`.65 * .31622777`, approximately `.205548` (-10dB). Future Blueprint tuning is
preserved by `setup_grenade_audio.py`; rerunning never applies another -10dB.
The Sonniss source/runtime WAVs, HE sample/class/gain, 6s duration, baked
4.5..6s fade, 12s cloud, server age offset, cleanup and dedicated-server guard
are unchanged. Release remains a single BeginPlay playback, skipped past its
sample duration; RepNotify does not restart it.

Prepare derivatives with `scripts/prepare_audio_pass.py` (numpy, macOS afconvert,
sndfile-convert), then run `scripts/setup_audio_pass.py` through Unreal Editor.
This imports only owned audio and changes seven current weapon audio references,
the inherited character component default and the smoke Blueprint save.
It does not rerun weapon art, recoil/accuracy migrations or map generation.
`setup_shot_feedback.py` preserves the fallback and the per-weapon palettes.

Validation entry points:

```
python3 scripts/test-audio.py
python3 scripts/test-audio.py --network --render
python3 scripts/test-network.py --shot-feedback
python3 scripts/test-movement.py
python3 scripts/test-grenades.py --render --audio
```

`-ERAudioAudit` records selected recordings, playback starts, perspectives and
footsteps. The opt-in audio probe waits for possession and per-weapon model acknowledgments, exercises
20+ accepted shots per weapon on both characters, sustained SMG/Assault, empty
and denied shots, six-pellet shotgun, both bots and actual owner ground travel on a clear transient floor. The development-only
readiness RPC is inert in normal games and carries no audio-variant state.
Graphical runs enable the real audio mixer; headless runs are structural tests,
not listening tests. Subjective headphone balance and A/B remain separate from
numerical gain and playback evidence. No new packaged build is claimed here.
