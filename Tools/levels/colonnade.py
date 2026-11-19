"""Hole 7, Dead Weight: a roofless hall of columns standing in still water at dawn.

A marble floor a hand above the water, low limestone walls, and columns in every direction to the fog. Some columns
have come apart and their drums hang in the air above the stumps. The sun is just up, straight down the hall.

Collision geometry comes from Content/Courses/colonnade.json (written by Tools/courses/colonnade.py).

Run inside the editor:  Tools/regen.sh colonnade
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

LEVEL = "/Game/Levels/Colonnade"
COURSE = "/Users/ken/Code/gravitygolf/Content/Courses/colonnade.json"
SURFACE = "/Game/Materials/M_GG_Surface.M_GG_Surface"
GLOW = "/Game/Materials/M_GG_Glow.M_GG_Glow"
PILLARS = "/Game/Ancient_Pillars/Meshes/"
WATER_Z = -9.0
rng = random.Random(53)


def materials():
    return {
        "floor": c.scan_material("MI_Colonnade_Floor", "Beige_Marble_Tiles", 300, (0.92, 0.88, 0.84, 1), roughness=0.5),
        "wall": c.scan_material("MI_Colonnade_Wall", "Roman_Limestone_Wall", 320, (0.98, 0.93, 0.88, 1)),
        "mirror": unreal.load_asset("/Game/Materials/M_GG_Mirror.M_GG_Mirror"),
        "glyph": c.material_instance(GLOW, "MI_Colonnade_Glyph", {"Strength": 9.0}, {"Color": (1.0, 0.72, 0.42, 1)}),
        "bronze": c.material_instance(SURFACE, "MI_Colonnade_Bronze", {"Roughness": 0.4, "Metallic": 1.0},
                                      {"BaseColor": (0.25, 0.15, 0.06, 1)}),
        "black": c.material_instance(SURFACE, "MI_Cup", {"Roughness": 1.0}, {"BaseColor": (0.004, 0.004, 0.004, 1)}),
    }


def dressed_wall(box, m):
    """Low limestone walls with a plinth, a cap and a seam of light."""
    cx, cy, cz = box["center"]
    sx, sy, sz = box["size"]
    base = cz - sz / 2
    c.box((cx, cy, cz), (sx, sy, sz), m["wall"], box["name"])
    c.box((cx, cy, base + 14), (sx + 24, sy + 24, 28), m["wall"], box["name"] + "_Plinth", folder="Ruins/Walls")
    c.box((cx, cy, base + sz + 10), (sx + 20, sy + 20, 20), m["wall"], box["name"] + "_Cap", folder="Ruins/Walls")
    along_x = sx >= sy
    for side in (-1, 1):
        if along_x:
            c.box((cx, cy + side * (sy / 2 + 1), base + sz * 0.7), (sx - 60, 2, 4), m["glyph"], box["name"] + "_Seam",
                  folder="Ruins/Walls")
        else:
            c.box((cx + side * (sx / 2 + 1), cy, base + sz * 0.7), (2, sy - 60, 4), m["glyph"], box["name"] + "_Seam",
                  folder="Ruins/Walls")


def course_geometry(course, m):
    for b in course["boxes"]:
        if b["look"] == "wall":
            dressed_wall(b, m)
        else:
            c.box(b["center"], b["size"], m["floor"], b["name"])


def markers(course, m):
    tee = course["tee"]["position"]
    c.mesh(c.CYLINDER, (tee[0], tee[1], 0.5), (200, 200, 1), material=m["wall"], label="TeeStone", folder="Course/Markers",
           cast_shadow=False)
    cup = course["cup"]
    x, y, z = cup["position"]
    c.mesh(c.CYLINDER, (x, y, z + 0.4), (cup["radius"] * 2 + 16, cup["radius"] * 2 + 16, 0.8), material=m["glyph"],
           label="CupRim", folder="Course/Markers", cast_shadow=False)
    c.mesh(c.CYLINDER, (x, y, z + 0.9), (cup["radius"] * 2, cup["radius"] * 2, 1), material=m["black"], label="Cup",
           folder="Course/Markers", cast_shadow=False)
    c.mesh(c.CYLINDER, (x, y, z + 130), (2.5, 2.5, 260), material=m["bronze"], label="FlagPole", folder="Course/Markers")
    for machine in course["attractors"]:
        mx, my, _ = machine["position"]
        c.dais((mx, my), 130, m["wall"], m["glyph"])


def inside_course(course, x, y, margin):
    """Is a point on or near anything the course is made of? Columns keep clear of it."""
    for b in course["boxes"]:
        cx, cy, _ = b["center"]
        sx, sy, _ = b["size"]
        if abs(x - cx) < sx / 2 + margin and abs(y - cy) < sy / 2 + margin:
            return True
    return False


def columns(course):
    """The hall of columns: a grid standing in the water, out to the fog. Near the course most are whole; further
    out more are stumps, and over some of the stumps the missing drums hang in the air, rising."""
    whole = [unreal.load_asset(f"{PILLARS}Joined/SM_Pillars_0{i}") for i in (1, 2, 3, 4, 7)]
    stump = unreal.load_asset(f"{PILLARS}Joined/SM_Pillars_06")
    drums = [unreal.load_asset(f"{PILLARS}Parts/SM_Pillars_Parts_{i:02d}") for i in (1, 6, 9, 10, 17, 18)]
    count = 0
    for row in range(-9, 12):
        y = 250 + row * 1250
        for column in range(-8, 18):
            x = column * 1000 + (500 if row % 2 else 0)
            if inside_course(course, x, y, 190):
                continue
            distance = math.hypot(x - 3000, y - 250)
            # The further from the hall, the more often a column has fallen.
            broken = rng.random() < min(0.75, 0.12 + distance / 22000.0)
            scale = rng.uniform(2.3, 2.9)
            yaw = rng.uniform(0, 360)
            if not broken:
                c.static_mesh(rng.choice(whole), (x, y, WATER_Z - 10), (0, yaw, 0), (scale, scale, scale), None, f"Column{count}",
                              "Landscape/Columns")
            else:
                c.static_mesh(stump, (x, y, WATER_Z - 10), (0, yaw, 0), (scale * 0.75, scale * 0.75, scale * 0.75), None,
                              f"Stump{count}", "Landscape/Columns")
                if rng.random() < 0.6:
                    # Its drums never fell: they hang over the stump, further apart the higher they are.
                    height = 420.0
                    for i in range(rng.randint(2, 5)):
                        height += rng.uniform(260, 520) * (1.0 + 0.35 * i)
                        drum = rng.choice(drums)
                        actor = c.static_mesh(drum, (x + rng.uniform(-40, 40) * i, y + rng.uniform(-40, 40) * i, height),
                                              (rng.uniform(-10, 10) * i, rng.uniform(0, 360), rng.uniform(-10, 10) * i),
                                              (scale * 0.75, scale * 0.75, scale * 0.75), None, f"Drum{count}_{i}",
                                              "Fantastical/Drums")
                        actor.static_mesh_component.set_collision_profile_name("NoCollision")
            count += 1
    return count


c.fresh_level(LEVEL)
mats = materials()
# Dawn: the sun is just up at the far end of the hall and a little to the right, so every column lays a long shadow
# across the floor toward the tee and the water is a sheet of light.
c.sky_and_sun(sun_pitch=-7.0, sun_yaw=168.0, fog_density=0.03, sun_kelvin=3900.0, sky_intensity=1.5, exposure_bias=0.3)
with open(COURSE) as f:
    course_data = json.load(f)
course_geometry(course_data, mats)
markers(course_data, mats)
c.player_start(course_data)
c.mesh(c.PLANE, (3000, 250, WATER_Z), (4000000, 4000000, 1), material=mats["mirror"], label="Water", folder="Landscape",
       cast_shadow=False)
placed = columns(course_data)
unreal.log(f"GG: colonnade built with {placed} columns")
c.LES.save_current_level()
