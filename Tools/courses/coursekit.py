"""Shared builder for course definitions.

A course script describes a hole with boxes and markers, then calls write(). That produces:
  Content/Courses/<id>.json     read by the game (AGGCourse) and by the level script
  Saved/SimTest/<id>.course     plain numbers for the standalone solver (Tests/Sim/solve.sh)
"""

import json
import os

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", ".."))
SURFACES = {"grass": 0, "green": 1, "stone": 2, "sand": 3, "ice": 4}


class Course:
    def __init__(self, course_id, name, level, next_level=None):
        self.id = course_id
        self.name = name
        self.level = level
        self.next = next_level
        self.boxes = []
        self.machines = []
        self.no_machine = []
        self.safe_zones = []
        self.tee = None
        self.cup = None
        self.ball_radius = 6
        self.kill_z = -1400
        self.placement = None
        self.extra = {}
        self.solutions = []
        self.switches = []
        self.gates = []
        self.plinths = []

    def box(self, name, center, size, surface="stone", look="stone", shape="box"):
        """An axis-aligned collision box. `look` tells the level script how to dress it."""
        self.boxes.append({"name": name, "center": list(center), "size": list(size), "surface": surface, "look": look,
                           "shape": shape})

    def set_tee(self, x, y, z=0, yaw=0):
        self.tee = {"position": [x, y, z + self.ball_radius], "yaw": yaw}

    def set_cup(self, x, y, z=0, radius=42, capture_speed=650, court=520):
        """The cup, and the paved court around it where machines can't stand."""
        self.cup = {"position": [x, y, z], "radius": radius, "captureSpeed": capture_speed}
        if court:
            self.no_machine.append({"position": [x, y, z], "radius": court, "kind": "court"})

    def safe_zone(self, x, y, z=0, radius=240, pad=True, needs_switch=-1):
        """A checkpoint: a ball that rests here stays, and any switch it pressed on the way stays pressed. With a
        pad, the level draws a patch there. `needs_switch` tells the solver which switch the leg is for."""
        self.safe_zones.append({"position": [x, y, z], "radius": radius, "needsSwitch": needs_switch})
        self.no_machine.append({"position": [x, y, z], "radius": radius + 60, "kind": "pad"})
        if pad:
            # The pad's own box is for the level's look only. It lies in the floor, and where two boxes share the
            # ground under the ball the simulation seats the ball on the first one listed and the second never
            # touches it: the floor comes first, so a pad does not slow the ball. (It must not: the next swing starts
            # from the middle of the pad, and sand would hold the ball there.) To stop a ball at a pad, put a
            # soft_patch just beyond it as a backstop.
            side = radius * 1.7
            self.box(f"SafePad{len(self.safe_zones)}", (x, y, z - 10), (side, side, 20), "sand", "pad")

    def soft_patch(self, name, center, size, look="sand"):
        """Soft ground lying in the floor: a ball that rolls onto it stops within a pace. It is listed before every
        other box so that it, not the floor under it, is what the ball touches."""
        self.boxes.insert(0, {"name": name, "center": list(center), "size": list(size), "surface": "sand", "look": look,
                              "shape": "box"})

    def switch(self, x, y, z=0, radius=70, weight=False):
        """A plate in the ground. A light plate is pressed by the ball rolling over it, and machines can't stand on
        it. A weight plate ignores the ball and is down only while a machine stands on it. Returns its index for
        gates."""
        self.switches.append({"position": [x, y, z], "radius": radius, "weight": weight})
        if not weight:
            self.no_machine.append({"position": [x, y, z], "radius": radius + 40, "kind": "switch"})
        return len(self.switches) - 1

    def gate(self, name, center, size, switch, open_seconds=0.0, surface="sand"):
        """A barrier that stops the ball until its switch is pressed. With open_seconds it closes again."""
        self.gates.append({"name": name, "center": list(center), "size": list(size), "switch": switch,
                           "openSeconds": open_seconds, "surface": surface})

    def machine(self, x, y, z=0, strength=420, reach=1900, core_height=45, movable=True):
        """A gravity machine at its starting spot. Positive strength attracts, negative repels."""
        self.machines.append({"position": [x, y, z], "strength": strength, "range": reach, "coreHeight": core_height,
                              "movable": movable})

    def plinth(self, x, y, z=0, radius=85):
        """A plinth. A hole that has any lets machines stand only on them (the game snaps a machine to the middle)."""
        self.plinths.append({"position": [x, y, z], "radius": radius})

    def mirror(self, x, y, z=0, turn=0, core_height=45, movable=True):
        """A mirror at its starting spot: an upright slab at 45 degrees that the ball bounces off like light. Turn 0
        lies along +X+Y ("/" with +Y up, sends an eastbound ball north), turn 1 along +X-Y. It is carried and set
        down like any machine, and can be turned."""
        self.machines.append({"position": [x, y, z], "strength": 0, "range": 0, "coreHeight": core_height,
                              "movable": movable, "kind": "mirror", "turn": turn})

    def solution(self, leg, machines, yaw, power, turns=None):
        """The intended answer for a leg: where each machine stands, and one swing that works. The solver verifies
        it and measures how forgiving it is. Required for holes with two or more machines. `turns` says which way
        each machine is turned (it only matters for mirrors)."""
        self.solutions.append({"leg": leg, "machines": [list(m) for m in machines], "yaw": yaw, "power": power,
                               "turns": list(turns) if turns else [0] * len(machines)})

    def placement_area(self, min_x, min_y, max_x, max_y):
        """Where the solver may try standing machines (the walkable ground)."""
        self.placement = [min_x, min_y, max_x, max_y]

    def write(self):
        data = {
            "id": self.id, "name": self.name, "level": self.level, "par": len(self.safe_zones) + 1,
            "ballRadius": self.ball_radius, "gravity": [0, 0, -980], "killZ": self.kill_z,
            "tee": self.tee, "cup": self.cup, "noMachine": self.no_machine, "safeZones": self.safe_zones,
            "attractors": self.machines, "boxes": self.boxes,
            "switches": self.switches, "gates": self.gates, "solutions": self.solutions,
        }
        if self.plinths:
            data["plinths"] = self.plinths
        if self.next:
            data["next"] = self.next
        data.update(self.extra)

        out = os.path.join(ROOT, "Content", "Courses", f"{self.id}.json")
        with open(out, "w") as f:
            json.dump(data, f, indent="\t")
            f.write("\n")

        write_flat(data, self.placement, os.path.join(ROOT, "Saved", "SimTest", f"{self.id}.course"))
        print(f"wrote {out} ({len(self.boxes)} boxes, {len(self.machines)} machines, {len(self.safe_zones)} safe zones)")


def write_flat(data, placement, path):
    """The course as plain numbers for the standalone solver, search and probe (Tests/Sim). `data` is the course as
    written to JSON; `placement` is where machines may be tried (min x, min y, max x, max y)."""
    os.makedirs(os.path.dirname(path), exist_ok=True)
    machines = data["attractors"]
    with open(path, "w") as f:
        tee, cup = data["tee"]["position"], data["cup"]
        f.write(f"{tee[0]} {tee[1]} {tee[2]}\n")
        f.write(f"{cup['position'][0]} {cup['position'][1]} {cup['position'][2]} {cup['radius']} {cup['captureSpeed']}\n")
        f.write(f"{data['ballRadius']} {data['killZ']}\n")
        f.write(" ".join(str(v) for v in placement) + "\n")
        f.write(f"{len(machines)}\n")
        for m in machines:
            f.write(f"{m['strength']} {m['range']} {m['coreHeight']} {m['position'][0]} {m['position'][1]} "
                    f"{1 if m.get('kind') == 'mirror' else 0}\n")
        f.write(f"{len(data['noMachine'])}\n")
        for z in data["noMachine"]:
            f.write(f"{z['position'][0]} {z['position'][1]} {z['radius']}\n")
        f.write(f"{len(data['safeZones'])}\n")
        for z in data["safeZones"]:
            f.write(f"{z['position'][0]} {z['position'][1]} {z['position'][2]} {z['radius']} {z['needsSwitch']}\n")
        f.write(f"{len(data['boxes'])}\n")
        for b in data["boxes"]:
            c, s = b["center"], b["size"]
            f.write(" ".join(str(v) for v in (c[0] - s[0] / 2, c[1] - s[1] / 2, c[2] - s[2] / 2, c[0] + s[0] / 2,
                                               c[1] + s[1] / 2, c[2] + s[2] / 2, SURFACES[b["surface"]])) + "\n")
        f.write(f"{len(data['switches'])}\n")
        for sw in data["switches"]:
            f.write(f"{sw['position'][0]} {sw['position'][1]} {sw['position'][2]} {sw['radius']} {1 if sw.get('weight') else 0}\n")
        f.write(f"{len(data['gates'])}\n")
        for g in data["gates"]:
            c, s = g["center"], g["size"]
            f.write(" ".join(str(v) for v in (c[0] - s[0] / 2, c[1] - s[1] / 2, c[2] - s[2] / 2, c[0] + s[0] / 2,
                                               c[1] + s[1] / 2, c[2] + s[2] / 2, SURFACES[g["surface"]], g["switch"],
                                               g["openSeconds"])) + "\n")
        f.write(f"{len(data['solutions'])}\n")
        for sol in data["solutions"]:
            spots = " ".join(f"{m[0]} {m[1]}" for m in sol["machines"])
            turns = " ".join(str(t) for t in sol.get("turns", [0] * len(sol["machines"])))
            f.write(f"{sol['leg']} {sol['yaw']} {sol['power']} {spots} {turns}\n")
        plinths = data.get("plinths", [])
        f.write(f"{len(plinths)}\n")
        for p in plinths:
            f.write(f"{p['position'][0]} {p['position'][1]} {p['radius']}\n")
