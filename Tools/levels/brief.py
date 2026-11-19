"""Hole 6, Second Wind: the lantern court again, later in the night. The lanterns have turned pale and stream across
the sky toward the cup, like something carried on a wind.

Collision geometry comes from Content/Courses/brief.json (written by Tools/courses/brief.py). The dressing is
Tools/levels/lantern.py, run with this hole's settings.

Run inside the editor:  Tools/regen.sh brief
"""

import runpy

runpy.run_path("/Users/ken/Code/gravitygolf/Tools/levels/lantern.py", run_name="__main__", init_globals={"GG_VARIANT": {
    "level": "/Game/Levels/Brief",
    "course": "brief",
    "prefix": "Brief",
    "seed": 77,
    # Pale jade lanterns instead of amber, and a higher, colder moon from the other side.
    "lantern_color": (0.55, 1.0, 0.78),
    "far_color": (0.5, 0.95, 0.85),
    "moon": dict(moon_pitch=-40.0, moon_yaw=150.0, moon_lux=0.45),
    "river": True,
    # Lit lanterns over the places that matter: the gap, the corner, and down the gallery.
    "lit": [(-2350, 200, 330), (-1500, 650, 340), (-300, 650, 330), (900, 650, 330), (2100, 650, 330),
            (-1200, -450, 320), (300, -450, 320), (1800, -450, 320), (3300, -450, 320)],
}})
