#!/usr/bin/env python3
"""Single source of truth for the LeviLamroid version.

The canonical version lives in xmake.lua's set_version() (numeric). The full
version (which may add a suffix such as "-beta") is mirrored into manifest.json
and the runtime header. This script checks that they agree, or propagates a
version to them.

Tracked files:
    xmake.lua                                set_version("X")       (numeric)
    manifest.json                            "version": "X[-suffix]" (full)
    include/levilamroid/LeviLamroid.h        LEVILAMROID_VERSION_*  (full)

tooth.json is deliberately NOT tracked - it is a legacy lip manifest that is not
read by the build or shipped in a release, and is left untouched.

Usage:
    python3 scripts/sync_version.py                     # print the full version
    python3 scripts/sync_version.py --check              # exit 1 if the repo drifts
    python3 scripts/sync_version.py --check 1.0.2-beta    # exit 1 unless all == arg
    python3 scripts/sync_version.py --set 1.0.2-beta      # write it everywhere
"""
import argparse
import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
XMAKE = ROOT / "xmake.lua"
MANIFEST = ROOT / "manifest.json"
HEADER = ROOT / "include" / "levilamroid" / "LeviLamroid.h"

VER_RE = re.compile(r"^(\d+)\.(\d+)\.(\d+)(?:-(.+))?$")


def die(msg):
    sys.exit(f"error: {msg}")


def numeric(v):
    return v.split("-")[0] if v else v


def parse(v):
    if not VER_RE.match(v):
        die(f"version '{v}' is not MAJOR.MINOR.PATCH[-suffix]")
    return v


def read_xmake():
    m = re.search(r'set_version\(\s*"([^"]+)"\s*\)', XMAKE.read_text())
    if not m:
        die("set_version() not found in xmake.lua")
    return m.group(1)


def read_json_version(path):
    try:
        return json.loads(path.read_text()).get("version")
    except FileNotFoundError:
        return None


def read_header_version():
    m = re.search(r'LEVILAMROID_VERSION_STRING\s+"([^"]+)"', HEADER.read_text())
    return m.group(1) if m else None


def write_xmake(v):
    text = re.sub(r'(set_version\(\s*)"[^"]*"', r'\g<1>"' + numeric(v) + '"', XMAKE.read_text())
    XMAKE.write_text(text)


def write_json_version(path, v):
    data = json.loads(path.read_text())
    data["version"] = v
    path.write_text(json.dumps(data, indent=2) + "\n")


def write_header(v):
    major, minor, patch = numeric(v).split(".")
    text = HEADER.read_text()
    text = re.sub(r"(#define LEVILAMROID_VERSION_MAJOR\s+)\d+", r"\g<1>" + major, text)
    text = re.sub(r"(#define LEVILAMROID_VERSION_MINOR\s+)\d+", r"\g<1>" + minor, text)
    text = re.sub(r"(#define LEVILAMROID_VERSION_PATCH\s+)\d+", r"\g<1>" + patch, text)
    text = re.sub(r'(#define LEVILAMROID_VERSION_STRING\s+)"[^"]*"', r'\g<1>"' + v + '"', text)
    HEADER.write_text(text)


def cmd_set(v):
    parse(v)
    for p in (XMAKE, MANIFEST, HEADER):
        if not p.exists():
            die(f"missing {p}")
    write_xmake(v)
    write_json_version(MANIFEST, v)
    write_header(v)
    print(f"version set to {v} in xmake.lua, manifest.json, LeviLamroid.h")


def cmd_check(expected):
    xmake_v = read_xmake()
    manifest_v = read_json_version(MANIFEST)
    header_v = read_header_version()

    if expected:
        parse(expected)
        rows = [
            ("xmake.lua", xmake_v, xmake_v == numeric(expected)),
            ("manifest.json", manifest_v, manifest_v == expected),
            ("LeviLamroid.h", header_v, header_v == expected),
        ]
        target = expected
    else:
        # Internal consistency: xmake is numeric; manifest and header must be
        # equal to each other and share xmake's numeric core.
        full = manifest_v
        rows = [
            ("xmake.lua", xmake_v, numeric(xmake_v) == numeric(full or "")),
            ("manifest.json", manifest_v, manifest_v is not None),
            ("LeviLamroid.h", header_v, header_v == full),
        ]
        target = full or "(none)"

    print(f"target version: {target}")
    bad = False
    for name, value, ok in rows:
        print(f"  {'OK      ' if ok else 'MISMATCH'} {name}: {value}")
        bad = bad or not ok

    if bad:
        hint = expected or (manifest_v or "1.0.0")
        print("\nVersion drift detected. Fix with: python3 scripts/sync_version.py --set " + hint)
        sys.exit(1)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", nargs="?", const="", metavar="VERSION")
    ap.add_argument("--set", dest="setver", metavar="VERSION")
    args = ap.parse_args()

    if args.setver:
        cmd_set(args.setver)
    elif args.check is not None:
        cmd_check(args.check)
    else:
        print(read_json_version(MANIFEST) or read_xmake())


if __name__ == "__main__":
    main()
