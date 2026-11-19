# Dev log

Newest first. Each entry says what changed, what was verified, and an honest critique.

## For Ken (read this first)

**To play.** Open the project (or run `Tools/editor.sh`), press Play. Keys: WASD and mouse, Shift to run, hold
left click to swing (aim is where you look, longer hold is harder), E picks up and sets down a machine, R returns
the ball, Backspace resets the hole, B jumps you to the ball, right click watches the ball, hold Space while the
ball rolls to run it fast. **F1 plays the
solution for the leg you are on (a hole solved that way is not recorded). Tab opens the menu. Keys 1 to 9 jump
between holes, 0 is hole 10, minus is 11, equals is 12 and the left bracket is 13.** F turns a mirror.

**Four things to try, in this order**
1. **Holes 5, 10, 11 and 12** (press the number; 0 is hole 10, minus is 11, equals is 12). Five is the first one
   meant to be hard: the cup is behind you, behind a gate. Ten, Periscope, is the best looking: two mirrors, the
   salt at night under an aurora. Eleven and twelve are the newest idea and the nearest this game has come to The
   Talos Principle: gates with rails stop a machine in your arms, so the order you carry things in is the puzzle.
   Eleven (Leapfrog) teaches it. **Twelve (After You) is the one I most want you to try without F1**: it took me
   one design, which worries me, so tell me whether it made you stop and think or just made you walk.
2. **The Workshop** (Tab, then Workshop). Build a hole: pick a tool along the bottom, drag on the table. Press T to
   test it, T again to come back. CHECK runs every swing without the machines; solving it yourself in a test is the
   other half of the proof. Tab has Keep, My holes, and a code you can copy to a friend.
3. **Playing together.** Tab, Play together, Host this hole. A second copy of the game on your network joins by
   typing this machine's address there. (I tested it with two copies on this Mac: `Tools/hosttest.sh`.)
4. **Your phone.** Run `Tools/phone.sh` in a terminal, point the phone's camera at the square it prints (same
   Wi-Fi), and choose to visit the page when the phone warns about the certificate. Tell me what happens. This is
   the one thing in the whole project I have no way to try: I can prove the Mac serves the game correctly
   (`Tools/phone.sh check` does) and that touch works in Chrome's phone mode, not that a real phone runs it.

**What I need from you** (things I cannot judge)
1. **Swing feel and sound.** I have never heard the game, but I can measure it (`Tools/sounds.sh`): every
   sound exists, none clips, and their levels sit within 10 dB of each other. Measuring showed the footsteps were
   as bright as cymbals (fixed) and that the worlds were silent between events (each place now has a quiet bed of
   air). **There is now music**: four slow chords per place and a glass note now and then, about 9 dB under a
   footstep. I checked what can be checked without ears: every chord contains exactly the notes I wrote, no chord
   holds two notes a semitone apart, the level never drops out, and the loop has no seam. Whether it is pleasant
   or irritating is yours to say; Settings has a Music volume (and Off).
2. **Do the holes make you think?** Holes 1 to 4 teach, 5 to 7 examine. Which felt like a puzzle and which felt
   like aiming?
3. **The Workshop with real hands.** I drove every tool through the game's own input code (console strokes, fake
   touches, and real mouse clicks in Chrome), but nobody has dragged a wall with a mouse for ten minutes. Tell me
   the first thing that annoys you.
4. **The look in motion.** Stills flatter it. Walk each world and tell me the first thing that looks cheap.
5. **A real phone.** See "Your phone" above: one command, and I need you to hold the phone.

**Decisions waiting on you**
- The name. "Gravity Golf" is used by other games (see VISION, Competition).
- Publishing the web build. It runs locally; nothing is public, and I will not publish without your word.
- **How the game gets hard.** You asked for puzzles people look up walkthroughs for. None of these is one yet.
  With plinths and mirrors the holes became pure deduction, which is fair but quick. What makes The Talos Principle
  hard is logistics: the thing that opens the door is the thing you need on the other side of it. This game had
  none, because the player and the machines walked through every gate. **So I built it, as a new kind of gate that
  changes no existing hole: the grille** (a gate with rails), which stops a machine in your arms while it is shut.
  Hole 11, Leapfrog, teaches it; hole 12, After You, uses it with a mirror that cannot be the weight, so the ball
  has to open the door for the machine. These are closer to what you asked for, not yet there. If you dislike the
  idea, it is two holes and one flag to take out.
- More holes or more polish next. There are thirteen holes, and the thirteenth is written as the last. I did not wait
  for the answer: I kept doing both (a new hole, then the worst-looking hole made better). Say if you want one or
  the other.

## 2026-11-19, session 3 (autonomous): Periscope, music, a phone command

**Changed**
- **Hole 10, Periscope** (the finale is now hole 11). Two mirrors on fifteen plinths. First an S: out, up and along,
  over a plate that opens a slit in a black wall. Then a U round a blind wall to the cup, with the two glasses facing
  different ways. The proof tries all 960 ways to stand and turn two mirrors on each swing: one works.
- **Its place: the salt at night.** A moon far too large low over the brine, an aurora over the far end, both drawn
  by the sky itself so the brine mirrors them. Black walls outlined in light.
- **A mirror's frame is dead.** The end of the frame had been acting as a second, smaller mirror turned the other
  way, and (once made soft) a full swing still dribbled past it onto a pad. Each was a second answer on two holes.
  A ball that runs into the end of a frame now stops there. The finale went from two answers a swing to one.
- **Music.** Four slow chords per place (D major for open ground, A Dorian in the canyon, E minor at night, F
  Lydian in the flooded hall, C major in the Workshop) and a glass note every few seconds. Written out by the game
  at start, off the main thread. Settings has a Music volume.
- **Plates and gates are marked by number as well as colour.** Dots like a die on the plate, the same number of
  studs over its gates, on any hole with more than one plate. The finale depends on telling five colours apart,
  and one man in twelve cannot.
- **Light plates have a rim and a groove** instead of being a flat disc of colour.
- **Lantern Court and Second Wind are brighter.** From the tee the paving was a black plane.
- **`Tools/phone.sh`**: serves the web build to a phone on the same Wi-Fi over HTTPS, with a certificate made on
  the spot, and prints the address and a QR code. `check` proves the Mac's side.
- `Tools/look.sh`: open a level, play, save pictures from the tee and from any views given.

- **Grilles, and hole 11, Leapfrog** (the finale is now hole 12). A grille is a gate with rails: while shut it
  stops a machine being carried, as well as the ball. The player alone walks through. Leapfrog has two of them and
  two machines that both have to end up beyond the first wall: one holds the near plate while the other is carried
  through; that one, on its plate beyond, holds the second grille for the first to follow. The proof plans the
  carries (three, in that order) and the demo makes them.

- **Hole 12, After You** (the finale is now hole 13). An attractor and a mirror. A mirror is too light to hold a
  plate, so the leapfrog cannot work: the weight lets the mirror through the first grille and can never follow.
  What opens the second grille is a plate the ball presses, kept pressed by a pad. So the first swing is spent on
  that, and the order is mirror, ball, weight. The proof shows the idea carries the hole: before the ball banks its
  plate 24 ways of standing the machines can be reached and one sinks the swing; take the banking away and the
  second leg's carries cannot be made at all.

**Verified** (results at the end of this entry are from the runs made after the last change)
- `Tools/testall.sh` after the music, marks and lighting changes: ALL PASSED (sim tests, eleven proofs, every hole in
  the engine, the Workshop, two players in step, two players in the Workshop, host and join).
- Frame rates from the tee, same run: Terraces 60, Salt Mirror 80, Narrows 82, Lantern Court 76, Long Way Round 80,
  Second Wind 88, Dead Weight 76, Stepping Stones 77, Looking Glass 98, Periscope 98, Crossed Wires 74.
- The web build made after the music: music starts in the browser ("Music: wind (playing)"), the phone-size play
  test and the two holes re-run pass.
- Grilles: the planner has a unit test (the leapfrog, a mirror too light to take part, a grille the ball has
  banked). Leapfrog's proof passes with the carries printed. In the engine, `Tools/grilletest.sh` moves the player
  in straight steps and the game's own rule decides: five checks pass (stopped when shut, through when open, shut
  again when the weight is lifted, the ball's grille opened from beyond, empty hands walk through).
- After You: its proof passes (leg one 50 working swings, leg two 15), its answer plays in the engine in two swings
  with the four carries in order.
- **The last full run, after every change in this entry:** `Tools/testall.sh` ALL PASSED with thirteen holes (sim
  tests, thirteen proofs, every hole in the engine, grilles, the Workshop, two players in step, two players in the
  Workshop, host and join). Then a fresh web build, and in Chrome: all thirteen holes' answers sink the ball (the
  two grille holes with their carries), the Workshop at phone size with touch, the phone-size play test, and
  `Tools/phone.sh check`. After that run two things changed and were tested on their own: the rule against reaching
  from inside a shut grille (`Tools/grilletest.sh`, seven checks) and the rival's order of carries (it finishes
  both grille holes at all three skills).
- Sim tests pass, with a new one for the frame's end. All thirteen proofs pass.
- Periscope in the engine: its recorded answer sinks the ball in two swings; the rival finishes it at all three
  skills (50 s, 96 s, 249 s).
- In Chrome, from the web build made before the music: all eleven holes' answers sink the ball, the Workshop works
  with a mouse and with touch at phone size, the phone-size play test passes.
- The music, by measurement: the notes written are the notes played (within 3 cents, which is the shimmer), level
  steady within 3 dB, loop seam smaller than an ordinary step between samples, peak 20 dB under full scale.

**Critique**
- *Decisions.* I added a hole when the finale already existed, which meant renumbering. The reason: holes 9 and 11
  each teach the mirror as one of two tools, and nothing made the player compose two mirrors. What would make it
  wrong: if Periscope turns out easier than the finale's neighbours, it is a third tutorial late in the game. With
  plinths, the hole is pure deduction; a player who sees it will solve it in minutes. It is a good puzzle, not a
  hard one. Nothing in this game is yet "look up a walkthrough" hard, and Ken asked for that.
- *The first design failed the proof twice*, usefully. A straight putt went over the plate and through the gate
  (I had put the plate on a line with both). And the second swing had two working swings out of 110,000 because it
  left the pad through the pad's own backstop. Both are now notes in CLAUDE.md.
- *The look.* The first pictures were bad: a white sticker of a moon with a bar of light across it, a black void
  for the sea, a black rectangle for a wall. Three rounds fixed the worst thing each time. The aurora drew as
  fourteen contour lines until its ray walk was dithered. What is still weak: the crust is a plain lit floor, and
  the aurora is soft glow rather than sharp curtains. On the web the brine does not mirror anything, so that
  version loses the picture's best part.
- *Music is the riskiest thing in this entry.* I cannot hear it. I chose the most conservative music there is
  (slow consonant pads, a pentatonic glass note) and tested everything testable, but "in tune and steady" is not
  "good". It is quiet and it has an off switch for that reason.
- *The field arrows* (blue arrowheads that stream along a machine's pull) look like debug marks in a still
  picture. I looked at replacing them and kept them: they move, and they are how a placement is read. A player
  might still find them ugly.
- *Grilles.* The decision I am least sure of. For: it is the one mechanic I can find that makes order matter, and
  it is opt-in. Against: it is a second kind of gate that looks like the first with three rails added, and a player
  who misses the difference will think the game is broken when they are put back. The message ("A machine won't
  pass a shut grille") is the only teaching. Leapfrog itself is a tutorial, not a hard hole: one dead end (carry
  the second machine straight to the far plate and the first can never follow) and then it is done. Its swing has
  only 18 working swings out of 110,000, which is not the "easy part" its own description claims. Not verified:
  a human walking it (my tests move the player in straight jumps), touch on a real screen. Two players: the joined
  player made all three carries through the grilles and the host's swing sank the ball, both copies of the hole
  agreeing at every step, and the host refused the joined player's attempt to set a machine down beyond a shut
  grille (`Tools/nettest_grille.sh`). The rival now makes its carries in an order the grilles allow.
- *I found a hole in the grille rule by rereading it, not by testing it.* Empty hands walk into a shut grille, and
  my first rule let someone standing inside the bars pick a machine up and walk out the far side with it. The test
  I had written could not have caught it; it has that case now, with a control beside it.
- *After You* passed its proof on the first design, which no hole since the fourth has done. Either the planner
  makes design easier (it tells me at once whether an order exists) or the hole is simpler than I think. Its second
  swing has 15 working swings, near the floor of 12. Its look is the weakest of the thirteen: the hall at noon is a
  beige floor under a plain blue sky, and the gates' colours wash out in sunlight.
- *Frame rates* for the two newest holes read exactly 60.0 with every frame at 16.7 ms: the session had become
  capped at 60 (the GPU time was 12 ms, and Lantern Court, re-measured beside them, read 60.0 too, an hour after
  reading 76). I could not find what capped it; the screen may have gone to sleep. They are not slower than their
  sister holes, but I have no uncapped number for them.
- *Process.* An hour after writing down "never edit a shell script while it is running", I edited the grille test
  while the full suite, which runs it, was running. It happened not to have reached it. I also blamed a hung connection on TLS before testing plain HTTP, which showed in one step that it was the
  firewall. And a server I thought I had stopped kept the port, because I had killed a subshell instead of it.

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
- **Mirrors: a third machine.** A glass standing at 45 degrees; the ball leaves it at the angle it came in at with
  four fifths of its speed. Carried like any machine; F turns it to its other diagonal (right shoulder on a gamepad,
  TURN on touch). In the deterministic simulation it is an ordinary box seen from its own frame, so nothing else
  changed (the old determinism hash is identical). The solver, the search, the rival, co-op and the Workshop all
  know it.
- **Hole 9, The Looking Glass**, in the canyon at noon: one mirror, two corners, a pad between.
- **Hole 10, Crossed Wires, the last hole** (first written as hole 9), in the hall of columns at night. Two walls, each with three gates of
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
- Frame rate from the tee after a performance pass, holes 1 to 9: 58, 81, 83, 78, 77, 86, 75, 82 and 74 fps
  (the first eight were 48, 75, 73, 67, 68, about 70, 63 and 68). Workshop 80 fps. What did it: bounced light one notch below
  the engine's maximum (a split-screen comparison showed no visible seam; about 2 ms everywhere); shrubs taken out of
  the ray-traced scene; on hole 1, no shadows from the pines whose shadows fall off the terrace, and heather that
  stands still. Hole 1 is still a hair under the 60 fps budget.
- The saved picture quality is now applied when the game starts. It was only ever applied when you changed it.
  The game also sets its own rendering resolution now (70% of the window at the highest quality, sharpened up by
  the engine's upscaler): a packaged game would otherwise draw at 100%, twice the pixels everything was measured at.
- **A false alarm about those frame rates, and its real cause.** Late in the session every hole measured about 40%
  lower. I first blamed the Mac (its load average was 31: media analysis, a backup upload) and wrote that down. That
  was wrong. The editor was still set to two players after the co-op tests, so every later run drew the game twice;
  it also crashed the editor once when play stopped. `Tools/play.sh` now sets the player count itself before every
  start. Measured again afterwards, with the Mac still busy: 58, 81, 83, 78, 77, 86, 75, 82 and 74 fps for holes 1
  to 9.

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
- **The finale went through four versions, and the third was a lie I caught.** Version 3 (one machine, three
  coloured plates before each wall) passed every check, and its stated idea was crossed wiring. By my own rule I
  tested the idea by uncrossing the wires: the answer did not change, so the crossing was decoration. Two attempts to
  make it necessary with a second field machine failed: once the right gate is open the target is either in a
  straight line (so nothing more is needed) or needs a turn at speed (a handful of working swings).
- **What fixed it was a new tool, the mirror** (see below). With a machine that turns the ball exactly, the finale
  became what it claimed: the attractor is the weight on the plate of the gate's colour, the mirror turns the ball
  into the wall's shadow. Of 264 ways to stand the two machines, one works on each swing; neither machine can do
  it alone; and uncrossing the wires now moves the answer to a different plate. A mirror is too light to hold a
  plate down: without that rule the mirror could be spent as the weight and the roles collapsed.
- **The deeper finding, after five failed designs:** in this physics the forgiving move is a straight reel (ball,
  machine, target in a line), and a straight reel from where the ball lies never needs the machine for direction.
  Turning the ball is what makes a machine necessary, and with fields alone turning is fiddly. The mirror is the
  answer to that: exact, readable, and it makes right-angle geometry something to think with.
- **Still true of holes 9 and 10: they are logic more than struggle.** A careful player reads the lines and the
  colours and has each in a few minutes. They are the first holes where the reasoning is the whole of it, which is
  the right direction, but nobody will need a walkthrough yet. Harder ones want two mirrors in a chain, or a mirror
  with a field bending the ball onto it.
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
