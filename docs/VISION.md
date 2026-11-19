# Gravity Golf, reimagined

**In one line:** a first-person puzzle golf game set in breathtaking, photoreal places, where a vanished
civilization left gravity machinery on every course. Walk the hole, place the machines, take the shot. Alone it
is a puzzle campaign. With friends it plays like Golf With Your Friends, with everyone on foot in the same garden.

**Direction (Ken, 2026-11-18):**
- Look in the spirit of The Talos Principle, but even better, with an original world. Not set in space.
- First-person on foot.
- Puzzle first. Physical plausibility is optional. Every level should be new, exciting and unlike the others.
- Full visual quality on PC, Mac and consoles. Phones and web accept a reduced form.

## What exists

- **The 2022 game** (`legacy/gravitygolf`, `legacy/gravitygolf-admin`). Before each shot you drag gravity wells
  (they pull) and antigravity wells (they push) onto the field, then tap to launch. You get one shot, three stars
  to collect, rectangular walls, a level editor and community levels. It was built with SvelteKit, 2D canvas,
  Firebase, Algolia and Capacitor. Only the first 12 levels are in git; the rest are in the `gravitygolfgame`
  Firebase project. grav.golf and admin.grav.golf return Vercel `402 DEPLOYMENT_DISABLED`. The domain is still
  registered until 2027-07-12.
- **What to keep:** the core idea of shaping the field and then swinging. Nobody else in the genre lets you place
  gravity yourself.
- **What to fix:** physics depended on frame rate, so the same shot played out differently on different devices.
  Collisions were leaky. There was no out-of-bounds, and community stats could be spoofed.

## Competition

| Game | What it proves | Gap we exploit |
| --- | --- | --- |
| Golf With Your Friends (Team17 paid $16.4M for the IP in 2021) | Up to 12 friends putting at once is a durable streamer and party format (it has had Twitch Rivals events) | Conventional courses, little depth |
| Golf Together, Golf It! | Mini golf with friends plus player-made courses sustains communities | Small, dated |
| Super Battle Golf (Feb 2026, about $8, PC) | Simultaneous chaotic golf still breaks out: 100k sold in 48h, a claimed 1M in 6 weeks, 20k peak concurrent players. Console versions due 2027 | Chaos comes from weapons, skill depth is low, players drop off after launch |
| What the Golf? | Surprise and comedy in golf sell (200–500k Steam owners) | Single player |
| Golf Peaks | Puzzle golf has a dedicated audience | 2D cards, modest presentation |
| Golf Clash, Golf Rival | Phones support real-time golf duels at large scale (Golf Clash peaked near $1.1M a week on US Android alone) | Pay-to-win |
| Space golf niche: Swingularity, Wormhole-in-One (Oct 2026), Spacegolf!, Putt It In Orbit, Celestial Golf, Gravity.Golf (JAMSoft) | Gravity golf is an obvious idea, and several people have made it | All are small, mostly 2D, single platform, with no player-placed gravity and no breakout hit |

**Takeaways.**
1. Space golf is crowded but nobody owns it. The way to win is a large step up in beauty and puzzle design, not
   being first.
2. Golf with friends sells and streams well because everyone plays at once and every fail is visible.
3. **The name "Gravity Golf" is taken** in the App Store and Google Play by JAMSoft since March 2026.

## The game

### Core loop: explore, think, set up, swing
- **Explore.** Each hole is a self-contained garden. Walk it on foot to find the machines, the gates, the switches
  and the cup.
- **Think.** With no machine placed, the cup can't be reached. Work out what the ball has to do and what has to be
  where for that to happen.
- **Set up.** Carry the machines into place. Some are locked away, and getting them is part of the puzzle.
- **Swing.** One swing from the tee with a putter. The ball rolls; everything else comes from the machines. If the
  ball stops anywhere but the cup, it returns to the tee.

### Difficulty: walkthrough-hard
Ken's bar: people should need to look up walkthroughs. Hard to figure out, easy to execute.
- **Tools interact.** Attractor (pulls), repulsor (pushes), plate (changes which way is down in a zone), relay
  (catches the ball and re-launches it in a pre-set direction), splitter (one ball becomes two), timer gate (opens
  for a few seconds after the ball crosses a switch).
- **Scarcity and access.** Fewer machines than the obvious solution needs. Machines locked behind gates only the
  ball can open. Spots reachable only by using another machine first. The order of operations is a puzzle.
- **The ball does errands.** In one swing it may have to hit a switch, pass the gate that opened, be caught by a
  relay and only then reach the cup.
- **Fairness.** Once the idea is found, the swing is forgiving. The solver requires a wide tolerance.

### Puzzle design rules
- **Necessity:** every hole is impossible without its tools. `Tests/Sim/solve.sh` proves it with the real physics.
- **Solvability:** the same solver proves a solution exists and measures its tolerance.
- **One new idea per hole,** taught by the level itself. Later holes combine ideas.
- Free retries, and a ghost of the last attempt.

### Levels (each one different)

Built, proven by the solver and played end to end in the engine (`Tools/smoke.sh`):

| # | Level | Place | Idea it teaches | Tools |
| --- | --- | --- | --- | --- |
| 1 | The Terraces | Limestone garden over a sea, a stone halo in the sky | A machine bends a shot around a wall | 1 attractor |
| 2 | Salt Mirror | White salt on an endless brine mirror at sunset, floating monoliths | Safe pads; the machine is yours to move between swings | 1 attractor, 1 pad |
| 3 | The Narrows | Red slot canyon | Pull, then push: two machines, neither enough alone | 1 attractor, 1 repulsor |
| 4 | Lantern Court | Walled court at night under hundreds of paper lanterns | A plate opens a gate of light; the pad banks the press | 1 attractor, 1 plate, 1 gate, 1 pad |
| 5 | The Long Way Round | The canyon again, in the evening | Plan the whole trip: the cup is behind you, behind a gate | 1 attractor, 1 repulsor, 1 plate, 1 gate, 1 pad |
| 6 | Second Wind | The lantern court, later that night | A pull is also a push: a ball slow enough to turn the corner needs a second machine to carry it the distance | 2 attractors |
| 7 | Dead Weight | A roofless hall of columns at dawn, drums of broken columns hanging in the air | A machine can be a weight: a heavy plate holds the gate open only while a machine stands on it, so you choose which one to spend | 1 attractor, 1 repulsor, 1 weight plate, 1 gate |
| 8 | Stepping Stones | The salt again, at noon: white crust, black glass, violet light | Machines stand only on seven plinths: read the line from the ball through each one, and pick the one that lands | 1 attractor, 7 plinths, 1 pad |
| 9 | The Looking Glass | The canyon at noon, three galleries turning two corners | A mirror turns the ball a right angle: stand it where the two lines cross, turned the right way of its two | 1 mirror, 1 pad |
| 10 | Periscope | The salt a third time, at night: a moon far too large over the brine, an aurora, a black wall with one slit of light | Two mirrors draw a path, and the path has to pass through things in order: an S that crosses the plate before it reaches the slit it opens, then a U round a blind wall, with the glasses facing different ways | 2 mirrors, 15 plinths, 1 plate, 1 gate, 1 pad |
| 11 | Crossed Wires | The hall of columns at night, lit by its gates | The last hole. Two machines, two jobs: the attractor is the weight on the plate whose colour matches the gate, the mirror stands beyond the gate and turns the ball into the wall's shadow. The plates are not in the order of their gates | 1 attractor, 1 mirror, 5 weight plates, 5 gates, 12 plinths, 1 pad |

Holes 1 to 4 are the tutorial. Hole 5 is the first exam: everything so far, on both swings. None is walkthrough-hard yet. Ideas for the worlds after them (not built):

| Place | Idea |
| --- | --- |
| Inverted Cloister: a courtyard where gravity points up | Putt across the vaulted ceiling |
| Canyon Orrery: rotating stone rings | Moving gravity sources and timing (timed gates exist in the sim, unused) |
| Drowned Library: a flooded hall of shelves | Buoyancy and currents |
| Spiral Lighthouse: gravity follows the spiral | The ball falls around the stairs |
| Glass Glacier: ice caves under an aurora | Frictionless ice (the surface exists in the sim, unused) |
| Floating Gardens: islands around a gravity core | Orbits: the 2022 game's soul as the finale |

### Playing together
- **Co-op, the main multiplayer mode (2 players, later up to 4).** Partners solve the same hole together, like
  Talos-style co-op. Holes are built so one person can't do it alone:
  - Machines that must be held or operated while the other swings.
  - Split courts: each partner can only reach half the garden, and machines are passed across gaps (or by gravity).
  - Two balls, two cups, one shared field: each partner's setup affects the other's shot.
  - Timed handoffs: one partner's ball opens a gate for the other's.
- **Versus.** Two teams (or solo players) race to solve the same hole in mirrored gardens. First to sink it wins.
- **Avatars (required):** players always see each other: full-body characters that walk, carry machines and swing,
  with emotes and cosmetics.
- **Bots, like CS:GO:** easy, medium and hard partners or opponents fill empty slots. A bot plans with the same
  solver that proves the holes, then acts it out on foot; difficulty sets how fast and how reliably it finds the
  idea. A co-op bot can also take instructions ("hold that", "stand there").
- **Daily Hole, Build-off, Streamer Cup:** as in the first proposal.

**Built so far (2026-11-19):**
- Co-op plumbing: a listen server and clients share one hole, kept in step by commands on a deterministic
  simulation. Anyone can carry a machine or swing. Players see each other as hovering caddies in their own colour.
  Verified with two players in the editor (`Tools/nettest.sh`). Not built: holes that need two people, a way to find
  and join a friend, more than the starting state for someone who joins late, networking on the web.
- A rival bot (Settings, "Rival to race": easy, medium, hard): another caddie works the same hole in a ghost copy
  of its own, starting from the verified answer with error that shrinks on every miss. On hole 3 it takes about
  2, 3 and 10 minutes. Not built: bots as co-op partners, bots in a shared hole.
- Joining a friend: Play together in the menu hosts the hole you are on, or joins a host by address. Verified with
  two standalone copies on one machine (`Tools/hosttest.sh`). Not built: invites, a lobby, anything across the
  internet without opening a port.
- The Workshop (the level editor): build a hole on a grid with twelve tools, test it, and have the game check it
  against the puzzle law (you solved it; no swing works without the machines) and rate how much placement matters
  (Too easy to Fiendish, from 120 random placements a swing). Holes are kept in the save and
  shared as a short code. Mouse, gamepad and touch; it runs in the browser build. A Workshop hole can be hosted and
  played together (the host builds, everyone plays), and the rival can race one once it has been solved. Not built:
  a place to browse other people's holes, the solver's remaining checks in-game (one machine alone must fail).
- Nine holes in six looks, the ninth written as the last, and a fast-forward for a rolling ball (hold Space).
- An ending: the last hole's card closes the course and leads to the Workshop.

### Beautiful
- **Look:** monumental and serene. Sunlit natural landscapes inhabited by vast, weathered structures of an unknown
  civilization: warm stone, bronze, and inlaid glyph lines that glow where gravity bends.
- **Lighting:** golden-hour lighting with Lumen global illumination, MegaLights, volumetric clouds and fog.
- **Detail:** Nanite geometry, Megascans photoscanned assets (free on Fab since November 2026) and the experimental
  Nanite Mesh Terrain.
- **Gravity made visible:** heat-haze distortion, drifting dust and pollen pulled into attractors, and glyphs that
  light up along the field.
- **Physical presence:** footsteps that change with the surface, a visible club that swings and strikes the ball,
  hands that carry things. Gravity machines are tangible devices on tripod legs that you pick up, carry and set
  down, and they hum, glow and unfold when placed.
- **Sound:** the "every solution is a song" idea from the first proposal carries over: machines hum as the ball
  passes.

## Platforms and stack

**Engine: Unreal Engine 5.9** (Ken's decision, 2026-11-18). This is the 5.9 preview now, and the final release
is due in 2027. Claude Code is the only developer and maintainer, so the project is **code-first**. Everything that
can be text is text, and binary Unreal assets are generated from scripts in the repo.

| Layer | Choice |
| --- | --- |
| Project | C++ project. Blueprints are avoided except as thin wrappers |
| Simulation | Deterministic fixed-step 3D ball physics in plain C++ (no UObjects, floating-point contraction off) against quantized collision meshes exported from each level. The same code runs in the game, the replays, the level solver and the server referee |
| Levels | Generated by editor Python scripts in `Tools/levels/`: terrain, Megascans placement, PCG scatter and course machinery. Course rules and par live in JSON |
| Rendering | Lumen (including the new World Cache), MegaLights, Substrate materials, Nanite, Mesh Terrain, SkyAtmosphere, Volumetric Clouds, Niagara, TSR. Phones and web use Lumen Lite or the mobile renderer |
| Materials and effects | Generated by editor Python scripts in `Tools/`, so the scripts are the source and the `.uasset` files are build output |
| Claude's workflow | Unreal MCP (experimental, in-editor). Editor Python run headless from the command line. Command-line builds, automation tests and screenshot captures |
| iOS / Android | Native Unreal builds |
| Desktop | Native Mac and Windows builds. Steam through Unreal's Steam online subsystem |
| Web | The 5.9 WebAssembly + WebGPU target (three rendering tiers; the desktop one has software Lumen). Experimental: on Mac it needs an engine built from source or Epic's fix build, and it has no networking yet. Plan and status in `docs/WEB.md` |
| Online | WebSockets to a Cloudflare Durable Object per room. The C++ simulation compiled to WebAssembly re-runs shots as referee. Supabase stores accounts, holes and leaderboards |

**What only Ken can do:**
- Run the Epic, Apple, Google Play and Steamworks accounts and pay their fees.
- Accept agreements and set up tax and banking.
- Approve store submissions.
- Test on real devices and judge whether it's fun and beautiful.

## Business
- **Free on every platform:** Steam (as free-to-play), the App Store, Google Play and the web.
- No ads, no energy timers, nothing pay-to-win.
- Open question: whether to offer optional cosmetics or a supporter pack to cover running costs.
- Running costs: Apple developer account ($99 a year), Google Play ($25 once), Steamworks ($100), and hosting for
  rooms and data, which is small at launch.
- Open a Steam page early to collect wishlists (Super Battle Golf reached about 100k within 4 weeks of its
  announcement). Run a Next Fest demo, and use streamer playtests as marketing.

## Plan
These timelines are rough. Art and audio are procedural and shader-driven, with CC0 or licensed samples
where needed.

0. **Spike:** one beautiful hole in Unreal, playable on Mac, plus the core feel. This proves the visual bar and Claude's Unreal workflow.
1. **Vertical slice (about 3 months):** two worlds, Same Hole for four players via a link, end-of-hole replays,
   Daily Orbit. Steam page goes live. Playtests with friends and streamers.
2. **Production (6–9 months):** every world, co-op, Build-off, the editor, Streamer Cup, accounts, cosmetics,
   and store builds.
3. **Launch:** the web Daily and demo first, then Next Fest, then 1.0 on Steam, iOS, Android and web. Evaluate a
   console port after launch.

## Open decisions
1. Name: "Gravity Golf" is taken in the app stores. Candidates: *Orrery*, *Perihelion*, *Lightyear Links*,
   *Orbit Club*. grav.golf can stay as the link domain either way.
2. Free with optional cosmetics, or completely free.
