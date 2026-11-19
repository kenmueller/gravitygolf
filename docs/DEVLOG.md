# Dev log

Newest first. Each entry says what changed, what was verified, and an honest critique.

## For Ken (read this first)

**To play.** Open the project (or run `Tools/editor.sh`), press Play. Keys: WASD and mouse, Shift to run, hold
left click to swing (aim is where you look, longer hold is harder), E picks up and sets down a machine, R returns
the ball, Backspace resets the hole, B jumps you to the ball, right click watches the ball. **F1 plays the
solution for the leg you are on. Keys 1 to 4 jump between holes.**

**What I need from you** (things I cannot judge):
1. **Swing feel and sound.** I have never heard the game. Footsteps, strike, bounce, cup, machine hum, gate: are
   they good, bad or missing?
2. **Do the holes make you think?** Try each without F1 first. My honest read is that all four are tutorial
   difficulty. Tell me which one felt like a puzzle and which felt like aiming.
3. **The look in motion.** Stills flatter it. Walk each world and tell me the first thing that looks cheap. My own
   list is under "Critique" below; the club and the lawn are at the top of it.
4. **Frame rate on your machine.** I measured 48 to 74 fps in the editor at about 2000x1100 (numbers below). Say
   so if it feels worse than that.

**Decisions waiting on you**
- The name. "Gravity Golf" is used by other games (see VISION, Competition).
- What to build next. My recommendation: hard holes before anything else (plates, relays and timed gates with
  fewer machines than you want), then avatars and co-op. Menus, a level editor and bots come after there is a
  game worth sharing.

## 2026-11-18, session 1 (autonomous): performance, measured for the first time

**Changed**
- `GGPerf <seconds>` console command and `Tools/perf.sh`: average, slowest 1% and worst frame, per hole.
- Found the editor throttles to 3 fps when it is not the front app, so every automated run so far was at 3 fps
  and hid the real numbers. Turned off in `Config/DefaultEditorSettings.ini`.
- Three fixes, each found with `ProfileGPU` and confirmed by switching it off and measuring:
  1. The field markers (rebuilt every frame) and the club were re-captured by Lumen every frame: 6 to 14 ms.
     They no longer take part in bounced light or ray tracing.
  2. Lantern Court's 26 swaying pines redrew the moon's shadow map every frame: 12 ms. They are background
     silhouettes; they cast no shadow now.
  3. Clouds cost 8 ms (the view plus the sky light's capture of them). Sample counts halved.

| Hole | Before | After |
| --- | --- | --- |
| The Terraces | 35 fps | 48 fps |
| Salt Mirror | 48 fps | 74 fps |
| The Narrows | 52 fps | 74 fps |
| Lantern Court | 34 fps | 66 fps |

Editor (Play-In-Editor), M4 Max, 2027x1090 panel, all settings at their highest, from the tee.

**Critique**
- I built four worlds before measuring once. That was a process failure; `Tools/perf.sh` now exists so it can be
  part of every art pass.
- The Terraces is still under 60 on the fastest Mac there is. What is left is spread thin: Lumen, shadows for 180
  swaying plants, the sky capture. It needs a settings tier below "everything on", which a PC or console build
  needs anyway.
- These are editor numbers from one spot per hole. A packaged build should be faster; walking around may be
  slower. Neither is measured. The standalone launch still hangs (see CLAUDE.md), so I cannot measure a real build.
- Phones and the web are a different renderer entirely and untouched.

## 2026-11-18, session 1 (autonomous): hole 4, solution playback, smoke test

**Changed**
- **Hole 4, Lantern Court.** A plate in the ground opens a gate of light. Pressing it only counts if the ball then
  comes to rest on the safe pad, which banks the press for the next leg. Leg 1: bend through the plate onto the
  pad. Leg 2: carry the machine on and bend through the open gate to the cup. World: a walled court at night,
  about 750 paper lanterns in the air, a column of them rising from the cup, pines against a star dome.
- Sim: switches, gates (timed or latched), switches held across legs. The solver proves each leg with them.
- Every course records its intended answer. `GGDemo` (or F1) plays it through the real player code: carry the
  machine, set it down, walk to the ball, swing. Keys 1 to 4 jump between holes.
- `Tools/smoke.sh`: opens each hole in the editor, plays the solution, checks the result. All four pass.

**Verified in the engine**
- `Tools/smoke.sh`: Terraces 1 swing, Salt Mirror 2, The Narrows 1, Lantern Court 2.

**Critique**
- The first design for hole 4 (a repulsor used as a mirror, with a timed gate) never got more forgiving than a
  few swings out of thousands. I shelved it instead of shipping a hole about precision. Timed gates are in the
  sim and unused.
- Lantern Court's idea is a rule ("the pad banks the press"), which the level has to teach without words. It
  doesn't yet: nothing on the gate or the pad shows that connection.
- Four holes, four ideas, zero hard puzzles. The puzzle law is enforced, the difficulty isn't there.
- The smoke test proves the recorded answers work in the engine. It does not prove a person can find them.

**Standing critique of the look** (worst first, judged against The Talos Principle)
1. The club is a brown box on a grey stick, in view all the time. There are no hands.
2. The Terraces lawn is one flat olive plane. Hedges in shadow are black slabs.
3. Machines are primitive shapes: readable, not beautiful.
4. Every course is a rectangle with walls. The worlds differ; the play spaces don't.
5. The HUD is debug text.

## 2026-11-18, session 1 (autonomous): polish and art pass on hole 1

**Changed**
- Machines: sim reports when a machine takes hold of the ball and when it lets go; the machine flashes and plays a
  rising or falling tone. Repulsors (negative strength) push whenever the ball is inside the dome.
- Field preview: a dashed ring always; flowing arrowheads only while carrying a machine, standing over the ball or
  watching a shot. Look is tunable live (`gg.Field.*` console variables).
- Club: rebuilt as grip, shaft and blade held like a first-person tool, with backswing, strike and follow-through.
  Tucked low while walking. Pose tunable live (`gg.Club.*`).
- Keys: Shift to run, R returns the ball, Backspace resets the hole. Falling off the world returns you to the ball.
- Solved holes advance to the course's `next` level when it exists.
- Level art: mowing stripes on the lawn, clipped fig hedges with heather, a dressed wall (plinth, pilasters, cap,
  glyph band, pier), floating rock islands with pines, hazier sea stacks, warm front light, local exposure.
- Pines and shrubs were grey because their material lives in the Procedural Vegetation Editor plugin; enabled it.
- Tools: `Tools/rebuild.sh`, `Tools/regen.sh`, `Tools/play.sh`, `Tools/console.sh`, `Tools/fab_search.sh`,
  `Tools/fab_install.sh`. Console test commands take one comma-separated argument, because the console drops
  negative numbers passed separately.

**Verified in the engine**
- Through the real player code (`GGCarry`, `GGSwing`): placing on the court is refused, placing at the solver's
  spot works, and the swing through the club animation sinks the ball in one attempt.

**Critique**
- The hole reads as a place now, but it is still a rectangle of lawn with one wall. Composition is flat: no height
  changes, no foreground interest at the tee.
- The lawn texture is soft and yellowish up close. Hedges in shadow go nearly black.
- The club is primitive shapes and there are no hands.
- Sounds are synthesized and unheard by me; they need a human ear.
- Nobody has hand-played since the machine rule changed.

## 2026-11-18, session 1 (autonomous): holes 2 and 3

**Changed**
- `Tools/courses/coursekit.py`: one small builder for course definitions (boxes, tee, cup and court, safe pads,
  machines, intended solutions). Each hole's script is now a page.
- Solver (`Tests/Sim/solve.sh`): handles several machines. With one machine it sweeps every placement. With two or
  more it proves that no single machine is enough, then checks the designer's intended answer and nudges each
  machine a pace in four directions to make sure the answer isn't fragile.
- Design search (`Tests/Sim/search.sh`): random search over all machines and swings; finds candidate answers for a
  new layout in about 15 seconds.
- **Hole 2, Salt Mirror.** Teaches the safe pad and that the machine is yours to move: bend onto the pad, carry the
  machine on, bend the other way. Proven per leg. World: white salt polygons on an endless brine mirror, obsidian
  monoliths with a line of light, an armada of floating monoliths at sunset.
- **Hole 3, The Narrows.** Attractor plus repulsor: "pull, then push." Proven that neither machine alone can do it
  from any spot. World: red slot canyon from Megascans cliffs and sand, a built doorway, a stone ring overhead.
- Safe pads settle the ball at their center, so the next leg starts from a known spot (and matches the proof).
- Levels chain: Terraces, Salt Mirror, The Narrows.
- Fixes: player starts in every level (the player failed to spawn in hole 2), content auto-import off (a new course
  JSON blocked the editor on a dialog), lens flares off.

**Verified in the engine**
- Hole 2: control swing fails, with the machine the ball is "Safe", old machine spot fails for leg 2, moved machine
  solves it.
- Hole 3: fails with no machine, fails with the attractor alone, solved with both.

**Critique**
- Holes 2 and 3 are still teaching holes. Nothing here would send anyone to a walkthrough yet. The hard holes need
  the other tools (plates, relays, gates) and scarcity; those don't exist.
- Hole 2's first leg works from 31% of placements, which is generous. Its two legs are mirror images, so the second
  is not a new thought, only a repeat.
- Hole 3's intended answer is something the search found, not something I can explain to a player in one sentence
  beyond "pull, then push". Whether a person can reason their way to it is untested.
- Soft walls (sand surface on rock) are a cheat to stop bank shots. In the canyon nothing shows the walls are soft.
- Salt Mirror and the canyon look striking in stills; neither has been walked through by a person. The canyon's
  sunlit sand blows out to flat orange, and its walls are flat slabs up close.
- With two machines the field arrows overlap into noise.
