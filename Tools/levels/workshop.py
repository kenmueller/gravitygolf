"""The Workshop: a drafting table without an edge, under a late sky, where players build holes of their own.

There is no course file. The player's design is drawn by the game itself (AGGCourse draws a Workshop hole's ground and
walls); this script makes the place it is built in, and the materials those ground and walls wear.

Run inside the editor:  Tools/regen.sh workshop --no-materials
"""

import importlib
import math
import random
import sys

import unreal

sys.path.insert(0, "/Users/ken/Code/gravitygolf/Tools/levels")
import common as c  # noqa: E402

importlib.reload(c)

LEVEL = "/Game/Levels/Workshop"
SURFACE = "/Game/Materials/M_GG_Surface.M_GG_Surface"
GLOW = "/Game/Materials/M_GG_Glow.M_GG_Glow"
rng = random.Random(77)


def materials():
    """What a Workshop hole is made of: an architect's model standing on a drafting table. White card for the ground
    (ruled in the squares it was built on) and for the walls, dark tops so a wall reads from above, green felt for
    the soft walls. The game looks these up by name: MI_Workshop_<look>."""
    grid = "/Game/Materials/M_GG_Grid.M_GG_Grid"
    salt = "/Game/Materials/M_GG_Salt.M_GG_Salt"
    return {
        "floor": c.material_instance(grid, "MI_Workshop_floor", {"Ink": 0.3, "Strength": 0.0, "Roughness": 0.62},
                                     {"Tint": (0.74, 0.75, 0.76, 1)}),
        "sand": c.material_instance(salt, "MI_Workshop_sand", {"CellSize": 22.0, "RimWidth": 0.12}, {"Tint": (0.8, 0.6, 0.34, 1)}),
        "pad": c.material_instance(SURFACE, "MI_Workshop_pad", {"Roughness": 1.0}, {"BaseColor": (0.05, 0.16, 0.1, 1)}),
        "wall": c.material_instance(SURFACE, "MI_Workshop_wall", {"Roughness": 0.55}, {"BaseColor": (0.8, 0.8, 0.82, 1)}),
        "soft": c.material_instance(SURFACE, "MI_Workshop_soft", {"Roughness": 1.0}, {"BaseColor": (0.1, 0.42, 0.16, 1)}),
        "cap": c.material_instance(SURFACE, "MI_Workshop_cap", {"Roughness": 0.4}, {"BaseColor": (0.1, 0.14, 0.2, 1)}),
        "table": c.material_instance(grid, "MI_Workshop_Table"),
        "line": c.material_instance(GLOW, "MI_Workshop_Line", {"Strength": 5.0}, {"Color": (0.25, 0.6, 1.0, 1)}),
        "ember": c.material_instance(GLOW, "MI_Workshop_Ember", {"Strength": 7.0}, {"Color": (1.0, 0.55, 0.2, 1)}),
    }


def frame(center, size, rotation, material, label):
    """The outline of a block: twelve bars of light. A thing that has been drawn but not yet built."""
    sx, sy, sz = size
    bar = max(sx, sy, sz) * 0.012
    # The engine's rotation (pitch, yaw, roll) as three axes, to turn each bar's offset the way the bar itself is turned.
    sp, sy_, sr = (math.sin(math.radians(a)) for a in rotation)
    cp, cy, cr = (math.cos(math.radians(a)) for a in rotation)
    axes = ((cp * cy, cp * sy_, sp), (sr * sp * cy - cr * sy_, sr * sp * sy_ + cr * cy, -sr * cp),
            (-(cr * sp * cy + sr * sy_), cy * sr - cr * sp * sy_, cr * cp))
    for axis in range(3):
        for a in (-1, 1):
            for b in (-1, 1):
                if axis == 0:
                    offset, dims = (0, a * sy / 2, b * sz / 2), (sx, bar, bar)
                elif axis == 1:
                    offset, dims = (a * sx / 2, 0, b * sz / 2), (bar, sy, bar)
                else:
                    offset, dims = (a * sx / 2, b * sy / 2, 0), (bar, bar, sz)
                turned = [sum(axes[k][n] * offset[k] for k in range(3)) for n in range(3)]
                c.box((center[0] + turned[0], center[1] + turned[1], center[2] + turned[2]), dims, material, label,
                      folder="Fantastical/Drafts", rotation=rotation, solid=False)


def drafts(m):
    """The fantastical: the sky is full of holes not built yet. Outlines of blocks, some vast, hang over the table,
    and a few have come down to stand on it like a skyline."""
    for i in range(34):
        angle = rng.uniform(-math.pi, math.pi)
        distance = rng.uniform(9000, 60000)
        height = rng.uniform(0.12, 0.5) * distance
        size = rng.uniform(0.05, 0.16) * distance
        frame((1400 + math.cos(angle) * distance, math.sin(angle) * distance, height),
              (size * rng.uniform(0.5, 1.6), size * rng.uniform(0.5, 1.6), size * rng.uniform(0.3, 1.2)),
              (rng.uniform(-25, 25), rng.uniform(0, 360), rng.uniform(-25, 25)), m["ember" if i % 6 == 0 else "line"], f"Draft{i}")
    for i in range(16):
        angle = rng.uniform(-math.pi, math.pi)
        distance = rng.uniform(14000, 42000)
        size = rng.uniform(1500, 5200)
        tall = size * rng.uniform(0.6, 3.2)
        frame((1400 + math.cos(angle) * distance, math.sin(angle) * distance, tall / 2 - 30), (size, size * rng.uniform(0.6, 1.4), tall),
              (0, rng.uniform(0, 90), 0), m["line"], f"Tower{i}")


c.fresh_level(LEVEL)
mats = materials()
# The hour after sunset: a cool, even light that shows a design plainly, and lets the table's lines and the machines glow.
c.sky_and_sun(sun_pitch=-58.0, sun_yaw=205.0, sun_lux=3.2, fog_density=0.004, sun_kelvin=7600.0, sky_intensity=1.6, exposure_bias=0.3)
# The table: its top lies 30 cm under the ball's ground, so a design's ground stands on it as a slab.
# It is laid in slabs: one slab tens of kilometres wide loses depth precision and draws over the hole standing on it.
TILE = 40000
for i in range(-3, 4):
    for j in range(-3, 4):
        c.box((i * TILE, j * TILE, -30 - 100), (TILE, TILE, 200), mats["table"], "Table", folder="Landscape/Table")
EDGE = TILE * 3.5
FAR = 4000000
for sx, sy, w, h in ((1, 0, FAR, FAR * 2 + EDGE * 2), (-1, 0, FAR, FAR * 2 + EDGE * 2), (0, 1, EDGE * 2, FAR), (0, -1, EDGE * 2, FAR)):
    c.box((sx * (EDGE + FAR / 2), sy * (EDGE + FAR / 2), -32 - 100), (w, h, 200), mats["table"], "TableFar", folder="Landscape/Table")
c.player_start({"tee": {"position": [325, 25, 6], "yaw": 0}})
drafts(mats)
c.write_plants(LEVEL, web_ambient=2.2, web_tint=(0.8, 0.9, 1.0))
c.LES.save_current_level()
