# Putting Gravity Golf on the web

Status on 2026-11-19: **not deployed.** The engine build on this Mac cannot compile the game for the web yet. This
page records what was tried, what blocks it, and the plan for when it is unblocked.

## What Unreal 5.9 offers

The 5.9 preview has a `Wasm` platform: the game compiles to WebAssembly with Emscripten 6.0.10 and draws with WebGPU.
It is experimental and needs a C++ project (this one is). Three rendering tiers can be cooked side by side and the
page picks one per visitor:

| Tier | What it has | Who gets it |
| --- | --- | --- |
| Mobile forward | The phone renderer | Always cooked; phones, weak GPUs |
| Mobile deferred | Phone renderer, deferred lighting | Default on |
| Desktop (`bTargetDesktopTier`) | Deferred, software Lumen bounce light, TSR | Desktop Chrome/Edge with a capable GPU |

None of them has Nanite, hardware ray tracing or MegaLights, which this game's full look leans on. The web version is
the "reduced form" by construction. Chrome and Edge are the supported browsers.

## What was tried here, and what happened

1. The launcher install has `Engine/Platforms/Wasm` (the platform's source and settings).
2. Asked the build tool for a web build: **"The Wasm platform is not supported from this engine distribution."**
3. Looked for why. The Mac launcher build ships only Mac libraries: none of the third-party libraries built for
   Wasm, no page template (`Platforms/Wasm/Build/Template`) and no web tools (`Build/Tools`: the local server and the
   deploy planner). Epic lists this as a known issue of the preview: "The premade builds on Mac and Linux are missing
   part of the Wasm (Web) support files", workaround a source build, with a fix build promised.
4. Installed the Emscripten SDK 6.0.10 at `~/emsdk` (the version the engine pins), which any route needs.
5. Tried cooking the content for the web with the installed editor; see "Cook test" below.

## Ways to unblock, in order of preference

1. **Epic's fix build.** If the launcher offers an update to 5.9 on Mac, take it and run `Tools/web.sh`.
2. **Engine from source.** Needs Ken's Epic account linked to GitHub (only he can do that; the account
   signed in here gets a 404 on `EpicGames/UnrealEngine`). Then: clone the 5.9 branch, `Setup.sh`, build, and point
   `UE_ROOT` at it. Roughly 150 GB of disk and a few hours of machine time, unattended.
3. **A Windows machine.** The Windows launcher build of the same changelist is reported to package for the web.

## Packaging (once unblocked)

```bash
Tools/web.sh Development static
```

That runs `BuildCookRun -platform=Wasm` (build, cook the five levels, stage, package) into `Saved/Web` and asks the
engine's deploy planner for a plan. The output is a folder of static files: `index.html`, the engine's page scripts,
`GravityGolf.js`, a large `.wasm`, and the content containers, each with a Brotli-compressed `.br` twin.

Project settings for the web live in `Platforms/Wasm/Config/WasmEngine.ini`.

## Hosting

The page is static files, but it has three requirements an ordinary static host may not meet:

- **Cross-origin isolation.** The engine uses threads, so every response needs
  `Cross-Origin-Opener-Policy: same-origin` and `Cross-Origin-Embedder-Policy: require-corp`. (The engine can inject
  these with a service worker, `bInjectIsolationHeaders`, for hosts that cannot set headers.)
- **Big files, served pre-compressed.** One measured 5.9 build had a 139 MB `.wasm` that is 26 MB as Brotli; a jam
  game's first load was about 270 MB. The host must serve the `.br` twin with `Content-Encoding: br` and must not
  have a per-file limit below the largest file (containers can be split with `MaxIoStorePartitionSizeMB`).
- **Byte ranges.** The engine reads containers in ranges, so the host must honour `Range` requests.

Recommendation: **Cloudflare R2 behind a Worker** (the planner's `r2worker` host). No per-file limit, no charge for
downloads (the thing a free game with a few-hundred-megabyte first load would otherwise pay for), full control of
headers, and it is the same platform VISION.md already picks for multiplayer rooms. Each deploy goes under its own
prefix, so a release is switched on and rolled back by changing one pointer.

For a first public test before that exists: **itch.io** as an HTML game with "SharedArrayBuffer support" ticked. It is
free, sets the isolation headers for you, and 5.9 web games already run there; its per-file limit is 200 MB.

Vercel and Railway are connected to this session and could serve it, but both charge for bandwidth and Vercel caps
static files, so they suit a small landing page better than the game itself.

## What will need work for this game on the web

- **Download size.** The levels use 8K Megascans textures and scanned trees. The web build needs its own texture
  size caps, or the first load will be far beyond what people wait for.
- **The look.** No Nanite means fallback meshes for cliffs and trees; no ray tracing or MegaLights means the lantern
  court's many lights and the reflections need a second look; the ocean, clouds and custom materials have to be
  checked tier by tier.
- **Controls.** Mouse look needs pointer lock (one click to capture the mouse); phones need touch controls, which
  do not exist yet.
- **Multiplayer.** The web target has no networking in this preview.
