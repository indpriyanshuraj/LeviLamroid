#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
DIST_DIR="${SCRIPT_DIR}/dist"
LEVILAMROID_DIR="${SCRIPT_DIR}/.."

echo "=== 1. Building ExampleMod for Android ARM64 ==="
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

if [ -f "/data/data/com.termux/files/usr/bin/clang++" ]; then
    cmake -DCMAKE_CXX_COMPILER=/data/data/com.termux/files/usr/bin/clang++ \
          -DLEVILAMROID_ROOT="${LEVILAMROID_DIR}" ..
else
    cmake -DLEVILAMROID_ROOT="${LEVILAMROID_DIR}" ..
fi

make -j$(nproc || echo 4)

echo "=== 2. Packaging ExampleMod Distribution ==="
mkdir -p "${DIST_DIR}"
cp -f "${BUILD_DIR}/libexample_mod.so" "${DIST_DIR}/libexample_mod.so"
cp -f "${SCRIPT_DIR}/manifest.json" "${DIST_DIR}/manifest.json"

LEVIPACK_OUT="${DIST_DIR}/example_mod.levipack"
ZIP_OUT="${DIST_DIR}/ExampleMod-1.0.0-android-arm64.zip"

python3 -c "
import zipfile, os
levipack_files = [
    ('${DIST_DIR}/libexample_mod.so', 'libexample_mod.so'),
    ('${SCRIPT_DIR}/manifest.json', 'manifest.json')
]
with zipfile.ZipFile('${LEVIPACK_OUT}', 'w', zipfile.ZIP_DEFLATED) as zf:
    for src, arc in levipack_files:
        if os.path.isfile(src): zf.write(src, arc)

zip_files = [
    ('${DIST_DIR}/libexample_mod.so', 'libexample_mod.so'),
    ('${SCRIPT_DIR}/manifest.json', 'manifest.json'),
    ('${SCRIPT_DIR}/README.md', 'README.md')
]
with zipfile.ZipFile('${ZIP_OUT}', 'w', zipfile.ZIP_DEFLATED) as zf:
    for src, arc in zip_files:
        if os.path.isfile(src): zf.write(src, 'ExampleMod/' + arc)
"

# Copy to /sdcard/Download if present
if [ -d "/sdcard" ]; then
    mkdir -p /sdcard/Download 2>/dev/null || true
    cp -f "${LEVIPACK_OUT}" "/sdcard/Download/example_mod.levipack" 2>/dev/null || true
    cp -f "${ZIP_OUT}" "/sdcard/Download/ExampleMod-1.0.0-android-arm64.zip" 2>/dev/null || true

    # Auto-deploy to LeviLauncher profiles
    for profile_dir in \
        "/sdcard/Android/media/org.levimc.launcher/minecraft/com.mojang.minecraftpe/mods" \
        "/sdcard/Android/media/org.levimc.launcher/minecraft/Minecraft_1.26.45.1/mods"
    do
        if [ -d "${profile_dir}" ]; then
            mod_dir="${profile_dir}/ExampleMod"
            mkdir -p "${mod_dir}"
            cp -f "${DIST_DIR}/libexample_mod.so" "${mod_dir}/"
            cp -f "${SCRIPT_DIR}/manifest.json" "${mod_dir}/" 2>/dev/null || true
            echo "Deployed ExampleMod to ${mod_dir}"

            # Register in mods_config.json
            cfg_file="${profile_dir}/mods_config.json"
            python3 -c "
import json, os
p = '${cfg_file}'
if os.path.isfile(p):
    try:
        with open(p, 'r') as f: data = json.load(f)
        found = False
        for m in data:
            if m.get('name') == 'ExampleMod':
                m['enabled'] = True
                found = True
        if not found:
            data.append({'name': 'ExampleMod', 'enabled': True, 'order': len(data)})
        with open(p, 'w') as f: json.dump(data, f, indent=2)
    except Exception as e: print(f'mods_config update error: {e}')
"
        fi
    done
fi

echo "=== Successfully built, packaged, and deployed ExampleMod ==="
