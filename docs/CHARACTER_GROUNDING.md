# Character ground contact

World skin placement previously used the reference foot bone minus a fixed 6cm
sole allowance. Actual animated boot vertices were 5.39–9.65cm below the capsule
base in the approved idle poses. This affected all five character families.

`setup_character_grounding.py` measures the lowest foot/ball vertex in 17 samples
across each skin's full idle clip and sets `FeetOffset = -92.5 - soleHeight`.
The engine's normal capsule floor clearance remains in effect. Scale, art,
animations, first-person presentation, camera and collision are unchanged.
Both team catalogs and fallback appearances are updated in BP_ERCharacter,
BP_ERTrainingTarget and BP_ERCorpse. Existing posture and network presentation
already consume this shared FeetOffset.

Run through Unreal's PythonScript commandlet after an explicit skin/catalog
rebuild. Each run backs up the three catalog packages and records before/after
values under `Saved/Testing/CharacterGrounding`. The operation is idempotent.
This is an idle support calibration, not terrain foot IK: uneven ground and
animated swinging feet can still separate from the floor.

Validation on 30 October 2026: saved-catalog reload measured all 20 skins at
17 idle samples each; maximum residual from capsule base was 0.000004cm.
All 1,289 character-package SHA256 hashes remain identical. Only character and
corpse catalog packages changed (training target inherits the character values).
The runtime-state manifest records those two new catalog hashes; the old/new
hash receipt is saved alongside the catalog rollback ZIP.
Metal skin-gallery rendering also passed for both pages (all 20 appearances),
with successful process exits and four saved Alpha/Omega captures.
