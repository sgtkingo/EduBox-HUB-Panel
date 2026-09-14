# Release Workflow Rules

EduBox HUB Panel uses a commit-message driven release flow on `main`. The automated workflow bumps the version, synchronizes generated metadata, builds firmware, refreshes `bin/latest`, creates a version tag, and publishes a GitHub Release.

Workflow files:

- [`release-auto.yml`](../../.github/workflows/release-auto.yml)
- [`build.yml`](../../.github/workflows/build.yml)

## Release Gate

The release workflow runs on every push to `main`, but it creates a release only when the latest commit message starts with one release marker:

```text
(major)
(minor)
(patch)
(build)
```

Expected format:

```text
(marker) Short release description
```

Valid examples:

```text
(build) Hotfix SD path
(patch) Fix CSV export header
(minor) Add DataBundle session metadata
(major) Protocol v2 migration
```

Invalid examples:

```text
Hotfix (build)
(build)(patch) Fix release
build: Hotfix
```

If no marker is found, the workflow exits without releasing. Commits beginning with `chore(release):` are also ignored by the gate so the workflow does not recurse on its own version-bump commits.

## Version Format

The source version is stored in [`VERSION`](../../VERSION):

```text
MAJOR.MINOR.PATCH.BUILD
```

Bump rules:

| Marker | Result |
| --- | --- |
| `(major)` | `(MAJOR + 1).0.0.0` |
| `(minor)` | `MAJOR.(MINOR + 1).0.0` |
| `(patch)` | `MAJOR.MINOR.(PATCH + 1).0` |
| `(build)` | `MAJOR.MINOR.PATCH.(BUILD + 1)` |

The release tag is `vX.Y.Z.B`.

## Automated Release Sequence

When a release marker is accepted, [`release-auto.yml`](../../.github/workflows/release-auto.yml) performs this sequence:

1. Checkout `main` with full history.
2. Read the latest commit message and decide the bump type.
3. Read [`VERSION`](../../VERSION), validate `x.y.z.b`, and compute the next version.
4. Write the next version to [`VERSION`](../../VERSION).
5. Run [`storage/sync_version.py`](../../storage/sync_version.py) to mirror the version into [`config.hpp`](../../libraries/engine/src/config.hpp).
6. Run [`storage/sync_config.py`](../../storage/sync_config.py) to sync app config mirrors.
7. Run [`storage/sync_db.py`](../../storage/sync_db.py) to sync DB mirrors and image assets.
8. Commit and push the metadata bump as:

```text
chore(release): bump version to X.Y.Z.B
```

9. Call reusable [`build.yml`](../../.github/workflows/build.yml) against the bumped commit.
10. Download the build artifacts.
11. Replace `bin/latest/*` with the freshly built `.bin` files.
12. Commit and push refreshed binaries as:

```text
chore(release): refresh bin/latest for X.Y.Z.B
```

13. Create and push tag `vX.Y.Z.B`.
14. Create the GitHub Release using [`RELEASE_NOTES.md`](../../RELEASE_NOTES.md) as the release body and `bin/latest/*` as assets.

This order ensures the published firmware is built from the bumped version, not from the previous commit.

## Build Workflow

[`build.yml`](../../.github/workflows/build.yml) is reusable through `workflow_call` and can also be started manually with `workflow_dispatch`.

Default build inputs:

| Input | Default |
| --- | --- |
| `artifact_name` | `firmware-esp32s3` |
| `esp32_core_version` | `3.1.1` |
| `build_path` | `ui/build/esp32.esp32.esp32s3` |
| `run_version_sync` | `true` |
| `run_sync` | `true` |
| `run_db_sync` | `true` |

Default board/FQBN:

```text
esp32:esp32:esp32s3:UploadSpeed=921600,USBMode=hwcdc,CDCOnBoot=default,MSCOnBoot=default,DFUOnBoot=default,UploadMode=default,CPUFreq=240,FlashMode=qio,FlashSize=4M,PartitionScheme=huge_app,DebugLevel=none,PSRAM=opi,LoopCore=1,EventsCore=1,EraseFlash=all,JTAGAdapter=default,ZigbeeMode=default
```

The build workflow:

1. Checks out the requested ref.
2. Sets up Python.
3. Installs Arduino CLI.
4. Installs the selected ESP32 Arduino core.
5. Optionally runs version/config/DB sync scripts.
6. Compiles `ui`.
7. Creates `ui.ino.merged.bin` with `esptool` if Arduino CLI did not create it.
8. Validates required `.bin` outputs.
9. Uploads all firmware `.bin` files as a workflow artifact.

Required outputs:

```text
ui.ino.bin
ui.ino.bootloader.bin
ui.ino.partitions.bin
ui.ino.merged.bin
```

## Release Notes

[`RELEASE_NOTES.md`](../../RELEASE_NOTES.md) is maintained manually. Update it before pushing a release marker commit.

Recommended top section:

```markdown
## vX.Y.Z.B - YYYY-MM-DD

- Added ...
- Fixed ...
- Changed ...
```

The release workflow uses the whole file as the GitHub Release body, so keep the newest release section at the top.

## Recommended Release Procedure

1. Make sure `main` is green and up to date.
2. Update [`RELEASE_NOTES.md`](../../RELEASE_NOTES.md).
3. Commit normal source/docs changes without a release marker if needed.
4. Push one final release trigger commit to `main` with exactly one marker at the beginning:

```bash
git commit -m "(patch) Fix CSV export header"
git push origin main
```

5. Watch `Release Auto` until it finishes.
6. Confirm that:

- `VERSION` was bumped.
- [`config.hpp`](../../libraries/engine/src/config.hpp) contains the same firmware version.
- `bin/latest/*` contains the new firmware files.
- tag `vX.Y.Z.B` exists.
- the GitHub Release contains the expected assets.

## Manual Build Without Release

Use the `Build Firmware` workflow manually when you want firmware artifacts without tagging a release. This can be useful for testing a branch or validating a specific ref.

Manual builds do not bump `VERSION`, do not refresh `bin/latest`, and do not create a GitHub Release.

## Failure Handling

Common failures:

| Failure | What to check |
| --- | --- |
| Gate does not release | Latest commit on `main` does not start with a marker. |
| Invalid version | [`VERSION`](../../VERSION) is not `x.y.z.b`. |
| Metadata bump fails | Sync scripts changed no tracked files or a generated file path changed. |
| Build fails before compile | Arduino ESP32 core install or FQBN issue. |
| Build fails at output validation | One of the required `.bin` files is missing. |
| Release publish fails | Tag already exists, missing assets, or insufficient `contents: write` permission. |

If a release failed after bumping `VERSION` but before publishing a release, inspect the workflow logs before retrying. Avoid creating a second marker commit until you know whether the tag or `bin/latest` commit already exists.

## Editing Rules

- Keep release marker parsing simple and explicit.
- Keep release-generated commits prefixed with `chore(release):` so they are ignored by the release gate.
- Keep [`build.yml`](../../.github/workflows/build.yml) reusable; release automation depends on `workflow_call`.
- Keep build artifacts limited to `.bin` files required by firmware deployment/update tools.
- Update this document whenever release marker rules, artifact names, version semantics, or workflow ordering changes.
