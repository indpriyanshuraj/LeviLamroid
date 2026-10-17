# Packaging and distribution: tooth.json vs the .levipack

There are two different package formats in play in this repo, and only one of
them is the right one for LeviLamroid. This note explains the difference, why the
current `tooth.json` is broken, and what a release should ship.

---

## 1. `tooth.json` is lip's manifest (the LeviLamina ecosystem)

`tooth.json` is the package manifest for **lip**, LiteLDev's package manager for
LeviLamina. Its documented examples and tutorials are all Windows/LeviLamina:
`platform: "win-x64"`, dependencies on `github.com/LiteLDev/LeviLamina`, assets
placed into `plugins/` or `mods/`, tags like `platform:levilamina`, `type:mod`.

The *format* itself is platform-generic - the `platform` field is a .NET runtime
identifier (RID) that supports globs and an empty "match all" value, and lip runs
on Windows, Linux and macOS. So an Android variant is not forbidden by the schema.
But every documented workflow, the LeviLamina dependency model, and the examples
are built around the Windows LeviLamina client/server, not Android.

**Bottom line:** tooth.json is a LeviLamina-ecosystem manifest. It is not the
native delivery format for an Android native mod.

## 2. The Android format is `manifest.json` + the `.levipack`

LeviLaunchroid's native modules use a different manifest, `manifest.json`:

```json
{
  "type": "preload-native",
  "name": "...",
  "author": "...",
  "version": "1.0.0",
  "entry": "libsome_mod.so",
  "minecraft_versions": [],
  "overwrite_files": [],
  "overwrite_folders": []
}
```

LeviLamroid's own `manifest.json` already matches this shape exactly
(`"type": "preload-native"`, `"entry": "liblevilamroid.so"`,
`"minecraft_versions": [...]`). The `.levipack` bundle wraps that manifest plus
`liblevilamroid.so` and the `data/` offset tables. **That is the real, functional
package for this project.**

## 3. The current `tooth.json` is invalid

Three concrete problems:

1. **Wrong `format_uuid`.** For format version 3 the value is fixed and must be
   `289f771f-2c9a-4d73-9f3f-8492495a924d`. The repo has
   `3a8b2c10-4d5e-4f6a-9b12-8e7c6b5a4d3f`, which lip's JSON-Schema validation
   will reject.
2. **Version drift.** `tooth.json` says `1.0.0-beta`; `manifest.json` says
   `1.0.1-beta`; the header says `1.0.0-beta`. `scripts/sync_version.py --check`
   catches this.
3. **Dead asset URL.** The manifest points at
   `.../releases/download/v{{version}}/LeviLamroid-1.0.0-beta-android-arm64.zip`,
   but the release workflow publishes
   `LeviLamroid-<version>-android-arm64-v8a.levipack`. The URL would 404.

## 4. Current decision: `tooth.json` is left untouched

`tooth.json` is not part of this migration. It is not read by the build, not
shipped in a release, and not touched by `scripts/sync_version.py`. It is left
byte-for-byte as it is.

The three problems above are therefore still present and worth revisiting if and
when a lip / Bedrinth listing is actually wanted. At that point the options are:

- **Drop it**, if you are not publishing through lip / Bedrinth - the simplest
  option for an Android-only native loader; or
- **Fix it**: correct the `format_uuid`, let `sync_version.py` own the version
  again, and point `urls` at the artifact the release workflow actually uploads.
  Verify first that a `platform` of `android-arm64` is accepted by whatever
  consumes it - the docs only demonstrate Windows RIDs, so treat Android support
  as unverified until you see it install.

The `.levipack` + `manifest.json` remains the artifact users install in
LeviLauncher.

---

## 5. What a release ships

`release.yml` publishes, for `android/arm64-v8a` only:

| Asset | What it is |
| --- | --- |
| `LeviLamroid-<version>-android-arm64-v8a.levipack` | the installable bundle: `liblevilamroid.so` + `manifest.json` + `data/` |
| `LeviLamroid-<version>-android-arm64-v8a.so` | the raw shared library, for manual placement |
| `SHA256SUMS.txt` | checksums for both |

plus a **build-provenance attestation** (`actions/attest-build-provenance`) that
cryptographically ties both binaries to the exact workflow run and commit, and
the composed release notes.
