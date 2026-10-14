# Public code and private assets

The current project lives in **agea/fdm-code** (public). The original
**agea/fdm** repository remains private with its original history. The code
repository starts with a fresh history containing text and three gitlinks;
it never imports the old Content history or LFS objects.

| Private repository | Checkout | Contents |
| --- | --- | --- |
| agea/fdm-assets-characters | Assets/characters | Twinblast, Drongo, mannequin reference, owned team skins, FPS rigs/poses and character animation |
| agea/fdm-assets-weapons | Assets/weapons | Arsenal, weapon data/Blueprints, shot audio, smoke/explosion effects and original import sources |
| agea/fdm-assets-maps | Assets/maps | Edited maps, external actors/objects, prototype geometry, common Core Blueprint defaults, input and template UI/Blueprints |

These bundles are used together: Core Blueprint defaults reference both
characters and weapons. The category split provides independent versioning,
not standalone packs. Template FirstPerson/Variant_Shooter content, including
their small animation folders, stays with maps to preserve each template tree.

## Clone and launch

Anyone can clone/read the code without private repository access:

```sh
git clone https://github.com/agea/fdm-code.git
cd fdm-code
```

To run or rebuild content, Git LFS and access to all three private repositories
are required. If your existing access uses SSH keys, clone with
`git clone git@github.com:agea/fdm-code.git` instead, or set an existing
clone origin with `git remote set-url origin git@github.com:agea/fdm-code.git`.
Relative URLs in .gitmodules inherit SSH or HTTPS from the parent
clone; use your normal GitHub authentication. Do not put tokens in URLs/files.

```sh
git lfs install
python3 scripts/setup-assets.py --init
python3 scripts/setup-assets.py --check
```

The script initializes the exact submodule commits recorded by the code
repository, downloads LFS content and creates 22 directory links below ignored
Content and SourceAssets. macOS/Linux use relative symlinks; Windows uses
directory junctions. No Unreal asset is renamed and every /Game package path
is preserved. Existing content or unexpected links cause an error, as do
unresolved LFS pointer files; the script never overwrites assets. Close Unreal
before updating submodules or mounts. The Windows junction path has not been
validated on a Windows machine.

Compile the Editor module using the README command, then launch normally.
Missing private credentials cause asset initialization to fail; a public code
clone alone is not a runnable game. Submodule repository names and commit IDs
are public metadata, while their files and LFS objects remain private.

## Asset edits

Edit assets through Unreal at their original Content paths. The directory links
write into the corresponding submodule, so check and commit its changes first:

```sh
git -C Assets/characters status
git -C Assets/characters add Content
git -C Assets/characters commit -m "Update Alpha appearances"
git -C Assets/characters push origin main
git add Assets/characters
git commit -m "Use updated character assets"
git push origin main
```

Push every changed asset repository before publishing the code commit that
references it. Update collaborators with `git pull` followed by
`python3 scripts/setup-assets.py --init`; avoid `submodule update --remote`
unless deliberately upgrading a dependency. Close Unreal first and commit
local asset edits before switching pinned versions.

Each private repository tracks packages, meshes, textures and source archives
with its own Git LFS storage. Generated caches/logs/builds stay outside history.
LFS quotas are shared at account level; splitting repositories does not reduce
total storage or download traffic. See
[GitHub LFS billing](https://docs.github.com/en/billing/concepts/product-billing/git-lfs).

## Publication boundaries

The public tree uses an explicit text allowlist and
`python3 scripts/verify-public-tree.py` checks all local Git history for assets,
binary blobs, LFS pointers and unexpected paths. Content and SourceAssets are
ignored, including their mounted directory links. Only the three Assets
gitlinks are committed in the parent repository.

Private access is limited to authorized project collaborators; it does not
grant redistribution rights. Keep Fab originals and generated derivatives
private, including the independent fitted FPS rig. Preserve provenance in
each asset repository and [ASSET_LICENSES.md](ASSET_LICENSES.md). No open-source
license has been selected for the project code. Public visibility permits
viewing and GitHub forking under GitHub's terms; broader use remains governed
by the respective author/third-party terms. Epic notices in template sources
remain unchanged. Publishing the code does not publish a packaged game.

## Recorded validation

All four repositories were created and uploaded on 14 October 2026.
A fresh anonymous code clone and authenticated download of all three asset
repositories passed: 1,336 verified file hashes, LFS fsck and 22 mounts.
The split checkout passed Mac Editor compilation, network skin/shot tests
and the ten-skin FPS Metal gallery. See TEST_RESULTS.md and REPOSITORY_SPLIT.json.
