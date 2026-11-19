# Dev log

Newest first. Each entry says what changed, what was verified, and an honest critique.

## For Ken (read this first)

**To play.** Open the project (or run `Tools/editor.sh`), press Play. Keys: WASD and mouse, Shift to run, hold
left click to swing (aim is where you look, longer hold is harder), E picks up and sets down a machine, R returns
the ball, Backspace resets the hole, B jumps you to the ball, right click watches the ball. **F1 plays the
solution for the leg you are on (a hole solved that way is not recorded). Tab opens the menu. Keys 1 to 6 jump
between holes.**

**Start with hole 5** (press 5). It is the first one meant to be hard: the cup is behind you, behind a gate.
Try it without F1 and tell me how long it took and whether the answer felt earned or fiddly.

**What I need from you** (things I cannot judge):
1. **Swing feel and sound.** I have never heard the game. Footsteps, strike, bounce, cup, machine hum, gate: are
   they good, bad or missing?
2. **Do the holes make you think?** Holes 1 to 4 are a tutorial. Hole 5 is the first exam. Which felt like a
   puzzle and which felt like aiming?
3. **The look in motion.** Stills flatter it. Walk each world and tell me the first thing that looks cheap. My own
   list is under "Standing critique of the look" below.
4. **Frame rate on your machine.** I measured 49 to 75 fps in the editor at about 2000x1100. Say so if it feels
   worse than that.
5. **The editor froze once** right after Play was stopped while you were at the keyboard (you force quit it). I
   could not reproduce it. If it happens again, tell me what you had just done.

**Decisions waiting on you**
- The name. "Gravity Golf" is used by other games (see VISION, Competition).
- What to build next. My recommendation: more hard holes first, which needs new tools (relays, splitters, timed
  gates) because two machines and a plate only stretch so far. Then avatars and co-op. Menus, a level editor and
  bots come after there is a game worth sharing.

## 2026-11-19, session 1 (autonomous): touch, menus, saved progress, hole 6, the web build

**Changed**
- **Touch play.** A movement stick under the left thumb, drag anywhere else to look, and round buttons: SWING (hold
  and release; slide the thumb to trim the aim), GRAB/PLACE, WATCH, BALL, RETURN, MENU. It switches on with the first
  touch. Gamepad too: sticks, right trigger to swing.
- **Menus.** Title screen (real launches only, not Play-In-Editor), pause (Tab, P or Escape), hole list, settings
  (look speed, invert, sound, picture quality). Mouse, keys, gamepad or touch.
- **Saved progress.** Best swings per hole. A hole unlocks when the one before is solved. Watching the solution does
  not count as solving. Keys 1 to 6 still jump anywhere, for testing.
- **HUD.** Crisp text at any size, and a card when a hole is solved.
- **Hole 6, Second Wind.** Two attractors. The way to the cup turns a hard corner and then runs the length of the
  court. A ball slow enough to be bent round the corner dies before the cup, so a second machine has to carry it:
  a pull is also a push. Proof: no machine 0 of 110,880; either attractor alone 0 of 442 spots; 230 working swings.
  Set in the lantern court later that night, with pale lanterns streaming across the sky.
- **Web build.** The launcher engine on Mac cannot target the web, so the engine is built from source at `~/UE59`
  (see `docs/WEB.md`). The Mac has no Rosetta, which the engine's C# build tools need for one Intel-only helper, so
  the source tree uses the launcher install's prebuilt copies of those tools.

**Verified in the engine**
- Simulated touches (`GGTouch`): walking with the stick, looking, the BALL button, hold-to-swing.
- Every menu screen draws; the hole list shows a solved hole and locks the rest.
- `Tools/smoke.sh`: all six holes.

**Critique**
- Hole 6's first draft had a timed gate, sold as "slow for the corner, fast for the gate". The solver showed the hole
  needed two machines with the gate open forever: the timer was decoration. I only found out because I tested the
  explanation, not just the hole. Every hole's stated idea should get that test: change the thing the idea depends
  on and check the proof changes.
- Hole 6 is forgiving (230 swings). It is a new idea, not a hard hole. The game still has one exam and five lessons.
- Touch controls have never been under a thumb. The layout is a guess at what is comfortable; a phone will say.
- The title screen's backdrop is the player's own view, club and all. It should be a composed shot of the place.
- The menus are plain text on a dimmed world. Clean, but there is no identity in them yet: no mark, no sound.
- Six holes in four worlds: two of them reuse a world at a different hour. That stretches "every level is different".

## 2026-11-18, session 1 (autonomous): hole 5, real models

**Changed**
- **Hole 5, The Long Way Round.** The cup is a few steps behind the tee, behind a gate of light. The plate that
  opens it is at the far end of the canyon's other gallery. Swing one: round the fin, pulled through the doorway,
  over the plate, pushed down onto the pad (which banks the open gate). Swing two: pushed back through the
  doorway, pulled home through the gate. Both machines on both swings, in opposite orders.
  Proof: no machine 0 of 110,880 swings on either leg; either machine alone 0 of 305 spots on either leg; the
  answers work with 22 and 16 swings of tolerance. Same canyon as hole 3, in the evening.
- `Tools/build_meshes.py`: the game's own models, generated with Geometry Script. A putter (tapered steel shaft,
  rubber grip, blade head with a flange, bronze weights, a sight line) and the gravity machine (turned bronze
  drum, tapered legs with feet and collars, a cradle of claws, two rings round the core).
- The machine's light was 260 candelas in scenes exposed for a 10 lux sun; it bleached the ground white. Now 6,
  tunable live (`gg.Machine.Light`, `gg.Machine.Core`).
- The plinth under each machine was a solid disc of light that read as a white pancake. Now stone with a ring.
- Paper lanterns glow from inside (`M_GG_Lantern`) instead of being flat squares.
- `Tools/levels/canyon.py` takes settings, so a second hole can share a world (`Tools/levels/returning.py`).

**Verified in the engine**
- `Tools/smoke.sh`: all five holes solve through the real player code (1, 2, 1, 2 and 2 swings).
- `Tools/perf.sh`: 49, 75, 73, 67 and 68 fps.

**Critique**
- Hole 5 is harder because it is longer and needs a plan, not because it has a new idea. Its two swings are
  hole 3's trick forwards and then backwards. I think that is a fair exam; it is not yet a puzzle people will
  remember.
- Leg 2 is tight: 16 swings work at the best placement and only half of the nudged placements still do. A
  player with the right idea may still miss several times. That is the wrong kind of hard.
- I found the layout by searching, three drafts in: the first two were trivial (the attractor alone "aims" the
  ball along the line through it) or nearly impossible (a one-swing hairpin). The solver caught both. The lesson
  is that an attractor is a cannon as well as a bend, and every layout has to block that line.
- It reuses the canyon. You asked for levels that are very different; this is a second room in a world, like a
  Talos area with several puzzles. A sixth hole should be a new place.
- With both machines near the tee the field arrows covered the floor in blue and yellow confetti. Fixed the same
  day: the game now draws one combined field on a grid (the sum of every machine's pull and push), so overlapping
  domes show what the ball will actually feel. Two machines side by side nearly cancel, and the arrows nearly
  vanish there. That is true to the physics; whether it reads as "nothing happens here" or as "it's broken" needs
  your eyes.
- Rebuilding the shared materials for the lanterns silently stripped the textures from the first two holes, and I
  only noticed in a later screenshot. `Tools/regen.sh` now regenerates every level whenever materials are rebuilt.
- The models are a clear step up from boxes and balls, but they are still untextured metal. No scratches, no
  maker's marks, no hands holding the club.

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
1. Field arrows are now calm but small; on grass they may be too faint to read at a glance.
2. The Terraces lawn is one flat olive plane. Hedges in shadow are black slabs.
3. There are no hands on the club. Club and machine are clean metal with no wear.
4. Every course is a rectangle with walls. The worlds differ; the play spaces don't.
5. The HUD is debug text.
6. Sunlit sand in the canyon bleaches to flat orange.

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
