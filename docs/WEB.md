# Putting Gravity Golf on the web

Status on 2026-11-19: **it runs in a browser on this Mac. Nothing is published.** Ken's instruction is to test
locally for now.

```bash
Tools/web.sh Development     # package (about 2 minutes once the engine is built)
Tools/webtest.sh             # run it in Chrome: desktop play and phone touch, with screenshots
Tools/webtest.sh tour        # every hole: open it, picture from the tee, play its recorded solution
../gravitygolf-web/Saved/Web/play-gravitygolf.sh    # serve it and play it yourself in your browser
```

What the browser runs showed (Chrome 156, M4 Max, the engine's harness):

| | |
| --- | --- |
| Ready to play | 4 to 6 seconds after opening the page, on a fast local link |
| First download | about 68 MB before the game is ready; about 100 MB in all (42 MB of code, 54 MB of content) |
| Memory | 512 MB heap at peak |
| Renderer | the mobile deferred tier, no WebGPU errors |
| Works | title screen, menus, walking, carrying machines, the swing, the physics (recorded solutions sink the ball), touch stick and buttons under phone emulation (852x393) |

What is different from the desktop game on the web:
- No bounced light, no ray tracing, shadows from cascaded shadow maps. Clear sky (a dome), no volumetric clouds.
- Scanned plants are replaced by simple generated ones (`SM_Web_Pine`, `SM_Web_Shrub`).
- World textures are capped at 1K.

Not done: a real phone (emulation only so far; a phone on the LAN needs HTTPS, see the engine's `serve.py --tls-cert`),
Safari and Firefox, frame-rate measurement, a Shipping build, saved progress across page reloads (untested).

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

## Why the engine is built from source

1. The launcher install has `Engine/Platforms/Wasm` (the platform's source and settings).
2. Asked the build tool for a web build: **"The Wasm platform is not supported from this engine distribution."**
3. Looked for why. The Mac launcher build ships only Mac libraries: none of the third-party libraries built for
   Wasm, no page template (`Platforms/Wasm/Build/Template`) and no web tools (`Build/Tools`: the local server and the
   deploy planner). Epic lists this as a known issue of the preview: "The premade builds on Mac and Linux are missing
   part of the Wasm (Web) support files", workaround a source build, with a fix build promised.
4. Installed the Emscripten SDK 6.0.10 at `~/emsdk` (the version the engine pins), which any route needs.
5. The installed editor will not even list the web platform for cooking until it finds that SDK
   (`WASM_EMSDK_ROOT`).
6. This Mac has no Rosetta, and building the engine's C# build tools needs one Intel-only helper (`protoc`). The
   source tree uses the launcher install's prebuilt copies of those tools instead (same engine version), which is why
   `Tools/web.sh` passes `-nocompileuat -nocompile`.

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

## What still needs work for this game on the web

- **A real phone.** Touch works under emulation. Nobody has held it.
- **Frame rate** on ordinary laptops and phones, unmeasured.
- **The look.** No clouds, flat ambient light, stand-in plants. It is clean, not beautiful.
- **Controls.** Mouse look needs one click to capture the mouse (the page waits for it).
- **Multiplayer.** The web target has no networking in this preview.
