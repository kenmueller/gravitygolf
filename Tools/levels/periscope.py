"""Hole 10, Periscope: the salt a third time, at night. A moon far too large stands low over the brine and lies in
it again, an aurora hangs over the far end, a black wall crosses the crust with one slit in it, barred with light.

It is the Salt Mirror's level script (Tools/levels/saltmirror.py) with its own course, sky and colours.

Run inside the editor:  Tools/regen.sh periscope --no-materials
"""

import math
import runpy

# The moon hangs ahead and to the left, low enough that the brine at the crust's edge holds its reflection. It is drawn by the sky itself, so the brine mirrors it.
AZIMUTH, HEIGHT = math.radians(-27.0), math.radians(7.0)
MOON = (math.cos(HEIGHT) * math.cos(AZIMUTH), math.cos(HEIGHT) * math.sin(AZIMUTH), math.sin(HEIGHT), 1.0)

runpy.run_path("/Users/ken/Code/gravitygolf/Tools/levels/saltmirror.py", run_name="__main__", init_globals={"GG_VARIANT": {
    "level": "/Game/Levels/Periscope",
    "course": "periscope",
    "prefix": "Periscope",
    "seed": 47,
    "glyph": (0.3, 1.0, 0.8),
    # Moonlight from the moon's side of the sky but from 42 degrees up: the long wall's shadow is a band five metres
    # wide on the near side of it, and the square of plinths lies in the light.
    "night": dict(moon_pitch=-42.0, moon_yaw=156.0, moon_lux=1.1, fog_density=0.004, exposure_bias=-0.5,
                  reflection_size=512),
    "sky": ({"MoonSize": 0.07, "AuroraStrength": 7.0},
            {"MoonDir": MOON, "MoonColor": (0.74, 0.83, 0.97, 1), "AuroraLow": (0.08, 1.0, 0.5, 1),
             "AuroraHigh": (0.5, 0.18, 1.0, 1)}),
    "monolith_height": 700,
    "edge_lights": True,
    "web_ambient": 2.6,
    "web_tint": (0.75, 0.85, 1.0),
}})
