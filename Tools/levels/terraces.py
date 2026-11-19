"""Hole 1, The Terraces: limestone terraces stepping down to a sea at sunrise.

Course: tee on the top terrace, green on the lowest by a ruined colonnade at the cliff edge. A broken wall on the
middle terrace blocks the straight line; the first attractor bends the shot around it.

Run inside the editor (MCP execute_python):  runpy.run_path('<repo>/Tools/levels/terraces.py')
"""

import importlib
import sys

import unreal

sys.path.insert(0, "/Users/ken/Code/gravitygolf/Tools/levels")
import common as c  # noqa: E402

importlib.reload(c)  # the editor's interpreter persists between runs

LEVEL = "/Game/Levels/Terraces"

# Units: centimeters. +X runs from the tee toward the sea, +Y to the right, +Z up.
TERRACE_LENGTH = 3000
TERRACE_WIDTH = 2600
DROP = 320
SEA_LEVEL = -3200


def materials():
    surface = "/Game/Materials/M_GG_Surface.M_GG_Surface"
    return {
        "grass": c.material_instance(surface, "MI_Terraces_Grass", {"Roughness": 0.9},
                                     {"BaseColor": (0.09, 0.2, 0.045, 1)}),
        "green": c.material_instance(surface, "MI_Terraces_Green", {"Roughness": 0.85},
                                     {"BaseColor": (0.07, 0.24, 0.05, 1)}),
        "stone": c.material_instance(surface, "MI_Terraces_Limestone", {"Roughness": 0.8},
                                     {"BaseColor": (0.62, 0.56, 0.46, 1)}),
        "rock": c.material_instance(surface, "MI_Terraces_Rock", {"Roughness": 0.9},
                                    {"BaseColor": (0.42, 0.37, 0.31, 1)}),
        "ocean": unreal.load_asset("/Game/Materials/M_GG_Ocean.M_GG_Ocean"),
    }


def terraces(m):
    for index in range(3):
        top = -DROP * index
        x = TERRACE_LENGTH * index + TERRACE_LENGTH / 2
        lawn = m["green"] if index == 2 else m["grass"]
        # Lawn slab, the limestone body under it, and a low wall along the far edge.
        c.box((x, 0, top - 15), (TERRACE_LENGTH, TERRACE_WIDTH, 30), lawn, f"Terrace{index + 1}_Lawn")
        c.box((x, 0, top - 30 - 1500), (TERRACE_LENGTH, TERRACE_WIDTH + 200, 3000), m["stone"],
              f"Terrace{index + 1}_Body")
        for side in (-1, 1):
            c.box((x, side * (TERRACE_WIDTH / 2 + 60), top + 40), (TERRACE_LENGTH, 120, 140), m["stone"],
                  f"Terrace{index + 1}_Parapet{'L' if side < 0 else 'R'}")
        # Steps down to the next terrace on the left side.
        if index < 2:
            for step in range(8):
                c.box((TERRACE_LENGTH * (index + 1) + step * 45, -TERRACE_WIDTH / 2 + 250, top - step * 40 - 20),
                      (45, 300, 40), m["stone"], f"Terrace{index + 1}_Step{step}")

    # The broken wall on the middle terrace: solid on the left, a gap on the right.
    c.box((4500, -350, -DROP + 200), (160, 1900, 400), m["stone"], "Ruin_Wall")
    c.box((4500, 900, -DROP + 70), (160, 300, 140), m["stone"], "Ruin_Wall_Stub")

    # Colonnade along the cliff edge past the green; a few columns have fallen.
    for i, y in enumerate(range(-1100, 1101, 440)):
        height = 650 if i % 3 != 1 else 260
        c.mesh(c.CYLINDER, (8850, y, -2 * DROP + height / 2), (110, 110, height), material=m["stone"],
               label=f"Column{i}", folder="Ruins")
    c.box((8850, 0, -2 * DROP + 680), (140, 2400, 60), m["stone"], "Architrave", folder="Ruins")
    c.mesh(c.CYLINDER, (7600, -700, -2 * DROP + 55), (110, 110, 520), rotation=(90, 20, 0), material=m["stone"],
           label="FallenColumn", folder="Ruins")


def surroundings(m):
    # The cliff falls away to the sea; distant headlands frame the horizon.
    c.box((4500, 0, (SEA_LEVEL - 600) / 2 - 400), (12000, 9000, -SEA_LEVEL - 200), m["rock"], "Cliff_Mass",
          folder="Landscape")
    for i, (x, y, size) in enumerate([(42000, -26000, 9000), (60000, 18000, 14000), (30000, 34000, 6000)]):
        c.mesh(c.SPHERE, (x, y, SEA_LEVEL - size * 0.2), (size * 2.2, size * 1.6, size), material=m["rock"],
               label=f"Headland{i}", folder="Landscape")
    c.mesh(c.PLANE, (0, 0, SEA_LEVEL), (400000, 400000, 1), material=m["ocean"], label="Sea", folder="Landscape",
           cast_shadow=False)


def course_markers(m):
    # Tee box and the cup with its flag (visual only until the course actor exists).
    c.box((600, 0, 1), (260, 260, 4), m["stone"], "TeeBox")
    cup = (7600, 500, -2 * DROP)
    c.mesh(c.CYLINDER, (cup[0], cup[1], cup[2] - 4), (22, 22, 10), material=m["rock"], label="Cup")
    c.mesh(c.CYLINDER, (cup[0], cup[1], cup[2] + 120), (4, 4, 240), material=m["stone"], label="FlagPole")


c.fresh_level(LEVEL)
mats = materials()
c.sky_and_sun(sun_pitch=-9.0, sun_yaw=150.0)
terraces(mats)
surroundings(mats)
course_markers(mats)
c.LES.save_current_level()
"built " + LEVEL
