"""Hole 14, Hand Over Hand: the Orrery. A low-walled court on a great dark dial that floats among the stars. A lamp
hangs over the middle of it, and round the lamp go the orbits: hoops of light hundreds of metres across, each with
its planet, some passing overhead and some dipping below the dial's edge.

Collision geometry comes from Content/Courses/handover.json (written by Tools/courses/handover.py). The walls are
low on purpose: this hole is a plan made before the first carry, so everything in it can be seen from the tee.

Run inside the editor:  Tools/regen.sh handover --no-materials
(The hoop, band and disc meshes come from Tools/build_meshes.py, the planets' material from Tools/build_materials.py.)
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

LEVEL = "/Game/Levels/HandOver"
COURSE = "/Users/ken/Code/gravitygolf/Content/Courses/handover.json"
PREFIX = "Orrery"
SURFACE = "/Game/Materials/M_GG_Surface.M_GG_Surface"
GLOW = "/Game/Materials/M_GG_Glow.M_GG_Glow"
PLANET = "/Game/Materials/M_GG_Planet.M_GG_Planet"
HOOP = "/Game/Meshes/SM_Hoop.SM_Hoop"
FINE = "/Game/Meshes/SM_HoopFine.SM_HoopFine"
BAND = "/Game/Meshes/SM_Band.SM_Band"
DISC = "/Game/Meshes/SM_Disc.SM_Disc"
GOLD = (1.0, 0.7, 0.3)
rng = random.Random(1414)


# The planets are lit by a sun a long way off to the south and a little above the dial, not by the lamp: seen from
# the court, where the lamp is, a planet lit by the lamp would always be full, and a full planet is a flat disc.
# (South is on the left going down the hole: the giant ahead is lit from the left, about half of it.)
SUN_WAY = (-0.1, -0.95, 0.3)
SUN = tuple(2200.0 * (i == 0) + 5.0e6 * SUN_WAY[i] for i in range(3))


def materials():
    def planet(name, a, b, air, bands, turbulence, seed, strength=1.0):
        return c.material_instance(PLANET, f"MI_{PREFIX}_{name}", {"Bands": bands, "Turbulence": turbulence, "Seed": seed, "Strength": strength},
                                   {"ColorA": (*a, 1), "ColorB": (*b, 1), "Air": (*air, 1), "SunPos": (*SUN, 1)})

    return {
        # Dark polished stone: it has to be dark for the lines of light to read, and polished to give them back.
        "paving": c.scan_material(f"MI_{PREFIX}_Floor", "Beige_Marble_Tiles", 300, (0.1, 0.17, 0.36, 1), roughness=0.3),
        "dial": c.scan_material(f"MI_{PREFIX}_Dial", "Beige_Marble_Tiles", 900, (0.05, 0.065, 0.11, 1), roughness=0.5),
        "wall": c.scan_material(f"MI_{PREFIX}_Wall", "Roman_Limestone_Wall", 320, (0.3, 0.36, 0.5, 1)),
        "brass": c.material_instance(SURFACE, f"MI_{PREFIX}_Brass", {"Roughness": 0.3, "Metallic": 1.0}, {"BaseColor": (0.78, 0.55, 0.22, 1)}),
        "glyph": c.material_instance(GLOW, f"MI_{PREFIX}_Glyph", {"Strength": 9.0}, {"Color": (*GOLD, 1)}),
        "inlay": c.material_instance(GLOW, f"MI_{PREFIX}_Inlay", {"Strength": 2.4}, {"Color": (*GOLD, 1)}),
        "orbit": c.material_instance(GLOW, f"MI_{PREFIX}_Orbit", {"Strength": 7.0}, {"Color": (*GOLD, 1)}),
        "farOrbit": c.material_instance(GLOW, f"MI_{PREFIX}_FarOrbit", {"Strength": 10.0}, {"Color": (1.0, 0.76, 0.42, 1)}),
        "lamp": c.material_instance(GLOW, f"MI_{PREFIX}_Lamp", {"Strength": 60.0}, {"Color": (1.0, 0.84, 0.58, 1)}),
        "bronze": c.material_instance(SURFACE, f"MI_{PREFIX}_Bronze", {"Roughness": 0.4, "Metallic": 1.0}, {"BaseColor": (0.25, 0.15, 0.06, 1)}),
        "black": c.material_instance(SURFACE, "MI_Cup", {"Roughness": 1.0}, {"BaseColor": (0.004, 0.004, 0.004, 1)}),
        "giant": planet("Giant", (0.95, 0.58, 0.24), (0.5, 0.22, 0.1), (1.0, 0.7, 0.45), 17.0, 0.22, 3.0, 2.0),
        "blue": planet("Blue", (0.08, 0.2, 0.75), (0.25, 0.5, 0.9), (0.35, 0.6, 1.0), 6.0, 0.6, 11.0, 1.6),
        "red": planet("Red", (0.7, 0.2, 0.09), (0.32, 0.09, 0.06), (1.0, 0.5, 0.35), 3.0, 1.4, 23.0, 1.5),
        "pale": planet("Pale", (0.55, 0.58, 0.6), (0.3, 0.32, 0.36), (0.7, 0.8, 1.0), 2.0, 2.0, 31.0, 1.4),
        "green": planet("Green", (0.1, 0.5, 0.4), (0.04, 0.22, 0.26), (0.4, 1.0, 0.8), 9.0, 0.4, 41.0, 1.6),
    }


def dressed_wall(box, m):
    """A low wall of blue-grey stone with a brass cap and a seam of light along each long side."""
    cx, cy, cz = box["center"]
    sx, sy, sz = box["size"]
    base = cz - sz / 2
    c.box((cx, cy, cz), (sx, sy, sz), m["wall"], box["name"])
    c.box((cx, cy, base + sz + 5), (sx + 14, sy + 14, 10), m["brass"], box["name"] + "_Cap", folder="Orrery/Walls")
    along_x = sx >= sy
    for side in (-1, 1):
        if along_x:
            c.box((cx, cy + side * (sy / 2 + 1), base + sz * 0.62), (sx - 40, 2, 3), m["glyph"], box["name"] + "_Seam",
                  folder="Orrery/Walls", solid=False)
        else:
            c.box((cx + side * (sx / 2 + 1), cy, base + sz * 0.62), (2, sy - 40, 3), m["glyph"], box["name"] + "_Seam",
                  folder="Orrery/Walls", solid=False)


def course_geometry(course, m):
    for b in course["boxes"]:
        if b["look"] == "wall":
            dressed_wall(b, m)
        else:
            c.box(b["center"], b["size"], m[b["look"]], b["name"])


def markers(course, m):
    tee = course["tee"]["position"]
    c.mesh(c.CYLINDER, (tee[0], tee[1], 0.5), (200, 200, 1), material=m["brass"], label="TeeStone", folder="Course/Markers",
           cast_shadow=False)
    cup = course["cup"]
    x, y, z = cup["position"]
    c.mesh(c.CYLINDER, (x, y, z + 0.4), (cup["radius"] * 2 + 16, cup["radius"] * 2 + 16, 0.8), material=m["glyph"],
           label="CupRim", folder="Course/Markers", cast_shadow=False)
    c.mesh(c.CYLINDER, (x, y, z + 0.9), (cup["radius"] * 2, cup["radius"] * 2, 1), material=m["black"], label="Cup",
           folder="Course/Markers", cast_shadow=False)
    c.mesh(c.CYLINDER, (x, y, z + 130), (2.5, 2.5, 260), material=m["brass"], label="FlagPole", folder="Course/Markers")
    for plinth in course.get("plinths", []):
        c.dais((plinth["position"][0], plinth["position"][1]), plinth["radius"] * 2 + 30, m["bronze"], m["glyph"], label="Plinth")


def hoop(center, radius, tilt, material, label, thick=1.0, asset=FINE):
    """An orbit: a hoop of light of this radius about a point, turned by a rotator or (pitch, yaw, roll)."""
    return c.mesh(asset, center, (radius, radius, radius * thick), rotation=tilt, material=material, label=label,
                  folder="Orrery/Orbits", cast_shadow=False, solid=False)


def moving(actor, spin=None):
    """Mark a piece of the sky as one that moves. It leaves the lighting scene (what moves every frame would be
    captured again every frame), and with a spin ("Z", degrees a second) the game turns it: see AGGCourse::FindSpinners."""
    component = actor.static_mesh_component
    component.set_mobility(unreal.ComponentMobility.MOVABLE)
    for flag in ("affect_dynamic_indirect_lighting", "affect_distance_field_lighting", "visible_in_ray_tracing"):
        component.set_editor_property(flag, False)
    if spin:
        actor.tags = [f"GGSpin:{spin[0]}:{spin[1]}"]
    return actor


def ride(actor, carrier):
    """Fix one moving piece to another, where it stands now: a planet to the hoop of its orbit."""
    keep = unreal.AttachmentRule.KEEP_WORLD
    moving(actor).attach_to_actor(carrier, "", keep, keep, keep, False)
    return actor


def toward(azimuth, elevation):
    a, e = math.radians(azimuth), math.radians(elevation)
    return unreal.Vector(math.cos(e) * math.cos(a), math.cos(e) * math.sin(a), math.sin(e))


def orbit_through(azimuth, elevation, lean):
    """The turn of a hoop about the lamp that passes through the sky at this bearing and height (degrees; bearing 0
    is down the hole, 90 to its right). Lean tips the hoop about the line to that point, so no two lie alike."""
    out = toward(azimuth, elevation)
    level = unreal.Vector(-math.sin(math.radians(azimuth)), math.cos(math.radians(azimuth)), 0.0)
    up = out.cross(level)
    side = level * math.cos(math.radians(lean)) + up * math.sin(math.radians(lean))
    turn = unreal.MathLibrary.make_rot_from_xy(out, side)
    return (turn.pitch, turn.yaw, turn.roll)


def orrery(course, m, lamp):
    """The fantastical: the machine the court is part of."""
    middle = (lamp[0], lamp[1])
    # The dial: a disc of dark stone 130 m across the radius, its top a hair under the court's floor.
    RIM = 13000
    c.mesh(DISC, (middle[0], middle[1], -1.0), (RIM, RIM, 12000), material=m["dial"], label="Dial", folder="Orrery/Dial")
    c.mesh(HOOP, (middle[0], middle[1], 6.0), (RIM, RIM, RIM * 6.0), material=m["brass"], label="DialRim", folder="Orrery/Dial",
           cast_shadow=False, solid=False)
    # Lines inlaid in it: circles about the lamp's foot, which run on across the court's own floor, and a ring of
    # marks near the rim, every five degrees, longer at the thirties.
    for radius in (900, 1700, 2600, 3900, 5600, 7800, 10400):
        c.mesh(BAND, (middle[0], middle[1], 0.3), (radius, radius, 100), material=m["inlay"], label="DialCircle",
               folder="Orrery/Dial", cast_shadow=False, solid=False)
    for step in range(72):
        angle = math.radians(step * 5.0)
        length = 900 if step % 6 == 0 else 380
        r = 11900 - length / 2
        c.box((middle[0] + math.cos(angle) * r, middle[1] + math.sin(angle) * r, 0.3), (length, 14 if step % 6 == 0 else 8, 0.6),
              m["inlay"], "DialMark", folder="Orrery/Dial", rotation=(0, step * 5.0, 0), solid=False)

    # The lamp, and the light it gives: warm, from straight overhead, so every wall stands in a small pool of shadow.
    c.mesh(c.SPHERE, lamp, (420, 420, 420), material=m["lamp"], label="Lamp", folder="Orrery/Lamp", cast_shadow=False, solid=False)
    light = c.point_light(lamp, (1.0, 0.8, 0.55), 9500.0, 14000.0, "LampLight", folder="Lighting")
    component = light.get_component_by_class(unreal.PointLightComponent)
    component.set_editor_property("source_radius", 200.0)
    component.set_editor_property("cast_shadows", True)
    # Three thin hoops close about the lamp, as an armillary: they are what says "instrument" when you look up.
    # (They turn, each about a line across itself, slowly: the instrument is running.)
    for radius, tilt, rate in ((900, (0, 0, 0), 2.2), (1050, (0, 20, 62), -1.4), (1200, (38, 110, -20), 0.9)):
        moving(hoop(lamp, radius, tilt, m["brass"], "Armillary", thick=4.0, asset=HOOP), spin=("X", rate))

    # The orbits, near to far: radius, where in the sky its planet stands (bearing, height, and the hoop's lean), the
    # planet's look and its radius. Each planet is seen above the low walls from everywhere in the court.
    # The last number is how fast it goes round, in degrees a second: the nearest (once round in 24 minutes) can be
    # seen to move, the giant takes eight hours and in a quarter of an hour has barely left its place over the line
    # to the cup.
    orbits = [
        (9000, (100, 22, 50), "pale", 420, 0.25),
        (21000, (-75, 16, -15), "red", 1300, 0.12),
        (38000, (150, 30, 35), "green", 2100, 0.06),
        (56000, (195, 20, -20), "blue", 5200, 0.03),
        (88000, (17, 7.5, 12), "giant", 9000, 0.012),
    ]
    for index, (radius, (azimuth, elevation, lean), look, size, rate) in enumerate(orbits):
        turn = orbit_through(azimuth, elevation, lean)
        orbit = moving(hoop(lamp, radius, turn, m["farOrbit" if radius > 30000 else "orbit"], f"Orbit{index + 1}"), spin=("Z", rate))
        out = toward(azimuth, elevation)
        spot = (lamp[0] + out.x * radius, lamp[1] + out.y * radius, lamp[2] + out.z * radius)
        ride(c.mesh(c.SPHERE, spot, (size * 2, size * 2, size * 2), rotation=(rng.uniform(-25, 25), rng.uniform(0, 360), rng.uniform(-25, 25)),
                    material=m[look], label=f"Planet_{look}", folder="Orrery/Planets", cast_shadow=False, solid=False), orbit)
        if look == "giant":
            # Its own rings: fine hoops close about it, tipped toward the court.
            for ring in (1.4, 1.5, 1.62, 1.7, 1.86, 1.95, 2.1):
                ride(c.mesh(FINE, spot, (size * ring, size * ring, size * ring * 2.0), rotation=(24, 35, 10), material=m["farOrbit"],
                            label="GiantRing", folder="Orrery/Planets", cast_shadow=False, solid=False), orbit)


c.fresh_level(LEVEL)
with open(COURSE) as f:
    course_data = json.load(f)
# The lamp hangs over the middle of the court.
xs = [b["center"][0] for b in course_data["boxes"] if b["look"] == "paving"]
LAMP = (xs[0], 0.0, 3400.0)
mats = materials()
# A clear night with no moon of its own: the light here is the lamp's. A little cool light from behind the tee fills
# the shadows; thin fog, or the far planets drown in it.
# (That cool light comes from where the planets' sun is.)
c.night_sky(moon_pitch=-17.5, moon_yaw=84.0, moon_lux=0.22, fog_density=0.0015, exposure_bias=-0.2, sky_intensity=1.6)
course_geometry(course_data, mats)
markers(course_data, mats)
c.player_start(course_data)
orrery(course_data, mats, LAMP)
c.write_plants(LEVEL, web_ambient=2.4, web_tint=(1.0, 0.9, 0.75))
c.LES.save_current_level()
