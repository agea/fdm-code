# Eclipse Recoil Unreal development

Use `docs/ECLIPSE_RECOIL_UNREAL_BASE.md` as the gameplay specification and
`docs/DEVELOPMENT.md` for the user's current scope and implementation status.
The user's current request takes precedence over the attached specification:
work on a locally playable Unreal alternative; defer packaging, distribution,
CI, external publication and standalone platform builds.

14 October publication scope: the user explicitly authorized public agea/fdm-code
and three private asset submodules (characters, weapons, maps), preserving the
original private agea/fdm. See docs/ASSET_SUBMODULES.md. This supersedes the
earlier publication deferral for these four repositories only. Keep licensed
binary assets out of every public commit; do not choose a code license implicitly.
Content/SourceAssets are generated mounts into Assets/*; edit/save via Unreal
at unchanged /Game paths, commit private assets first and then their gitlinks.
Do not run old cleanup/migration commands against linked vendor folders without
reviewing their new repository ownership.

- Keep this existing UE 5.8.3 project (`fdm.uproject`, native module `fdm`).
- Do not modify the original game under `/Users/agea/git/csgopen/eclipse-recoil`.
- Combat, ammo, teams, scoring, protection and respawn are server authoritative.
- Point damage supplies server-authorized shot direction to owner hit feedback.
  Keep camera flinch cosmetic; never move the capsule or change ControlRotation.
- Death spawns a separate replicated cosmetic team-character corpse. Ragdoll contacts
  only WorldStatic, never players/projectiles; retain it through respawn, settle
  after five seconds, cap twelve/90 seconds and clear on round reset.
- Rifle spread is real and server sampled; the ammo reticle reads that same
  cone plus replicated owner burst state against the synchronized server clock.
  Keep posture, speed, airborne and burst penalties shared with the HUD.
  No client-selected shot direction or cosmetic-only dispersion indicator.
- Enhanced Input lives on the controller so dead players can request respawn.
- Crouch accepts Ctrl and C. `scripts/fix_controls.py` adds C to an existing
  mapping without replacing the other bindings. `-ERInputAudit` is an opt-in
  graphical-test log; distinguish observed inputs from headless probe results.
- Opt-in local test bots were requested by the user. Keep normal LAN hosting
  free of bots; bots share player combat, team, scoring and respawn rules.
- Movement scope (13 October): 185cm standing capsule, 10km/h default run,
  Shift walks at 5km/h, acceleration/braking inertia and a lower, faster jump.
  Step automatically through 30cm; timed weapon-held vault through 80cm;
  Space+forward climbs through 150cm relative to current feet, including in air.
- Sprint/posture scope (14 October): double W within 300ms, held second press,
  runs at 18km/h for at most five seconds, followed by ten seconds of recovery
  including early interruption. Require a fresh gesture after recovery. Hold
  C/Ctrl crouches; double C/Ctrl latches prone, next press/release rises.
  Keep physical posture/eye transitions timed (0.4s crouch, about 1.1s prone),
  validate final standing clearance and keep the owner camera/server eye aligned.
  Prone world pose is procedural on team rigs; its extra query hit volume must
  stop accepting hits at death. Movement still uses the standard vertical capsule.
  Verify with test-movement.py (including InputKey binding tests) and accuracy;
  ERPostureGallery is opt-in art capture, never enabled in ordinary play.
- Presentation: hide the FPS left arm with the rifle during high climbing.
  Render FPS arms, rifle and muzzle flash as FirstPerson primitives, using
  camera depth scale 0.2 and 1cm near plane. Keep physical socket transforms,
  world meshes and authoritative shots unchanged; preserve viewmodel framing.
  Keep walking bob cosmetic in CalcCamera, faded when idle/airborne/traversing;
  preserve the capsule and server eye position. IA_Look scalar is 0.18.
- Use UERMovement and Unreal saved/compressed moves for traversal and walking
  intent. Do not implement movement through unrestricted client position RPCs.
- L_MovementLab contains twenty 200x200cm blocks, 10..200cm in 10cm increments.
  Keep this test map separate from the depot and preserve edited geometry.
- User-approved Fab low-poly arsenal: seven selectable primaries, a fixed
  M1911 sidearm and HE/smoke/stun utility slots. Server validates slot changes
  and loadout indices; primary changes apply at respawn, never refill live ammo.
  Keep ammo, cooldown and bloom per weapon; cancel firing/reload when holstering.
  Mouse-wheel bursts select one immediate slot and require 250ms without wheel
  events before another gesture; numeric slot keys remain immediate.
  No pickups, corpse loot, melee or matchmaking.
- Store Fab source archives/FBX/palette in SourceAssets/LowPolyWeapons and
  imported assets in Content/EclipseRecoil/Weapons/LowPoly. Never load Downloads
  at runtime. Preserve supplied meshes/skeletons; grip transforms and animations
  belong in project-owned weapon data/poses. Use actual model names.
- Grenade/rocket damage is server-only with WorldStatic cover checks. Smoke
  is a cosmetic volume; stun is owner feedback. Do not claim smoke affects bot
  vision, bespoke weapon audio or authored reload animations without implementing.
- HE detonations multicast a local BP_ERGrenadeExplosion with baked flying
  fragments from SourceAssets/BombCrackGround, excluding the source floor.
  Keep this cosmetic actor separate from projectile cleanup, collision-free,
  non-replicated, capped at eight bursts and removed after animation playback.
  Art defaults live in Effects/Grenade; preserve the supplied FBX/archive.
- Smoke grenades spawn a server-owned replicated ERSmokeCloud, with a local
  Niagara Fluids simulation per peer. Keep position/lifetime authoritative,
  collision disabled, a 40-cell maximum axis, four clouds/12 seconds and round
  cleanup. Each simulation owns its volume render target. Bullet carve positions
  derive only from authorized shot feedback; never affect damage or server aim.
  The low-shadow profile disables volumetric fog, so enable its rendering pass
  only while clouds exist and use transient fog support without editing maps.
  Imported source packages stay in InteractiveSmoke; owned defaults in
  Effects/Smoke. Smoke still does not interrupt bot sight checks.
- Team world characters (13 October): Alpha uses owned Twinblast ActionHero;
  Omega uses owned Drongo GDC. Keep meshes/materials/retargeted locomotion in
  Characters/Teams and art references in FERTeamAppearance Blueprint defaults.
  Apply the replicated PlayerState team to humans, bots and matching corpses.
  Hide Paragon weapons and Drongo mask hoses/bone decorations at every LOD.
  Use matte olive/brown clothing with no hero emissive effects; preserve skin.
  FPS renders only the chosen skin's extracted forearms/hands on the fitted
  fitted FPS bind rig, keeping its existing component frame, weapon poses and grips.
  Do not claim the original cybernetic geometry was replaced with human limbs.
  Use owned PA_ER_{Alpha,Omega}_Ragdoll (16 bodies/15 joints); preserve saved
  DefaultProfile locks and limits. Corpse meshes retain LOD0 and evaluate the
  frozen animation until settling. Collapse hidden bones after physics blending
  in ERCorpseMeshComponent::FinalizeBoneTransform, before publishing the pose.
- Edit or move `.uasset` and `.umap` through Unreal, never by binary patching.
- Selectable skins (14 October): five appearances per team in AlphaSkins and
  OmegaSkins BP arrays. Cold blue/navy Alpha and warm rust/orange Omega are
  dominant, with neutral trousers/straps/boots/gear and natural skin/hair.
  Use distinct source clothing plus owned helmet/pack geometry; preserve the
  Paragon geometry limitation. Selection persists locally and is server validated;
  ActiveSkin changes at respawn, and each corpse retains its initial skin index.
  Gear follows head/spine bones, has no collision and shares body visibility.
  Use setup_team_skins.py after the base rig setup and verify_team_skins.py;
  render all ten silhouettes and corpse poses before claiming visual validation.
- Keep art references in Blueprint defaults. Reuse template assets first.
- In Unreal Python, always construct rotations with named `pitch`, `yaw`, `roll`
  arguments. Positional `Rotator` arguments use the Blueprint factory order.
- The template rifle's root GripPoint attaches to Manny's `HandGrip_R` socket;
  do not independently rotate the FPS rifle under the camera.
- FPS uses the project-owned `A_ER_Rifle_HipPose`, generated by
  `scripts/create_rifle_pose.py`. Preserve template animations and keep the
  left grip aligned with the rifle foregrip when replacing this static pose.
- FPS keeps the fitted independent rig after the user supplied the mannequin reference.
  Match the weapon framing, keep the left hand below the foregrip, exclude
  upper-arm material sections and show slightly less forearm than the mannequin.
  `restore_mannequin_first_person.py` remains available for comparison.
- FPS uses `Characters/FirstPerson/SKEL_ER_FirstPerson`, two base rifle poses
  and twenty-two weapon poses in FirstPerson/Weapons. The one-time
  detach_mercenary.py migration preserves fitted bone data and poses, removes
  vendor compatibility/preview links, and replaces every body/material fallback.
  Skin FPS meshes in Teams/{Alpha,Omega}/FirstPerson keep only lowerarm/hand
  descendant geometry and preserve skin UVs/material slots. The Alpha Officer
  forearms are the geometry/reference provider; no mercenary mesh or texture is
  needed. Use setup_skin_first_person_forearms.py through
  Unreal; keep (-11.2263,1.4908,-159.9167), yaw -90 and component scale 1.
  FirstPersonMesh selects the skin geometry while all existing weapon idle/fire
  poses and socket grips remain unchanged. Do not use the world animation
  blueprint or move/scale the complete hero body to frame FPS arms. Verify both
  grips and capture all ten variants with -ERFPSForearmsGallery.
- Firing uses the project-owned `A_ER_Rifle_Fire` (100ms), generated from the
  hip pose with retargeting disabled during copying. Preserve both grips and
  exact idle endpoints. `MulticastShot` triggers cosmetic recoil, muzzle flash
  and template audio only for server-authorized shots. Camera recoil must not
  change ControlRotation or server aim. Art references stay in BP defaults.
- `scripts/setup_shot_feedback.py` rebuilds only its generated firing assets;
  verify material expression connections and real Metal compilation. The
  opt-in `-ERShotAudit` and `test-network.py --shot-feedback` audit local and
  remote shot events; nullrhi/nosound do not verify visual or audio quality.
- Rifle impacts create a replicated procedural crater decal on world surfaces;
  never draw hit marks on players. Keep the decal material generated by
  `scripts/setup_bullet_impact.py` and avoid importing textures from the
  original game without verified licensing.
- `scripts/setup_editor.py` creates assets only when absent; preserve edited maps.
- Record real compiler, Editor and network test outcomes in `docs/TEST_RESULTS.md`.
- Update development decisions and the asset inventory whenever scope changes.
- Do not claim tests, Fab compatibility or license verification without evidence.
- README and AGENTS files are English; development notes may be Italian.

Compile the Editor module as needed for local execution; do not add packaging or
release automation unless the user requests it. Run meaningful network tests for
gameplay changes. Never publish the repository or acquired assets implicitly.

Stationary Target training opens ten targets (five skins per team) from
the menu. These exhibition targets do not consume the four human LAN slots;
keep patrol bots limited by the combined four-slot cap. Preserve each
target's team, skin and assigned clear start across respawns. Verify all ten
replicated meshes with scripts/test-target-skins.py and bot regressions.

Repository maintenance: clean-project.py defaults to a dry run and protects
Content/SourceAssets, Saved settings/autosaves and Git/LFS. Never equate an
Asset Registry candidate with proof that it is disposable. Keep the original
mesh inputs required by skin-generation scripts and the independent FPS rig/poses.
Use Unreal for asset deletion/migration; preserve package paths. See
docs/REPOSITORY_CLEANUP.md and docs/ASSET_LICENSES.md before separating assets
or preparing a public repository. Do not commit/publish the current index
indiscriminately; Fab raw assets and derivatives remain outside code exports.

Mercenary removal (14 October): the vendor Mercenary_Male_WaveforceStudio tree
and old EclipseRecoil/Characters/Mercenary tree are obsolete. Keep fitted rig
and poses in FirstPerson; preserve their provenance rather than assuming that
removing a vendor dependency changes licensing. Historical mercenary setup
scripts live in scripts/legacy and are not rebuild roots. Verify with
verify_no_mercenary.py and scripts/test-without-mercenary.py; use only read-only
verifiers or game execution in its hardlinked fixture, never setup/import/save.

Cleanup executed (14 October): 272 obsolete Mercenary and 3399 unreachable
Paragon packages removed through Unreal after independent verified backups.
Preserve the 892-package current authoring/runtime closure and edited maps.
DeleteUnloadedParagonPackages is an Editor-only, namespace-scoped package
removal helper: require an archive receipt, no loaded packages and no outside
referencers. Never treat runtime reachability alone as deletion approval.
Local LFS storage is /Users/agea/git/fdm/_cache/lfs; no objects pruned.
Backups/verification fixture moved to the sibling _backups directory.
