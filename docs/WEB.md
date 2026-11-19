# Putting Gravity Golf on the web

Status on 2026-11-19: **not deployed.** The launcher engine on this Mac cannot build for the web, so the engine is
being built from source. This page records what was tried, the route taken, and how hosting will work.

## What Unreal 5.9 offers

The 5.9 preview has a `Wasm` platform: the game compiles to WebAssembly with Emscripten 6.0.10 and draws with WebGPU.
It is experimental and needs a C++ project (this one is). Epic's own instructions ship in the engine source at
`Engine/Platforms/Wasm/Docs/` (start with `GettingStarted.md`) and as a procedure for an agent at
`Engine/Platforms/Wasm/Skills/web-deploy/SKILL.md`. Read those before changing anything here.

Every tier uses the phone renderer in this preview:

| Tier | What it is | Who gets it |
| --- | --- | --- |
| Mobile forward | Phone renderer, forward shading. Has a known reflection bug (metal draws black) | Phones, weak GPUs, Firefox |
| Mobile deferred | Phone renderer, deferred shading | Desktop-class GPUs; the default there |
| Desktop (`bTargetDesktopTier`) | Deferred renderer with software Lumen | **Experimental, not supported in Preview 1.** Only runs when a link asks for it (`?tier=desktop`). Off in this project |

No tier has Nanite, virtual shadow maps, hardware ray tracing or MegaLights, which this game's full look leans on.
The web version is the "reduced form" by construction. Chrome and Edge are the reference browsers; Safari and Firefox
run with caveats. The heap is capped at 4 GB.

## What was tried here, and what happened

1. The launcher install has `Engine/Platforms/Wasm` (the platform's source and settings).
2. Asked the build tool for a web build: **"The Wasm platform is not supported from this engine distribution."**
3. Looked for why. The Mac launcher build ships only Mac libraries: none of the third-party libraries built for
   Wasm, no page template (`Platforms/Wasm/Build/Template`) and no web tools (`Build/Tools`: the local server and the
   deploy planner). Epic lists this as a known issue of the preview: "The premade builds on Mac and Linux are missing
   part of the Wasm (Web) support files", workaround a source build, with a fix build promised.
4. Installed the Emscripten SDK 6.0.10 at `~/emsdk` (the version the engine pins), which any route needs.
5. The installed editor will not even list the web platform for cooking until it finds that SDK
   (`WASM_EMSDK_ROOT`).

## The route taken: engine from source

Ken linked his Epic account to GitHub on 2026-11-19. The engine source (tag `5.9.0-preview-1`, the same version as
the launcher install) is at `~/UE59`, a path with no spaces. The Emscripten SDK is at `~/emsdk`.

```bash
cd ~/UE59 && ./Setup.sh --force --exclude=Win64 --exclude=Linux --exclude=LinuxArm64 --exclude=Android
```

If Epic ships the fix build for the Mac launcher install, `UE_ROOT` can point at that instead.

## Packaging

```bash
Tools/web.sh Development static     # iterate
Tools/web.sh Shipping r2worker      # anything public; Development builds are blocked by the release scan
```

The script copies the project to `../gravitygolf-web` (so building against the source engine never touches the
binaries the launcher editor uses), drops the MCP toolset plugins from the copy (they break a web cook), runs
`BuildCookRun -platform=Wasm`, checks the result with the engine's `check_stage.py`, and writes a deploy plan. The
first run also builds the editor from source, which is the long part.

Output, under `../gravitygolf-web/Saved/Web`: the `Wasm` folder is the whole site (`index.html`, the page scripts, a
service worker, `GravityGolf.js`, a large `.wasm`, the content containers, and a Brotli `.br` twin of each large
file). `Wasm-Debug` must never be uploaded. `play-gravitygolf.sh` serves it locally with the right headers.

Web settings live in `Config/DefaultEngine.ini` under `[/Script/WasmTargetPlatform.WasmTargetSettings]`.

## Hosting

The site is static files, but the host has to get several things right (Epic's `Docs/Deploy.md` has the full table
and ready-made configuration for each host):

- **HTTPS.**
- **Cross-origin isolation**, because the engine uses threads: `Cross-Origin-Opener-Policy: same-origin` and
  `Cross-Origin-Embedder-Policy: require-corp` on the page, and `Cross-Origin-Resource-Policy` on every file. For a
  host that cannot set headers, `bInjectIsolationHeaders` makes the service worker add them.
- **Byte ranges answered raw**, with no compression applied on the fly. The engine reads the containers in ranges and
  a compressed range corrupts them.
- **The `.br` twin for whole-file downloads**, which cuts the download about threefold.
- **`Cache-Control: no-cache`.** The service worker versions files itself; `immutable` would pin old ones.
- **Large files.** The `.wasm` or a container is often 100 to 300 MB. Cloudflare Pages caps a file at 25 MiB.

Recommendation: **Cloudflare R2 behind a Worker** (the planner's `r2worker` host; the engine ships the Worker). No
per-file limit, no charge for downloads (the thing a free game with a few-hundred-megabyte first load would otherwise
pay for), and it is the platform VISION.md already picks for multiplayer rooms. Each build is uploaded under its own
prefix and switched on by changing one variable; rollback is the same switch.

For a first public test: **itch.io** as an HTML game with **Edit game > Embed options > SharedArrayBuffer support**
ticked. Per-file limit 200 MB, 1,000 files in the zip.

Publishing is a person's step: the engine's tools only write a plan, and they check it for leaked paths, names and
keys first. After upload, `webdoctor.py <url>` verifies the live host read-only.

## What will need work for this game on the web

- **Download size.** The levels use 8K Megascans textures and scanned trees. The web build needs its own texture
  size caps, or the first load will be far beyond what people wait for.
- **The look.** No Nanite means fallback meshes for cliffs and trees; no ray tracing or MegaLights means the lantern
  court's many lights and the reflections need a second look; the ocean, clouds and custom materials have to be
  checked tier by tier.
- **Controls.** Mouse look needs pointer lock (one click to capture the mouse); phones need touch controls, which
  do not exist yet.
- **Multiplayer.** The web target has no networking in this preview.
