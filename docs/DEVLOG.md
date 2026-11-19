# Dev log

Newest first. Each entry says what changed, what was verified, and an honest critique.

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

## When Ken is back

See the top entry's "For Ken" list.

