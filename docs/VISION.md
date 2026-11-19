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

### Core loop: walk, place, swing, watch
- **Walk.** Each hole is a self-contained garden: a puzzle course inside a vast landscape. Walk it on foot to read
  the slopes, hazards and machinery.
- **Place.** You carry a handful of artifacts (attractors, repulsors and later inventions) and set them on the
  course. Some machinery is fixed in place.
- **Swing.** From beside the ball: look to aim, hold to set power, release. A short guide line shows the start of
  the path.
- **Watch.** The camera follows the ball as the machinery bends its flight. The ball comes to rest and you walk to
  it for the next stroke. Scoring is golf: strokes, par and a medal under par, plus optional relics to collect.

### Puzzle design rules
- Solutions are ideas, not pixel-perfect aims. A server-side solver measures each intended solution's tolerance
  window and fuzzes for unintended shortcuts.
- Free retries, and a ghost of your last attempt.
- Each level is its own idea and its own place. Rules can change per level: gravity sideways, courses on walls,
  balls that float in water, time that slows in a field.

### Levels (each one different)
| Level | Place | Idea |
| --- | --- | --- |
| The Terraces | Limestone terraces over a sea at sunrise | Learn to swing; one attractor bends a shot around a ruin |
| Salt Mirror | Endless reflective salt flat with monoliths | Repulsors; the horizon is the fairway |
| Inverted Cloister | Monastery courtyard where gravity points up | Putt across the vaulted ceiling |
| Canyon Orrery | Sandstone canyon with rotating stone rings | Moving gravity sources and timing |
| Drowned Library | A flooded hall of shelves | Buoyancy and currents |
| Spiral Lighthouse | A tower whose gravity follows its spiral | The ball falls around the stairs |
| Glass Glacier | Ice caves under an aurora | Frictionless ice and refraction |
| Floating Gardens | Islands in the sky around a gravity core | Orbits: the 2022 game's soul as the finale |

### Playing together
- **Same Hole:** everyone plays the same garden at once, on foot, with no turns. Every solution replays together at
  the end of the hole.
- **Chaos Field:** everyone's machinery affects everyone's ball.
- **Co-op:** split toolkits, so players have to talk.
- **Build-off, Daily Hole and Streamer Cup:** as in the first proposal.
- **Avatars (required):** players always see each other. Everyone has a full-body character that walks, carries
  machinery and swings, with emotes and cosmetics. Unreal's free MetaHuman and mannequin animation sets cover the
  base; the style is original.
- **Bots, like CS:GO:** easy, medium and hard bots fill empty slots or give a solo player opponents. They walk the
  course as avatars too. Because the simulation is deterministic, a bot searches shots with the real physics, then
  adds aim and power error by difficulty: hard bots find near-optimal lines, easy bots make believable mistakes.

### Beautiful
- **Look:** monumental and serene. Sunlit natural landscapes inhabited by vast, weathered structures of an unknown
  civilization: warm stone, bronze, and inlaid glyph lines that glow where gravity bends.
- **Lighting:** golden-hour lighting with Lumen global illumination, MegaLights, volumetric clouds and fog.
- **Detail:** Nanite geometry, Megascans photoscanned assets (free on Fab since November 2026) and the experimental
  Nanite Mesh Terrain.
- **Gravity made visible:** heat-haze distortion, drifting dust and pollen pulled into attractors, and glyphs that
  light up along the field.
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
| Web | The 5.9 WebAssembly + WebGPU target. Experimental: it needs an engine built from source and has no networking yet |
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
