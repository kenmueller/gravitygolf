# Gravity Golf, reimagined: proposal

**In one line:** a beautiful puzzle-golf game set on solar systems small enough to hold in your hand. You shape
gravity, then take the shot. Alone it's a puzzle campaign of about 100 holes. With friends it plays like Golf With
Your Friends, except everyone is trying to solve the same impossible hole at the same time.

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

### Core loop: plan, shoot, watch
- Every hole is a small handcrafted star system: the tee planet, a few bodies, and a green planet with the cup and
  a flag.
- You get a tool tray, for example two wells and a wormhole pair. Place the tools, aim, swing, then watch the shot
  play out.
- The ball can land on planets and roll to a stop, so a hole can take one, two or three strokes. The clever line
  is usually a hole in one.
- **Physics stays on one plane; everything else is full 3D.** Planning happens in a readable tactical view, which
  is what a precise puzzle needs on a phone. When the ball launches, a cinematic camera follows it. The last world
  breaks the plane for true 3D holes.

### Puzzle design rules
- **Solutions are ideas, not pixel-perfect aims.** A solver run on the server measures how wide each intended
  solution's tolerance window is. It also fuzzes millions of shots to find unintended shortcuts before a hole ships.
- **A guide line until the first encounter.** Like Peggle, you see the path up to the first flyby or bounce. After
  that you read the bent spacetime grid yourself.
- **Free rewind** and a ghost of your last attempt, so trying ideas is quick.
- **One new mechanic per world.** It is taught without words, then combined with earlier ones.
- **Scoring:** sinking the ball solves the hole. Par is counted in strokes plus tools. Three stardust motes give
  an optional route for perfectionists. Global histograms (as in Opus Magnum) show how your solution ranks, which
  gives puzzle players a reason to keep optimizing.

### Worlds (about 100 holes, plus a bonus course)
| World | New mechanic |
| --- | --- |
| Tee Time | Aiming, landing, rolling, the cup |
| Pull | Gravity wells (the original mechanic). Bonus: a *Classic* course remastering the 12 levels from 2022 |
| Push | Repulsors and space-station walls |
| Ice & Dust | Surfaces: ice slides, dust stops the ball, gas giants slow it |
| Wormholes | Portal pairs you place yourself; momentum is preserved and orientation matters |
| Clockwork | Orbiting moons. A launch-delay dial turns timing into planning instead of reflexes |
| Charge | Gates flip the ball's polarity; planets attract or repel depending on that charge |
| Split | One shot splits into two balls, and both must sink |
| Event Horizon | Black holes, slow motion near the horizon, and finally holes off the plane |

### Playing together
- **Same Hole (versus, 2–12 players).** Everyone solves the same hole at once, with no turns and no waiting. Other
  players' balls appear as ghosts. **At the end of the hole, every player's solution replays at once** in slow
  motion, with awards such as "fewest tools", "closest miss" and "most unhinged".
- **Chaos Field (optional setting).** Everyone shares one field. Your tools bend everyone's shots, and balls collide
  and attract each other. The chaos comes from physics rather than bazookas.
- **Together (co-op, 2–4 players).** Each player gets a different toolkit for the same field, so you have to talk.
  Think Portal 2 co-op, as golf.
- **Build-off.** Everyone has three minutes to build a hole, then everyone plays everyone's holes. You score when
  your hole is solvable but others struggle with it.
- **Daily Orbit.** One verified hole per day, the same for everyone, shared as a Wordle-style emoji result.

### Built for streamers
- **Streamer Cup.** The streamer hosts and up to 100 viewers join from a link in their browser with nothing to
  install. Everyone solves the same hole against a live leaderboard. Chat can vote to drop a repulsor on the
  streamer.
- Instant replays and one-tap vertical clips. Every shot is a replay only a few bytes long, so a clip link opens
  in a browser and lets the viewer try that exact hole.
- A streamer mode that hides lobby codes, and UI that stays readable at 720p.

### Beautiful
"AAA" here means AAA art direction and polish, in the league of Astro Bot or Monument Valley, not photorealism.
That bar is reachable on a phone.
- **Look:** jewel-box orreries. Each hole floats in its own nebula, and the materials are tactile and toy-like:
  enamel, brass, frosted glass, felted grass on tiny planets.
- **Tilt-shift depth of field** while planning, so each hole reads as a miniature. A cinematic camera with slow
  motion as the ball approaches the cup.
- **Technical art:** atmospheric rims, gas-giant bands that move, rings that cast shadows, volumetric nebulae, a
  trail that looks light-painted, and the spacetime grid drawn as golden filigree that ripples when you drop a well.
- **Every solution is a song.** Planets hum a note when the ball passes, bounces are percussion, and sinking the
  ball resolves the chord. Each world has its own key, so elegant solutions literally sound better. The score is
  adaptive, orchestral and ambient.
- **Personality:** the ball is the hero, customizable with skins, trails and finishes. A dry robot caddie,
  built from simple shapes, gives hints. Everything is procedural or built from primitives, so no hand-sculpted
  characters are needed. There is a photo mode.
- **Performance target:** 60 fps on a three-year-old phone, with quality tiers above that.

## Platforms and stack

**Engine: Unreal Engine 5.9** (Ken's decision, 2026-11-18). This is the 5.9 preview now, and the final release
is due in 2027. Claude Code is the only developer and maintainer, so the project is **code-first**. Everything that
can be text is text, and binary Unreal assets are generated from scripts in the repo.

| Layer | Choice |
| --- | --- |
| Project | C++ project. Blueprints are avoided except as thin wrappers |
| Simulation | Deterministic fixed-step 2D physics in plain C++ (no UObjects, floating-point contraction off). The same code runs in the game, the replays, the level solver and the server referee |
| Holes | JSON files in `Content/Holes`, built into the scene at runtime. No hand-made levels |
| Rendering | Lumen (including the new World Cache), MegaLights, Substrate materials, Niagara, Heterogeneous Volumes for nebulae, TSR. Phones and web use Unreal's mobile renderer |
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
