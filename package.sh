#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
DIST_DIR="${SCRIPT_DIR}/dist"

echo "=== 1. Building LeviLamroid for Android ARM64 ==="
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"
if [ -f "/data/data/com.termux/files/usr/bin/clang++" ]; then
    cmake -DCMAKE_CXX_COMPILER=/data/data/com.termux/files/usr/bin/clang++ ..
else
    cmake ..
fi
make -j$(nproc || echo 4)

echo "=== 2. Packaging LeviLamroid Distribution ==="
mkdir -p "${DIST_DIR}"
cp -f "${BUILD_DIR}/liblevilamroid.so" "${DIST_DIR}/liblevilamroid.so"
cp -f "${SCRIPT_DIR}/manifest.json" "${DIST_DIR}/manifest.json" 2>/dev/null || true

# Build levipack and zip
LEVIPACK_OUT="${DIST_DIR}/levilamroid.levipack"
ZIP_OUT="${DIST_DIR}/LeviLamroid-1.0.0-beta-android-arm64.zip"

python3 -c "
import zipfile, os
levipack_files = [
    ('${DIST_DIR}/liblevilamroid.so', 'liblevilamroid.so'),
    ('${SCRIPT_DIR}/manifest.json', 'manifest.json')
]
with zipfile.ZipFile('${LEVIPACK_OUT}', 'w', zipfile.ZIP_DEFLATED) as zf:
    for src, arc in levipack_files:
        if os.path.isfile(src): zf.write(src, arc)

zip_files = [
    ('${DIST_DIR}/liblevilamroid.so', 'liblevilamroid.so'),
    ('${SCRIPT_DIR}/manifest.json', 'manifest.json'),
    ('${SCRIPT_DIR}/README.md', 'README.md'),
    ('${SCRIPT_DIR}/FUNCTIONS.md', 'FUNCTIONS.md')
]
with zipfile.ZipFile('${ZIP_OUT}', 'w', zipfile.ZIP_DEFLATED) as zf:
    for src, arc in zip_files:
        if os.path.isfile(src): zf.write(src, 'LeviLamroid/' + arc)
"

# Copy to /sdcard/Download and /sdcard/MG if sdcard is present
if [ -d "/sdcard" ]; then
    mkdir -p /sdcard/Download /sdcard/MG 2>/dev/null || true
    cp -f "${LEVIPACK_OUT}" "/sdcard/Download/levilamroid.levipack" 2>/dev/null || true
    cp -f "${LEVIPACK_OUT}" "/sdcard/MG/levilamroid.levipack" 2>/dev/null || true
    cp -f "${ZIP_OUT}" "/sdcard/Download/LeviLamroid-1.0.0-beta-android-arm64.zip" 2>/dev/null || true
    cp -f "${ZIP_OUT}" "/sdcard/MG/LeviLamroid-1.0.0-beta-android-arm64.zip" 2>/dev/null || true

    # Auto-deploy to LeviLauncher profiles as core loader
    for profile_dir in \
        "/sdcard/Android/media/org.levimc.launcher/minecraft/com.mojang.minecraftpe/mods" \
        "/sdcard/Android/media/org.levimc.launcher/minecraft/Minecraft_1.26.45.1/mods"
    do
        if [ -d "${profile_dir}" ]; then
            mod_dir="${profile_dir}/LeviLamroid"
            mkdir -p "${mod_dir}"
            cp -f "${DIST_DIR}/liblevilamroid.so" "${mod_dir}/"
            cp -f "${SCRIPT_DIR}/manifest.json" "${mod_dir}/" 2>/dev/null || true
            echo "Deployed LeviLamroid to ${mod_dir}"

            # Enable in mods_config.json with order 0 (load first)
            cfg_file="${profile_dir}/mods_config.json"
            python3 -c "
import json, os
p = '${cfg_file}'
if os.path.isfile(p):
    try:
        with open(p, 'r') as f: data = json.load(f)
        found = False
        for m in data:
            if m.get('name') == 'LeviLamroid':
                m['enabled'] = True
                m['order'] = 0
                found = True
        if not found:
            data.insert(0, {'name': 'LeviLamroid', 'enabled': True, 'order': 0})
        for idx, m in enumerate(data): m['order'] = idx
        with open(p, 'w') as f: json.dump(data, f, indent=2)
    except Exception as e: print(f'mods_config update error: {e}')
"
        fi
    done
fi

echo "=== Successfully built, packaged, and deployed LeviLamroid v1.0.0-beta ==="
