"""Hole 2, Salt Mirror: a white salt crust on an endless mirror of brine, under an armada of silent monoliths.

Collision geometry comes from Content/Courses/saltmirror.json (written by Tools/courses/saltmirror.py).

Run inside the editor:  Tools/regen.sh saltmirror
"""

import importlib
import json
import math
import random
import sys

import unreal

sys.path.insert(0, "/Users/ken/Code/gravitygolf/Tools/levels")
import common as c  # noqa: E402

importlib.reload(c)

LEVEL = "/Game/Levels/SaltMirror"
COURSE = "/Users/ken/Code/gravitygolf/Content/Courses/saltmirror.json"
SURFACE = "/Game/Materials/M_GG_Surface.M_GG_Surface"
rng = random.Random(11)


def materials():
    return {
        "crust": c.material_instance("/Game/Materials/M_GG_Salt.M_GG_Salt", "MI_Salt_Crust", {"CellSize": 135.0},
                                     {"Tint": (0.82, 0.8, 0.76, 1)}),
        "pad": c.material_instance("/Game/Materials/M_GG_Salt.M_GG_Salt", "MI_Salt_Pad", {"CellSize": 38.0, "RimWidth": 0.09},
                                   {"Tint": (0.42, 0.37, 0.3, 1)}),
        "mirror": unreal.load_asset("/Game/Materials/M_GG_Mirror.M_GG_Mirror"),
        "obsidian": c.material_instance(SURFACE, "MI_Salt_Obsidian", {"Roughness": 0.07, "Metallic": 0.0},
                                        {"BaseColor": (0.008, 0.008, 0.01, 1)}),
        "glyph": c.material_instance("/Game/Materials/M_GG_Glow.M_GG_Glow", "MI_Salt_Glyph", {"Strength": 22.0},
                                     {"Color": (1.0, 0.55, 0.25, 1)}),
        "farGlyph": c.material_instance("/Game/Materials/M_GG_Glow.M_GG_Glow", "MI_Salt_FarGlyph", {"Strength": 3.0},
                                        {"Color": (1.0, 0.55, 0.25, 1)}),
        "black": c.material_instance(SURFACE, "MI_Cup", {"Roughness": 1.0}, {"BaseColor": (0.004, 0.004, 0.004, 1)}),
    }


def monolith(box, m):
    """A course monolith: far taller than it needs to be, with one line of light down each face."""
    cx, cy, cz = box["center"]
    sx, sy, sz = box["size"]
    height = 1500
    c.box((cx, cy, height / 2), (sx, sy, height), m["obsidian"], box["name"])
    for side in (-1, 1):
        c.box((cx + side * (sx / 2 + 1), cy, height * 0.5), (3, 6, height * 0.9), m["glyph"], f"{box['name']}_Glyph{side}",
              folder="Course/Glyphs")


def course_geometry(m):
    with open(COURSE) as f:
        course = json.load(f)
    for b in course["boxes"]:
        look = b["look"]
        if look == "monolith":
            monolith(b, m)
        elif look == "brine":
            c.box(b["center"], b["size"], m["mirror"], b["name"], folder="Landscape/Brine")
        elif look == "pad":
            # The soft pad shows as a round patch of dark damp salt, a hair above the crust.
            side = max(b["size"][0], b["size"][1])
            c.mesh(c.CYLINDER, (b["center"][0], b["center"][1], 0.4), (side * 1.25, side * 1.25, 0.8), material=m["pad"],
                   label=b["name"], folder="Course/Markers", cast_shadow=False)
        else:
            c.box(b["center"], b["size"], m[look], b["name"])
    return course


def markers(course, m):
    tee = course["tee"]["position"]
    c.mesh(c.CYLINDER, (tee[0], tee[1], 0.5), (200, 200, 1), material=m["obsidian"], label="TeeStone", folder="Course/Markers",
           cast_shadow=False)
    cup = course["cup"]
    x, y, z = cup["position"]
    for zone in course["noMachine"]:
        if zone["kind"] != "court":
            continue
        zx, zy, _ = zone["position"]
        size = zone["radius"] * 2
        c.mesh(c.CYLINDER, (zx, zy, 0.3), (size, size, 0.6), material=m["obsidian"], label="Court", folder="Course/Markers",
               cast_shadow=False)
        c.mesh(c.CYLINDER, (zx, zy, 0.2), (size + 24, size + 24, 0.4), material=m["glyph"], label="CourtEdge",
               folder="Course/Markers", cast_shadow=False)
    c.mesh(c.CYLINDER, (x, y, z + 0.9), (cup["radius"] * 2, cup["radius"] * 2, 1), material=m["glyph"], label="Cup",
           folder="Course/Markers", cast_shadow=False)
    c.mesh(c.CYLINDER, (x, y, z + 1.0), (cup["radius"] * 2 - 10, cup["radius"] * 2 - 10, 1), material=m["black"], label="CupHole",
           folder="Course/Markers", cast_shadow=False)
    c.mesh(c.CYLINDER, (x, y, z + 130), (2.5, 2.5, 260), material=m["obsidian"], label="FlagPole", folder="Course/Markers")
    for machine in course["attractors"]:
        mx, my, _ = machine["position"]
        c.dais((mx, my), 240, m["obsidian"], m["glyph"])


def mirror_and_horizon(m):
    c.mesh(c.PLANE, (0, 0, -3), (4000000, 4000000, 1), material=m["mirror"], label="Mirror", folder="Landscape",
           cast_shadow=False)


def armada(m):
    """The fantastical: hundreds of black monoliths hang over the mirror, silent, each doubled in its reflection."""
    count = 0
    for ring, (near, far, number) in enumerate([(9000, 30000, 26), (30000, 90000, 34), (90000, 260000, 26)]):
        for _ in range(number):
            angle = rng.uniform(-math.pi, math.pi)
            # Keep the line of play clear of anything close.
            if ring == 0 and abs(angle) < 0.5:
                continue
            distance = rng.uniform(near, far)
            height = rng.uniform(0.04, 0.16) * distance
            width = height * rng.uniform(0.12, 0.3)
            hover = rng.uniform(0.0, 0.08) * distance
            x = 3200 + math.cos(angle) * distance
            y = math.sin(angle) * distance
            c.box((x, y, hover + height / 2), (width * rng.uniform(0.3, 0.7), width, height), m["obsidian"], f"Armada{count}",
                  folder="Fantastical/Armada", rotation=(rng.uniform(-9, 9), rng.uniform(0, 360), rng.uniform(-9, 9)))
            count += 1

    # One colossal slab lies on its side high over the far end, a horizon line in the sky.
    c.box((70000, -20000, 26000), (9000, 120000, 2600), m["obsidian"], "SkySlab", folder="Fantastical",
          rotation=(4, 18, -6))
    c.box((70000, -20000, 24660), (9100, 110000, 60), m["farGlyph"], "SkySlabGlyph", folder="Fantastical", rotation=(4, 18, -6))

    # A stair of cubes climbs out of the brine beside the course, each a little higher and a little more turned.
    for i in range(14):
        t = i / 13.0
        size = 150 + 40 * i
        c.box((7600 + i * 800, 2600 + i * 500, 80 + t * t * 9000), (size, size, size), m["obsidian"], f"Stair{i}",
              folder="Fantastical/Stair", rotation=(i * 5, i * 11, i * 3))


c.fresh_level(LEVEL)
mats = materials()
# The sun sits just above the horizon ahead and a little left, so every monolith throws a long shadow toward you.
c.sky_and_sun(sun_pitch=-6.5, sun_yaw=196.0, fog_density=0.006, sun_kelvin=4300.0, sky_intensity=1.6, exposure_bias=0.4)
course_data = course_geometry(mats)
markers(course_data, mats)
c.player_start(course_data)
mirror_and_horizon(mats)
armada(mats)
c.LES.save_current_level()
