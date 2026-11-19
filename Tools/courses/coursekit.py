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
        pad, a soft surface catches the ball. `needs_switch` tells the solver which switch the leg is for."""
        self.safe_zones.append({"position": [x, y, z], "radius": radius, "needsSwitch": needs_switch})
        self.no_machine.append({"position": [x, y, z], "radius": radius + 60, "kind": "pad"})
        if pad:
            side = radius * 1.7
            self.box(f"SafePad{len(self.safe_zones)}", (x, y, z - 10), (side, side, 20), "sand", "pad")

    def switch(self, x, y, z=0, radius=70):
        """A plate in the ground. The ball presses it by rolling over it. Returns its index for gates."""
        self.switches.append({"position": [x, y, z], "radius": radius})
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

    def solution(self, leg, machines, yaw, power):
        """The intended answer for a leg: where each machine stands, and one swing that works. The solver verifies
        it and measures how forgiving it is. Required for holes with two or more machines."""
        self.solutions.append({"leg": leg, "machines": [list(m) for m in machines], "yaw": yaw, "power": power})

    def placement_area(self, min_x, min_y, max_x, max_y):
        """Where the solver may try standing machines (the walkable ground)."""
        self.placement = [min_x, min_y, max_x, max_y]

    def write(self):
        data = {
            "id": self.id, "name": self.name, "level": self.level, "par": len(self.safe_zones) + 1,
            "ballRadius": self.ball_radius, "gravity": [0, 0, -980], "killZ": self.kill_z,
            "tee": self.tee, "cup": self.cup, "noMachine": self.no_machine, "safeZones": self.safe_zones,
            "attractors": self.machines, "boxes": self.boxes,
            "switches": self.switches, "gates": self.gates,
        }
        if self.next:
            data["next"] = self.next
        data.update(self.extra)

        out = os.path.join(ROOT, "Content", "Courses", f"{self.id}.json")
        with open(out, "w") as f:
            json.dump(data, f, indent="\t")
            f.write("\n")

        flat = os.path.join(ROOT, "Saved", "SimTest", f"{self.id}.course")
        os.makedirs(os.path.dirname(flat), exist_ok=True)
        with open(flat, "w") as f:
            tee, cup = self.tee["position"], self.cup
            f.write(f"{tee[0]} {tee[1]} {tee[2]}\n")
            f.write(f"{cup['position'][0]} {cup['position'][1]} {cup['position'][2]} {cup['radius']} {cup['captureSpeed']}\n")
            f.write(f"{self.ball_radius} {self.kill_z}\n")
            f.write(" ".join(str(v) for v in self.placement) + "\n")
            f.write(f"{len(self.machines)}\n")
            for m in self.machines:
                f.write(f"{m['strength']} {m['range']} {m['coreHeight']} {m['position'][0]} {m['position'][1]}\n")
            f.write(f"{len(self.no_machine)}\n")
            for z in self.no_machine:
                f.write(f"{z['position'][0]} {z['position'][1]} {z['radius']}\n")
            f.write(f"{len(self.safe_zones)}\n")
            for z in self.safe_zones:
                f.write(f"{z['position'][0]} {z['position'][1]} {z['position'][2]} {z['radius']} {z['needsSwitch']}\n")
            f.write(f"{len(self.boxes)}\n")
            for b in self.boxes:
                c, s = b["center"], b["size"]
                f.write(" ".join(str(v) for v in (c[0] - s[0] / 2, c[1] - s[1] / 2, c[2] - s[2] / 2, c[0] + s[0] / 2,
                                                   c[1] + s[1] / 2, c[2] + s[2] / 2, SURFACES[b["surface"]])) + "\n")
            f.write(f"{len(self.switches)}\n")
            for sw in self.switches:
                f.write(f"{sw['position'][0]} {sw['position'][1]} {sw['position'][2]} {sw['radius']}\n")
            f.write(f"{len(self.gates)}\n")
            for g in self.gates:
                c, s = g["center"], g["size"]
                f.write(" ".join(str(v) for v in (c[0] - s[0] / 2, c[1] - s[1] / 2, c[2] - s[2] / 2, c[0] + s[0] / 2,
                                                   c[1] + s[1] / 2, c[2] + s[2] / 2, SURFACES[g["surface"]], g["switch"],
                                                   g["openSeconds"])) + "\n")
            f.write(f"{len(self.solutions)}\n")
            for sol in self.solutions:
                spots = " ".join(f"{m[0]} {m[1]}" for m in sol["machines"])
                f.write(f"{sol['leg']} {sol['yaw']} {sol['power']} {spots}\n")
        print(f"wrote {out} ({len(self.boxes)} boxes, {len(self.machines)} machines, {len(self.safe_zones)} safe zones)")
