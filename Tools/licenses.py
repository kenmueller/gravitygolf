#!/usr/bin/env python3
"""Gathers the licence texts of the open software inside the game into one file that ships with the web build:
Build/Wasm/Template/licenses.txt (UAT stages a project's Build/Wasm/Template beside the page).

    python3 Tools/licenses.py

The list follows Epic's own record of what a web build carries (Engine/Platforms/Wasm/Docs/THIRD-PARTY.md, the rows
marked for web builds) and the engine's records for FreeType, libpng, zlib, Opus and BLAKE3. The texts are copied
from the engine source at ~/UE59 and the Emscripten SDK at ~/emsdk, never retyped. The game's Credits screen carries
the short notices; this file carries the full texts those licences ask to be passed on.
"""

import os
import sys

HOME = os.path.expanduser("~")
UE = os.environ.get("UE_ROOT", f"{HOME}/UE59")
EMSDK = os.environ.get("WASM_EMSDK_ROOT", f"{HOME}/emsdk")
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = f"{ROOT}/Build/Wasm/Template/licenses.txt"

PARTS = [
    ("FreeType 2.14.1 (used under the FreeType Project Licence)", [
        f"{UE}/Engine/Source/ThirdParty/FreeType2/FreeType2-2.14.1/LICENSE.TXT",
        f"{UE}/Engine/Source/ThirdParty/FreeType2/FreeType2-2.14.1/docs/FTL.TXT"]),
    ("libpng 1.6.44", [f"{UE}/Engine/Source/ThirdParty/libPNG/libPNG-1.6.44/LICENSE"]),
    ("zlib", [f"{UE}/Engine/Source/ThirdParty/zlib/1.3/LICENSE"]),
    ("Opus 1.1", [f"{UE}/Engine/Source/ThirdParty/libOpus/opus-1.1/COPYING"]),
    ("BLAKE3 1.3.1", [f"{UE}/Engine/Source/ThirdParty/BLAKE3/1.3.1/LICENSE"]),
    ("Emscripten (the runtime of the browser version)", [f"{EMSDK}/upstream/emscripten/LICENSE"]),
    ("musl libc (in the browser version)", [f"{EMSDK}/upstream/emscripten/system/lib/libc/musl/COPYRIGHT"]),
    ("LLVM libc++ and compiler-rt (in the browser version)", [f"{EMSDK}/upstream/emscripten/system/lib/libcxx/LICENSE.TXT"]),
    ("Dawn's WebGPU bindings for Emscripten (in the browser version)", [
        f"{UE}/Engine/Platforms/Wasm/Build/ThirdParty/emdawnwebgpu/webgpu_cpp/LICENSE",
        f"{UE}/Engine/Platforms/Wasm/Build/ThirdParty/emdawnwebgpu/webgpu/src/LICENSE"]),
]

HEAD = """GRAVITY GOLF: NOTICES AND LICENCES

Gravity Golf uses Unreal(R) Engine. Unreal(R) is a trademark or registered trademark of Epic Games, Inc. in the
United States of America and elsewhere. Unreal(R) Engine, Copyright 1998 - 2026, Epic Games, Inc. All rights reserved.

Oodle and Bink Audio are Epic Games Tools, used under the Unreal Engine licence.
Scanned stone, sand and plants are Quixel Megascans, from Fab.

The game also includes the open software below. Each part is under its own licence, given here in full.
"""

missing = [path for _, paths in PARTS for path in paths if not os.path.isfile(path)]
if missing:
    sys.exit("licence texts not found:\n  " + "\n  ".join(missing))
os.makedirs(os.path.dirname(OUT), exist_ok=True)
with open(OUT, "w", encoding="utf-8") as out:
    out.write(HEAD)
    for title, paths in PARTS:
        out.write("\n\n" + "=" * 100 + "\n" + title + "\n" + "=" * 100 + "\n\n")
        for index, path in enumerate(paths):
            if index:
                out.write("\n" + "-" * 100 + "\n\n")
            with open(path, encoding="utf-8", errors="replace") as text:
                out.write(text.read().rstrip() + "\n")
print(f"wrote {OUT} ({os.path.getsize(OUT) // 1024} KB, {len(PARTS)} parts)")
