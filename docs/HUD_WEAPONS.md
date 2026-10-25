# Weapon inventory HUD

Original owner-supplied ZIP, PNGs and claymore image live unchanged in the private
maps SourceAssets/HUDWeapons directory. setup_hud_weapons.py imports eleven UI
textures with SimpleAverage mipmaps, trilinear filtering and clamped edges.

ERWeaponHUD draws only replicated Inventory entries, in inventory order from
bottom to top. The right edge uses the shared proportional viewport inset.
Transparent image margins are cropped through UVs; firearm scale is calibrated
against grip size rather than total weapon length. Each entry shows its actual
catalog input (1–8, F, G, H); legacy inventories use their numeric slot bindings.
Icons are 10% smaller than their initial grip-calibrated size. The selected icon
uses the shared Alpha blue / Omega orange team tint in place of the vertical mark. Icons and shortcut labels use 90% opacity normally and 33% for spent firearms
and utility. Shortcut text uses scale 0.5 (slightly above the previous 0.425), with visible
capitals/digits centered on the icon rather than the full font line box; melee stays at 90%. The count below the stack belongs to the
selected item: loaded / (loaded + reserve), utility total only, none for melee.
Reloading changes the count to amber without obscuring the ammunition numbers.
The old weapon-name/reloading footer and overlapping claymore sentence are gone.
Placement and spawn protection messages remain separate gameplay feedback.

Validation uses build-editor.command, test-hud.py --render --weapons (selection, utility totals, exhausted firearms) and the existing
15-kit test-loadouts.py --render fixture for real catalog/loadout replication.
