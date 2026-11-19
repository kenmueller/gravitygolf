# Dev log

Newest first. Each entry says what changed, what was verified, and an honest critique.

## For Ken (read this first)

**To play.** Open the project (or run `Tools/editor.sh`), press Play. Keys: WASD and mouse, Shift to run, hold
left click to swing (aim is where you look, longer hold is harder), E picks up and sets down a machine, R returns
the ball, Backspace resets the hole, B jumps you to the ball, right click watches the ball, hold Space while the
ball rolls to run it fast. **F1 plays the
solution for the leg you are on (a hole solved that way is not recorded). Tab opens the menu. Keys 1 to 7 jump
between holes.** (Keys 1 to 9 now.)

**Three things to try, in this order**
1. **Holes 5, 8 and 9** (press the number). Five is the first one meant to be hard: the cup is behind you, behind
   a gate. Eight is new: machines stand only on seven plinths, and each swing has exactly one that works. Nine is
   the last hole: coloured plates and gates with the wiring crossed. Try them without F1.
2. **The Workshop** (Tab, then Workshop). Build a hole: pick a tool along the bottom, drag on the table. Press T to
   test it, T again to come back. CHECK runs every swing without the machines; solving it yourself in a test is the
   other half of the proof. Tab has Keep, My holes, and a code you can copy to a friend.
3. **Playing together.** Tab, Play together, Host this hole. A second copy of the game on your network joins by
   typing this machine's address there. (I tested it with two copies on this Mac: `Tools/hosttest.sh`.)

**What I need from you** (things I cannot judge)
1. **Swing feel and sound.** I have never heard the game. Footsteps, strike, bounce, cup, machine hum, gate: are
   they good, bad or missing?
2. **Do the holes make you think?** Holes 1 to 4 teach, 5 to 7 examine. Which felt like a puzzle and which felt
   like aiming?
3. **The Workshop with real hands.** I drove every tool through the game's own input code (console strokes, fake
   touches, and real mouse clicks in Chrome), but nobody has dragged a wall with a mouse for ten minutes. Tell me
   the first thing that annoys you.
4. **The look in motion.** Stills flatter it. Walk each world and tell me the first thing that looks cheap.
5. **A real phone.** The browser build passes in Chrome's phone emulation. A real phone needs it served over HTTPS
   on your network, which is a decision for you (see `docs/WEB.md`).

**Decisions waiting on you**
- The name. "Gravity Golf" is used by other games (see VISION, Competition).
- Publishing the web build. It runs locally; nothing is public, and I will not publish without your word.
- More holes or more polish next. There are nine holes, and the ninth is written as the last. Holes 8 and 9 took
  five designs between them (three thrown away; the reasons are in the entry below). My recommendation: holes are
  the game, so that is where the next long stretch should go, with the Workshop as the place to prototype them.

## 2026-11-19, session 2 (autonomous): the Workshop, a rival, an ending, real host and join

**Changed**
- **The Workshop: a level editor inside the game.** Ground, sand, stone walls and soft walls are painted on a 50 cm
  grid; safe pads, light plates, weight plates, gates, attractors and repulsors stand on it. Twelve tools, undo,
  and a view from above that slides and zooms. It works with a mouse, a gamepad (the view slides under a fixed
  pointer) and touch (one finger works the tool, two move the view).
- **A hole made there is held to the puzzle law.** Solving it in a test play records your answer. CHECK plays every
  swing with no machine, with the real physics, a slice per frame (about five seconds). Both done and the hole is
  marked proven. If a plain shot works, the Workshop draws that shot on the table.
- **Holes are kept in the save and travel as a short code** (60 to 150 characters), copied and pasted.
- **Its own look:** a white architect's model standing on a dark drafting table ruled with light, under a sky full
  of the outlines of blocks not built yet.
- **A rival to race** (Settings, Rival): a bot that walks, carries the machines and swings in a ghost copy of the
  hole, at three skills. It finished every kind of hole in testing (hard: 2 to 3 minutes; easy: 8 to 19).
- **An ending.** The last hole's card says the course is complete, with your record, and leads to the Workshop.
- **Play together has a screen**: host this hole, or join a host by address.
- The course can load from text, take itself down, and draw a hole with no level around it. The proof's building
  blocks moved into the game (`Sim/GGProof.h`) and the standalone solver now uses the same file.

- **Hole 8, Stepping Stones**, on the salt at noon. New rule: plinths. A machine stands only on a plinth (it snaps
  to the middle), and there are seven. Two swings: out round the end of a long wall to a pad, then all the way back
  to a cup behind two walls. Each swing has exactly one plinth that works; the proof checks the other six fail.
- **Hole 9, Crossed Wires, the last hole**, in the hall of columns at night. Two walls, each with three gates of
  light; in front of each, three weight plates. A plate holds open the gate of its own colour, and the machine
  standing on it draws the ball in and lets it go along the line through itself. The wiring is crossed, so only one
  plate's line runs through its own gate. This is the hole that hole 7 was meant to be ("the weight is also the
  engine"). Any soft swing toward the right plate goes in: nothing in it is a matter of aim.
- **Found and fixed: sand in the floor did nothing.** Where two boxes share the ground under the ball, only the
  first one listed touches it. So no safe pad has ever slowed a ball (which the holes rely on: the next swing starts
  on the pad), and the Workshop's Sand tool was decoration. The Workshop now gives sand its own ground, and courses
  have `soft_patch` for ground that really stops a ball (hole 8's pad has a strip of wet salt behind it).
- A plate and the gates it opens now glow the same colour.
- **Hold Space to run a rolling ball at four times its speed** (the gamepad's bottom button, or FAST on a touch
  screen). Hole 7's answer takes 16 seconds to play out; held, it takes 4.5, with the same result.

**Verified**
- Hole 9: `Tests/Sim/solve.sh wires` passes (one working plate of seven plinths on each swing; 375 and 115
  working swings), and the recorded answers play through in the engine.
- Hole 8: `Tests/Sim/solve.sh stones` passes (one working plinth of seven on each swing; 46 and 84 working swings),
  and the recorded answers play through in the engine. A machine is refused off a plinth and snaps onto one.
- `Tools/workshoptest.sh`: 25 checks pass (every tool, undo, the code there and back, keep and reopen, touch,
  the proof both ways, a test play that records the answer).
- The standalone solver agrees with the in-game check on the starter hole (0 of 110,880 swings without the machine).
- In Chrome: the Workshop with real mouse clicks, and at phone size with touch (tap a tool, drag, pinch, TEST).
  All eight holes boot and their recorded answers sink the ball there, with the same swing counts as on the desktop.
- `Tools/hosttest.sh`: two standalone copies of the game; the joiner connects by address and loads the host's hole.
- `Tools/smoke.sh` (seven holes), `Tools/nettest.sh` (13 of 13), sim tests: all pass after the changes.
- Frame rate from the tee, the first eight holes, after a performance pass: 58, 83, 84, 75, 79, 85, 73 and 78 fps
  (they were 48, 75, 73, 67, 68, about 70, 63 and 68). Workshop 80 fps. What did it: bounced light one notch below
  the engine's maximum (a split-screen comparison showed no visible seam; about 2 ms everywhere); shrubs taken out of
  the ray-traced scene; on hole 1, no shadows from the pines whose shadows fall off the terrace, and heather that
  stands still. Hole 1 is still a hair under the 60 fps budget.
- The saved picture quality is now applied when the game starts. It was only ever applied when you changed it.

**Critique**
- **The in-game proof is still weaker than the solver's, though less so.** It proves "impossible without the
  machines" and "the designer solved it", and (added late in the session) it rates how much placement matters: it
  stands the machines at random 120 times a swing and counts how often some swing still works, then names the
  hole Too easy, Easy, Medium, Hard or Fiendish. The starter rates 28 in 100: Easy. What it still does not check:
  that one machine alone fails, and that an answer survives being nudged. A hole nobody has solved that rates 0 in
  100 is reported as "very hard, or impossible", which is the honest reading.
- **Hole 8 is a choice more than a deduction.** Seven plinths, one right: a player can try them in turn. The lines
  can be reasoned out, but nothing forces it. And its second answer is not pretty: the ball skims the long wall,
  nearly dies, and is reeled through the gap. It passes every check and I would not call it elegant.
- **Hole 9 is clear rather than hard, and its name oversells it.** Three plates, see which one points at the pad,
  done: most players will have it in a few minutes, and a careless one can try all three. I tested the stated idea
  by uncrossing the wires: the answer did not change, because the pad's position alone picks the plate. So the
  crossing is decoration, which is exactly what my own rule exists to catch. I then tried twice to make it
  necessary with a second machine (one as the weight on a far plate of the gate's colour, one as the engine) and
  both failed for the same reason: once the right gate is open the target is either in a straight line, so the
  engine is not needed, or it needs a turn at speed, which gives a handful of working swings. The hole that ships
  is the proven single-machine one, described truthfully in its course file. As a last hole it gathers what the
  course taught; it is not the stumper the course still lacks. Its first draft also had a second answer (kill the
  ball on the south wall and it is reeled through a different plate and gate); moving the plates back closed it.
- **The deeper finding, after five failed designs this session:** in this physics the forgiving move is a straight
  reel (ball, machine, target in a line), and a straight reel from where the ball lies never needs the machine for
  direction. Turning the ball is what makes a machine necessary, and turning is fiddly unless walls funnel it or the
  ball is first killed against a wall. Harder holes need either a new tool that turns the ball reliably (a deflector,
  a rail, a one-way gate) or holes built around the kill-and-collect move, taught on purpose. That is a design
  decision worth a conversation.
- **Three other designs failed first.** A machine pinned to a weight plate that must also bend the ball is too
  fiddly (no cup position gets twelve working swings). Sand cannot pass a ball from one machine to another (a
  stopped ball is never pulled, and sand holds harder than any machine pulls). A three-swing finale passed the
  proof, but two of its swings only worked by killing the ball on a wall. Nine holes is still a short game.
- **The coursekit comment said a pad "catches the ball" for the whole project, and it never did.** Every proof was
  run with the real physics, so nothing shipped wrong, but I designed with a false picture of my own simulation.
- **A Workshop hole can be played together** (added at the end of the session: host from the Workshop's menu; the
  hole's text is sent to whoever joins and again on every change; `Tools/nettest_workshop.sh`, 8 of 8 steps agree).
  Only the host builds; the others walk the hole while it changes under them. It can't be raced against the rival,
  and there is no browsing of other people's holes: sharing is a code in a chat. Honest for a game with no servers,
  but it is not Mario Maker.
- Gates did not show which plate opens them while playing. Fixed at the end of the session: a plate and its gates
  now glow the same colour (a hole with one plate looks as before). Not yet judged in a hole with several.
- **Hole 1's lawn** was a flat olive plane, and it is the first thing a player sees. It now has bold mowing stripes
  and a cooler green. Its hedges were black walls in the shade (the fig-wall scan is that dark however it is
  tinted); they now wear the lawn's scan tiled small, with a clipped crown and a limestone kerb.
- **The look.** The Workshop is the cleanest screen in the game, and the plainest: flat white and one glow colour.
  Fine for a tool. The test view's daytime sky fights the glowing table; dusk would suit it better.
- The rival can race a Workshop hole once you have solved it (it follows your recorded answer). Checked once: at
  hard it solved the starter hole in 31 seconds and 3 swings.
- **The rival is slow to watch** (easy takes 19 minutes on hole 5) and only ever replays the recorded answer with
  errors; it does not think.
- **Process.** My tests had been writing solves and test holes into your save file. They now use a scratch save
  (`GGTestSave`), and I deleted the file they had made. Also: one 90 km slab drew over everything standing on it,
  which cost an hour; big ground is tiled now.

## 2026-11-19, session 1 (autonomous): it runs in a browser; hole 7; playing together

**Changed**
- **The web build works locally.** Built from the source engine, packaged, and run in Chrome with the engine's own
  harness: title screen, menus, walking, carrying, the swing, and every hole's recorded solution sinking the ball.
  Phone emulation: the touch stick and buttons take a stroke. Ready in 4 to 6 seconds; about 100 MB first download.
  `docs/WEB.md` has the numbers and the commands. Nothing is published.
- **What the web needed:** a sky dome (the mobile renderer draws no sky without one, and no sky means no light);
  the scanned plants replaced by generated stand-ins, by taking plants out of the levels and spawning them from a
  list; textures capped at 1K (content went from 593 MB to 54 MB).
- **Hole 7, Dead Weight**, in a new world: a roofless hall of about 500 columns at dawn. New tool: a weight plate,
  down only while a machine stands on it. One of your two machines has to be spent holding the gate open.
- **Playing together.** A listen server and clients. The server turns each action into a command and every machine
  applies it to its own copy of the deterministic hole, so a shot is identical for everyone. Players see each
  other as hovering caddies with a visor in their own colour. Anyone can carry a machine or swing; a machine in
  someone's hands is switched off.
- Tools: `Tools/web.sh`, `Tools/webtest.sh` (browser runs with screenshots), `Tools/nettest.sh` (two players in the
  editor, both copies compared after every step), `Tools/net.sh`.

**Verified**
- Browser: all seven holes boot and their solutions play through (`Tools/webtest.sh tour`).
- Two players: the client carries and places both machines on hole 3, the server swings, and both copies end with
  the ball in the cup. 13 of 13 steps agree.
- `Tools/smoke.sh`: all seven holes, after the networking refactor.

**Critique**
- The web version is clean, not beautiful: no clouds, no bounced light (shadowed canyon walls go nearly black),
  cartoon stand-in trees next to scanned stone. It needs an ambient fill and a look of its own.
- Hole 7's stated idea was "it still works from there". The layout that passed with comfortable margins leaves the
  plate so far from the action that the machine on it does nothing but weigh. The honest idea is smaller: choose
  which machine to spend. The stronger hole (the weight's field is the first bend) is still to be designed.
- Its best answers reel a nearly dead ball along slowly: 14 seconds from swing to cup. Legal, and dull to watch.
  (Session 2 added a fast-forward for this: hold Space.)
- Co-op is plumbing so far. No hole needs two people, there is no way to join a friend (no lobby, no invite), and
  the web build has no networking at all. Late joiners get the hole's starting state, not its current one.
- The avatar has no walk, no hands on the club, and the same pose for everyone. It hovers, which hides a lot.
- Process: three editor hang-ups this session came from my own tooling. A failed build still launched the editor
  (it sat on a dialog); the MCP stop-play tool crashes the editor with a second player's window open; a crash
  leaves a report window holding the MCP port. All three are now handled in the scripts and written in CLAUDE.md.

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
