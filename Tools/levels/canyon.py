"""Hole 3, The Narrows: two galleries in a red slot canyon, joined by one doorway, under a ring of stone.

Collision geometry comes from Content/Courses/canyon.json (written by Tools/courses/canyon.py).

Run inside the editor:  Tools/regen.sh canyon
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

LEVEL = "/Game/Levels/Canyon"
COURSE = "/Users/ken/Code/gravitygolf/Content/Courses/canyon.json"
SURFACE = "/Game/Materials/M_GG_Surface.M_GG_Surface"
OUTER = {"Wall_South", "Wall_North", "Wall_West", "Wall_East", "Lower_End"}
rng = random.Random(23)


def materials():
    return {
        "floor": c.scan_material("MI_Canyon_Floor", "Red_Desert_Sand", 240, (0.8, 0.7, 0.62, 1)),
        "sand": c.scan_material("MI_Canyon_Drift", "Red_Desert_Sand", 160, (1.1, 0.98, 0.86, 1)),
        "rock": c.scan_material("MI_Canyon_Rock", "Eroded_Sandstone_Cliff", 700, (0.95, 0.56, 0.36, 1)),
        "stone": c.scan_material("MI_Canyon_Limestone", "Roman_Limestone_Wall", 320, (1.0, 0.9, 0.78, 1)),
        "glyph": c.material_instance("/Game/Materials/M_GG_Glow.M_GG_Glow", "MI_Canyon_Glyph", {"Strength": 16.0},
                                     {"Color": (0.3, 1.0, 0.85, 1)}),
        "dais": c.material_instance("/Game/Materials/M_GG_Glow.M_GG_Glow", "MI_Canyon_Dais", {"Strength": 2.5},
                                    {"Color": (0.3, 1.0, 0.85, 1)}),
        "black": c.material_instance(SURFACE, "MI_Cup", {"Roughness": 1.0}, {"BaseColor": (0.004, 0.004, 0.004, 1)}),
    }


def scree(box, m, boulder):
    """Fallen rock along the foot of a wall, on every face that looks into a gallery."""
    cx, cy, _ = box["center"]
    sx, sy, _ = box["size"]
    along_x = sx >= sy
    length = sx if along_x else sy
    half_thick = (sy if along_x else sx) / 2
    count = max(1, int(length / 420))
    for side in (-1, 1):
        for i in range(count):
            if rng.random() < 0.35:
                continue
            t = (i + rng.uniform(0.1, 0.9)) / count - 0.5
            offset = side * (half_thick + rng.uniform(-10, 14))
            x = cx + (t * length if along_x else offset)
            y = cy + (offset if along_x else t * length)
            size = rng.uniform(0.35, 1.0)
            c.static_mesh(boulder, (x, y, -8 * size), (rng.uniform(-20, 20), rng.uniform(0, 360), rng.uniform(-20, 20)),
                          (size, size, size), m["rock"], f"{box['name']}_Scree", "Landscape/Scree")


def walls(course, m, cliff, boulder):
    cliff_bounds = cliff.get_bounds()
    length = cliff_bounds.box_extent.x * 2
    depth = cliff_bounds.box_extent.y * 2
    height = cliff_bounds.box_extent.z * 2
    base = cliff_bounds.origin.z - cliff_bounds.box_extent.z
    for b in course["boxes"]:
        if b["look"] != "rock":
            continue
        cx, cy, _ = b["center"]
        sx, sy, _ = b["size"]
        along_x = sx >= sy
        run = sx if along_x else sy
        outer = b["name"] in OUTER
        tall = 1900 if outer else rng.uniform(430, 520)
        c.box((cx, cy, tall / 2 - 20), (sx, sy, tall), m["rock"], b["name"])
        scree(b, m, boulder)

        if outer:
            # Clad the canyon's outer walls in scanned cliffs. Each cliff is a solid mass about 13 m deep, so it
            # stands behind the wall's inner face with only a little of its front leaning into the slot.
            scale_z = 2100 / height
            scale_len = 1.2
            scale_depth = 0.6
            half_depth = depth * scale_depth * 0.5
            if b["name"] == "Lower_End":
                c.static_mesh(cliff, (cx, cy, -base * scale_z - 60), (0, 0, 0),
                              ((sx + 60) / length, (sy + 60) / depth, scale_z * 0.9), m["rock"], "Lower_End_Cliff",
                              "Landscape/Cliffs")
                continue
            # Which way is "out"? Away from the middle of the course.
            out_x = 0 if along_x else (1 if cx > 3000 else -1)
            out_y = (1 if cy > 450 else -1) if along_x else 0
            face_x = cx - out_x * sx / 2
            face_y = cy - out_y * sy / 2
            step = length * scale_len * 0.6
            count = max(1, int(run / step) + 1)
            for i in range(count):
                t = (i + 0.5) / count - 0.5
                jitter = rng.uniform(-120, 120)
                location = (face_x + out_x * (half_depth - 45) + (t * run + jitter if along_x else 0),
                            face_y + out_y * (half_depth - 45) + (0 if along_x else t * run + jitter),
                            -base * scale_z - 80)
                yaw = (0 if rng.random() < 0.5 else 180) if along_x else (90 if rng.random() < 0.5 else -90)
                c.static_mesh(cliff, location, (0, yaw + rng.uniform(-4, 4), 0),
                              (scale_len, scale_depth, scale_z * rng.uniform(0.85, 1.15)), m["rock"], f"{b['name']}_Cliff",
                              "Landscape/Cliffs")
        else:
            # The inner ribs are capped with loose boulders so their tops read as broken rock, not a box.
            count = max(2, int(run / 260))
            for i in range(count):
                t = (i + rng.uniform(0.1, 0.9)) / count - 0.5
                x = cx + (t * run if along_x else rng.uniform(-20, 20))
                y = cy + (rng.uniform(-20, 20) if along_x else t * run)
                size = rng.uniform(1.6, 3.0)
                c.static_mesh(boulder, (x, y, tall - 45), (rng.uniform(-15, 15), rng.uniform(0, 360), rng.uniform(-15, 15)),
                              (size, size, size * rng.uniform(0.9, 1.5)), m["rock"], f"{b['name']}_Cap", "Landscape/Boulders")


def doorway(m):
    """The doorway between the galleries is the one thing here that was built: two piers and a lintel."""
    for x in (2300 - 45, 3100 + 45):
        c.box((x, 450, 270), (150, 190, 540), m["stone"], "DoorPier", folder="Ruins")
        c.box((x, 450, 20), (190, 230, 40), m["stone"], "DoorPierBase", folder="Ruins")
        c.box((x, 450, 525), (190, 230, 30), m["stone"], "DoorPierCap", folder="Ruins")
    c.box((2700, 450, 580), (1100, 200, 80), m["stone"], "DoorLintel", folder="Ruins")
    for side in (-1, 1):
        c.box((2700, 450 + side * 101, 580), (900, 3, 8), m["glyph"], "DoorGlyph", folder="Ruins")


def markers(course, m):
    tee = course["tee"]["position"]
    c.mesh(c.CYLINDER, (tee[0], tee[1], 0.5), (200, 200, 1), material=m["stone"], label="TeeStone", folder="Course/Markers",
           cast_shadow=False)
    cup = course["cup"]
    x, y, z = cup["position"]
    for zone in course["noMachine"]:
        if zone["kind"] != "court":
            continue
        zx, zy, _ = zone["position"]
        size = zone["radius"] * 2
        c.mesh(c.CYLINDER, (zx, zy, 0.3), (size, size, 0.6), material=m["stone"], label="Court", folder="Course/Markers",
               cast_shadow=False)
        c.mesh(c.CYLINDER, (zx, zy, 0.2), (size + 24, size + 24, 0.4), material=m["glyph"], label="CourtEdge",
               folder="Course/Markers", cast_shadow=False)
    c.mesh(c.CYLINDER, (x, y, z + 0.9), (cup["radius"] * 2, cup["radius"] * 2, 1), material=m["black"], label="Cup",
           folder="Course/Markers", cast_shadow=False)
    c.mesh(c.CYLINDER, (x, y, z + 130), (2.5, 2.5, 260), material=m["stone"], label="FlagPole", folder="Course/Markers")
    for machine in course["attractors"]:
        mx, my, _ = machine["position"]
        c.dais((mx, my), 130, m["stone"], m["dais"])


def sky_ring(m, boulder):
    """The fantastical: a ring of stone lies over the canyon like a lid, and boulders hang in the shaft of light."""
    center = (3000.0, 450.0, 3600.0)
    radius = 3300.0
    segments = 64
    for i in range(segments):
        if i in (9, 10, 30, 31, 32, 50):
            continue
        angle = 2 * math.pi * i / segments
        x = center[0] + math.cos(angle) * radius
        y = center[1] + math.sin(angle) * radius
        length = 2 * math.pi * radius / segments * 1.03
        yaw = math.degrees(angle) + 90
        c.box((x, y, center[2]), (length, 420, 300), m["stone"], f"Ring{i}", folder="Fantastical/Ring", rotation=(0, yaw, 0))
        c.box((x, y, center[2] - 152), (length * 0.9, 40, 6), m["glyph"], f"RingGlyph{i}", folder="Fantastical/Ring",
              rotation=(0, yaw, 0))
    for i in range(22):
        x = rng.uniform(300, 5700)
        y = rng.uniform(-300, 1200)
        z = rng.uniform(700, 2400)
        size = rng.uniform(1.2, 4.5)
        c.static_mesh(boulder, (x, y, z), (rng.uniform(0, 360), rng.uniform(0, 360), rng.uniform(0, 360)),
                      (size, size, size), m["rock"], f"Floater{i}", "Fantastical/Floaters")


c.fresh_level(LEVEL)
mats = materials()
# High sun raking across the slot: one wall burns orange, the other glows with its bounce.
c.sky_and_sun(sun_pitch=-64.0, sun_yaw=38.0, fog_density=0.035, sun_kelvin=5600.0, sky_intensity=1.5, exposure_bias=0.0)
with open(COURSE) as f:
    course_data = json.load(f)
cliff_mesh = c.fab_mesh("Desert_Western_Cliff_Layered_XL_14")
boulder_mesh = c.fab_mesh("Canyon_Sandstone_Boulder")
for b in course_data["boxes"]:
    if b["look"] == "floor":
        c.box(b["center"], b["size"], mats["floor"], b["name"])
walls(course_data, mats, cliff_mesh, boulder_mesh)
doorway(mats)
markers(course_data, mats)
c.player_start(course_data)
sky_ring(mats, boulder_mesh)
c.LES.save_current_level()
