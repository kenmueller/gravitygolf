"""Hole 12, After You: the hall of columns a third time, at noon. A high white sun, short shadows, sea-green light in
the gates and plinths.

It is the Colonnade's level script (Tools/levels/colonnade.py) with its own course, sun and colours.

Run inside the editor:  Tools/regen.sh afteryou --no-materials
"""

import runpy

runpy.run_path("/Users/ken/Code/gravitygolf/Tools/levels/colonnade.py", run_name="__main__", init_globals={"GG_VARIANT": {
    "level": "/Game/Levels/AfterYou",
    "course": "afteryou",
    "prefix": "AfterYou",
    "seed": 131,
    "glyph": (0.4, 0.95, 0.85),
    # A high sun whose light runs along the cross wall rather than across it: its shadow is a strip at its foot.
    "sun": dict(sun_pitch=-62.0, sun_yaw=255.0, fog_density=0.012, sun_kelvin=6100.0, sky_intensity=1.2, exposure_bias=-0.1),
    "web_ambient": 2.0,
    "web_tint": (0.95, 0.97, 1.0),
}})
