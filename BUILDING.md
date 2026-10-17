# Building LeviLamroid

LeviLamroid is built with **xmake**. There is no second build system: the CI and
every published release are produced by `xmake.lua` and nothing else.

---

## Prerequisites

- **Android NDK** r28c or newer (`r29` also works). Set `ANDROID_NDK_HOME`, or
  pass `--ndk=<path>` explicitly.
- **xmake** >= 2.9.8. Install with
  `curl -fsSL https://xmake.io/shget.text | bash` (or your package manager).

## Build

```bash
# configure once
xmake f -p android -a arm64-v8a -m release --ndk=$ANDROID_NDK_HOME -y

# build (+ package automatically)
xmake -y

# outputs
#   build/android/arm64-v8a/release/liblevilamroid.so
#   build/android/arm64-v8a/release/levilamroid.levipack
```

Useful variants:

```bash
# debug build (symbols, no LTO/strip)
xmake f -p android -a arm64-v8a -m debug --ndk=$ANDROID_NDK_HOME -y && xmake -y

# verbose engine-discovery logging
xmake f -p android -a arm64-v8a -m release --ll_verbose_log=y -y && xmake -y

# clean
xmake clean --all
```

The `.levipack` is produced automatically after every successful build: the
`after_build` hook bundles `liblevilamroid.so`, `manifest.json` and `data/` into
it.

## Deploy to a device

```bash
xmake deploy
```

Copies the built `.levipack` into the `mods/LeviLamroid/` directory of every
LeviLauncher profile it finds under
`/sdcard/Android/media/org.levimc.launcher/minecraft/`. Build first.

## Build cache

xmake keeps a **built-in content-addressed object cache** (`build.ccache`). It
preprocesses each translation unit, hashes the result together with the compile
flags, and stores the object file, so a recompile with unchanged inputs is a
cache hit. It is on by default locally and lives under the build directory
(`build/.build_cache`).

Two things that keep the hit rate high, and that this repo already follows:

- Use **relative** include directories (`add_includedirs("include")`), never an
  absolute path like `os.projectdir()`; absolute paths change the cache key and
  cause 0% hits.
- Do not enable it for C++ modules; it is incompatible with module builds.

Tuning knobs (all optional):

```bash
# share one cache across every checkout on this machine
xmake f --policies=build.ccache.global_storage:y

# put the cache somewhere explicit
xmake f --ccachedir=/path/to/cache

# turn it off entirely
xmake f --ccache=n
```

`xmake -v` prints a `build cache stats:` block (hit rate, hits/misses, remote
hits, preprocess failures) so you can see whether it is actually helping.

## Editor / IDE support

Generate the files editors want from the same `xmake.lua`, so they cannot drift
from the real build:

```bash
xmake project -k compile_commands .vscode   # clangd / ccls / VS Code
xmake project -k vsxmake -m "debug,release" # Visual Studio
xmake project -k ninja                      # alternate backend
```

These outputs are gitignored. To regenerate `compile_commands.json` on every
build, uncomment the `plugin.compile_commands.autoupdate` rule in `xmake.lua`.

---

## Versioning

The version is defined in one place and mirrored by a script:

```bash
python3 scripts/sync_version.py                 # print the current version
python3 scripts/sync_version.py --check          # fail if the repo has drifted
python3 scripts/sync_version.py --set 1.0.2-beta # update every tracked file
```

It keeps `xmake.lua`, `manifest.json` and `include/levilamroid/LeviLamroid.h` in
step. `--check` runs in CI, so a PR that bumps one file but not the others is
rejected before it can ship a mislabelled `.levipack`.

## Releases

Releases are cut by pushing a tag:

```bash
python3 scripts/sync_version.py --set 1.0.2-beta
git commit -am "Release v1.0.2-beta"
git tag v1.0.2-beta
git push origin main --tags
```

`release.yml` then validates the version, builds android/arm64-v8a with xmake,
attaches the `.levipack` and the raw `.so` plus `SHA256SUMS.txt`, publishes the
GitHub Release, and records build provenance. A manual `workflow_dispatch` run
with a version input does the same without a tag.

See [PACKAGING.md](PACKAGING.md) for what the release assets are.

---

## Adding this to the README

Paste the following into `README.md` (indented here so the fences stay readable):

    ## Building

    LeviLamroid is built with **xmake**. See [BUILDING.md](BUILDING.md) for the
    full guide.

    ```bash
    xmake f -p android -a arm64-v8a -m release --ndk=$ANDROID_NDK_HOME -y
    xmake -y
    ```
