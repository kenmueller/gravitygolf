"""Hole 12, After You: the hall of columns a third time, in the evening. A low amber sun from the side, long shadows
across the floor, sea-green light in the plinths.

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
    # Evening: a low sun from the south side of the hall, a little behind the tee. Its light runs along the cross wall
    # (whose shadow is then only a strip at its foot) and rakes the floor, and the gates are the brightest things in
    # the picture. (At noon, the first try, the hall was a beige floor under a plain sky and the gates washed out.)
    "sun": dict(sun_pitch=-15.0, sun_yaw=68.0, fog_density=0.022, sun_kelvin=3500.0, sky_intensity=1.5, exposure_bias=0.25),
    "web_ambient": 2.0,
    "web_tint": (0.95, 0.97, 1.0),
}})
