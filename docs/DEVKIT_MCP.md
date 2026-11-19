# RenoDX Devkit MCP

This guide covers the current RenoDX devkit and MCP workflow for inspecting a running game, dumping shaders, iterating on replacements, and validating HDR resource behavior.

See also:

- `docs/CONTRIBUTING.md`

## 1. What the pieces are

The live inspection stack has three parts:

1. the game-side `devkit` addon
2. the `renodx-mcp-bridge` process
3. an MCP client such as Codex or another JSON-RPC MCP host

The game addon exposes frame, shader, and resource inspection over a named pipe. The bridge exposes that pipe as an MCP tool server.

## 2. What to build

For live inspection and MCP-driven modding, build:

```powershell
cmake --preset clang-x64
cmake --build --preset clang-x64-debug --target devkit mcp_bridge
```

The debug artifacts are:

- `build/Debug/renodx-mcp-bridge.exe`
- `build/Debug/renodx-devkit.addon64`

The bridge advertises the DevKit tool surface up front. If there is exactly one live backend pipe, it auto-connects on first use. Otherwise use the bridge tools first:

- `renodx_list_connections`
- `renodx_connect`

## 3. Tool groups

The exact MCP schema can evolve, but the current workflow is built around these groups.

### Connection, tracking mode, and frame state

- `renodx_list_connections`
- `renodx_connect`
- `devkit_status` (includes `tracking` Active/Idle state)
- `devkit_set_tracking_active` (switch hot-path hooks between Active and Idle/low-overhead mode)
- `devkit_select_device`
- `devkit_get_swapchain_info` (API, format, color space, HDR signaling, backbuffer handles, proxy state)
- `devkit_get_device_proxy_state` (full SwapChain Device Proxy state: mode, outputMode sRGB/HDR10/scRGB, HWND route, flip/D3D9Ex compatibility, custom FXC shader status)
- `devkit_set_device_proxy_mode` (configure SwapChain Device Proxy mode, outputMode sRGB/HDR10/scRGB, proxyWaitIdle, custom FXC shader compilation/toggle, and D3D9Ex/flip boot compatibility flags safely on the present thread)
- `devkit_queue_snapshot` (supports `returnToIdle` for one-shot capture from Idle mode)
- `devkit_snapshot_summary`
- `devkit_compare_snapshots` (`action`: `"save"`, `"list"`, `"compare"` — diff two snapshot profiles for added/removed shaders, draw-count shifts, target format changes, swapchain changes, and output-chain changes)
- `devkit_queue_trace` (queue a 1-frame API trace into the in-memory trace buffer and `ReShade.log`, with `returnToIdle` support)
- `devkit_get_trace_log` (read, filter, and page captured in-memory trace log lines)

### Frame & pipeline inspection

- `devkit_trace_output_chain` (walk backwards from backbuffer through RTV/UAV/Copy → SRV links; detects UI boundary and format bottlenecks)
- `devkit_classify_postprocess` (heuristically classifies passes in the output chain into tonemap/grade, LUT, bloom, sharpen, HDR10/scRGB/SDR output, copy/resolve, and surfaces detected LUT handles)
- `devkit_check_upgrade_candidates` (analyzes RGBA8/BGRA8/R11G11B10F/RGB10A2 targets in the output chain or full snapshot, checks usage/MSAA/mips/array/aspect/copy-mismatch/view-aliasing risks and tracked-resource blast radius, and generates ready-to-paste `ResourceUpgradeInfo` C++ snippets for `addon.cpp`)
- `devkit_list_shaders` (supports `stage` and `bypassedOnly` filters)
- `devkit_list_draws` (supports `writesBackbuffer`, `isFullscreenPass`, `blendEnabled`, stage, hash, and binding-count filters)
- `devkit_get_draw`
- `devkit_get_draw_constants` (reads bound CBVs/UBOs via snapshot cache or live GPU staging readback plus captured push constants formatted as `float4`/`uint32`, with optional `saveAs` baseline)
- `devkit_diff_draw_constants` (diffs a saved constants baseline against another baseline or a fresh live read of a draw to pinpoint changed `cbN[reg].xyzw` and push-constant offsets)
- `devkit_get_shader` (supports `view`/`views` disassembly & decompilation, `startLine`/`lineCount` line-range paging, and `includeSnapshotContext` binding table `tN`/`uN`/`rtvN`/`cbN`)
- `devkit_set_shader_bypass` (enable/disable live draw/dispatch bypass for a shader hash)

### Resource inspection, 3D LUT analysis & batch HDR scan

- `devkit_list_resources` (list tracked GPU resources with type, format, dimension, backbuffer, and snapshot RTV/UAV/SRV usage filters)
- `devkit_analyze_resource` (supports 2D and 3D textures; includes `percentiles` p99/p99.9 and `gamut` BT.709/DCI-P3/BT.2020 breakdown; unwraps 3D previews into horizontal strips)
- `devkit_analyze_lut` (reads back 3D textures `texture_3d` and 2D strip LUTs like `256x16` / `1024x32`; analyzes range/clipping, neutral axis curve, black lift, white point, baked-tonemap and input-space heuristics, primary/secondary hue & saturation shifts, and optionally exports `.cube`, `.json`, or unwrapped `.png`/`.exr`)
- `devkit_scan_pass_resources` (single-frame batch GPU readback across explicit handles, draw indices, or `includeOutputChain: true` with `hdrDropOff` summary)
- `devkit_compare_resource_ab` (automated 2-frame A/B GPU readback: Present 1 with live shader replacements active, Present 2 vanilla with live replacements temporarily suspended, returning luminance/p99/p99.9/HDR/WCG/NaN/gamut deltas)
- `devkit_dump_resource_with_hash`
- `devkit_set_resource_clone`
- `devkit_replace_resource_with_file`

### Texture replacement rules (experimental)

- `devkit_set_texture_replace_enabled`
- `devkit_reload_texture_replace_boot_cache`
- `devkit_set_texture_replace_rules`
- `devkit_get_texture_replace_state`
- `devkit_list_texture_replace_observations`
- `devkit_clear_texture_replace_observations`
- `devkit_dump_texture_replace_observation`

### Shader dump and live iteration

- `devkit_dump_shader`
- `devkit_set_tools_path`
- `devkit_set_live_shader_path`
- `devkit_load_live_shaders`
- `devkit_unload_live_shaders`
- `devkit_get_live_shader_status` (reports live shader directory, reload status, active runtime replacement count, per-shader compilation status & exact compiler error text, and `snapshotDrawCount`)

Default devkit folder layout under the ReShade base path:

- `renodx-dev/live`
- `renodx-dev/dump`
- `renodx-dev/boot`

`boot/` is scanned and decoded at devkit startup so matching `initial_data` uploads can be replaced immediately.
If boot assets are found, devkit texture replacement is enabled automatically.
Texture replacement lookup checks `live` first, then the decoded `boot` cache.

## 4. Recommended investigation loop

DevKit starts in **Idle (low-overhead) mode** by default (`StartActive=0`), keeping hot-path hooks (`draw`, `dispatch`, `bind_pipeline`, `state`, `trace`, `swapchain` render-pass hooks) unregistered during normal gameplay. `KeepDescriptorTrackingInIdle` defaults to `1` so static Vulkan/DX12 descriptor tables populated at boot remain tracked for `devkit_trace_output_chain` (can be disabled under Settings → `Other`). Calling `devkit_queue_snapshot` while Idle automatically activates tracking for the capture frame and returns to Idle right after (`returnToIdle` defaults to `true` when Idle). For full frame-0 tracking, enable **Start Active on Launch** (`StartActive=1` under `[renodx-dev]`) or call `devkit_set_tracking_active({ active: true })`.

For a new game, work backward from the end of the frame:

1. Connect, check `devkit_status`, and select the active device with `devkit_select_device`.
2. Inspect output format, color space, and backbuffers with `devkit_get_swapchain_info`.
3. Queue a snapshot (`devkit_queue_snapshot`) and wait for the next present.
4. Trace the post-processing pipeline backwards from the backbuffer with `devkit_trace_output_chain` to locate the UI/HUD boundary (`uiStartDrawIndex`) and UNORM/R11G11B10F/RGB10A2 format bottlenecks.
5. Run a batch HDR/Gamut/NaN scan over the chain with `devkit_scan_pass_resources({ includeOutputChain: true })` to find the exact `hdrDropOff` pass.
6. Use `devkit_get_draw`, `devkit_list_resources`, and `devkit_set_shader_bypass` to verify individual passes (e.g. isolating UI, bloom, vignette, or tonemap).
7. Only after the target pass is clear, dump and edit shaders.

This usually answers the important question first:

- is HDR already gone before the final blit
- or is the final blit still carrying recoverable scene data

Do not start from the first bright shader you find. Start from the latest pass that still has scene HDR potential.

## 5. Choosing the right device

Many games expose multiple D3D devices. Only one may be the active renderer.

Check `devkit_status` and `devkit_select_device` before analyzing anything:

- active swapchains
- pipeline counts
- tracked shaders
- captured draws

If one device shows `0` draws or `0` tracked shaders, it is usually not the one you want.

## 6. Reading frame structure

Start with `devkit_trace_output_chain` for an automatic backwards graph from the swapchain backbuffer(s), or use `devkit_list_draws` (filtering by `writesBackbuffer`, `isFullscreenPass`, `blendEnabled`, `shaderHash`, or `method`) and `devkit_list_resources` to find specific targets, then `devkit_get_draw` for the exact pass.

The useful fields are:

- pixel, compute, and vertex shader hashes
- `writesBackbuffer`, `isFullscreenPass`, and `blendEnabled` flags
- render target view and UAV details and formats
- SRV inputs and formats
- blend state
- swapchain and clone flags

For HDR work, the most important draw categories are:

- the last scene composite or tonemap pass
- later SDR UI and text passes
- the actual final copy or swapchain pass

If the final pass is only a one-texture blit from an already SDR target, it is not the place to recover HDR.

## 7. Shader inspection and dumping

`devkit_get_shader` returns metadata plus optional disassembly or decompilation text over MCP. It does not send raw shader bytecode by default.

If the game ships its own `dxcompiler.dll` or other conflicting shader tools, point `devkit_set_tools_path` at the repo `bin` directory first so devkit loads the repo toolchain instead of the game-local copy.

Use `devkit_dump_shader` when you want the original compiled shader on disk.

Important rule:

- dump original `.cso` files outside `src/games/**`

Do not place dumped `.cso` files in your live shader folder. If both a dumped binary and an editable HLSL replacement exist for the same hash, the binary can shadow the editable source.

Recommended layout:

- originals: `tmp/{game}/original/`
- editable live replacements: `src/games/{game}/`

For dumped DXBC shaders, the repo-standard external decompiler is:

- `bin/cmd_Decompiler.exe`

If it is missing, bootstrap the repo tools from the repo root before using the decompiler:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\setup-dev-env.ps1 -Install
```

That installs `cmd_Decompiler.exe` into `.\bin`. For the full tool bootstrap and the remaining optional reverse-engineering binaries, see `docs/CONTRIBUTING.md`.

Devkit MCP can already disassemble tracked DirectX shaders. Decompilation is best-effort: DXBC SM4/5 shaders use `cmd_Decompiler.exe` from the configured tools path, while DXIL SM6 shaders use the internal DXC decompiler path. The external tools in `.\bin` are still useful when you want standalone files on disk or alternate reverse-engineering workflows.

Example:

```powershell
bin\cmd_Decompiler.exe --decompile tmp\road96\original\0x1561F9BA.cso
```

## 8. Live shader iteration

The safe loop is:

1. dump original shader to `tmp`
2. decompile it when available, or fall back to disassembly / external tools
3. copy the editable HLSL into `src/games/{game}`
4. point `devkit_set_tools_path` at your repo `bin` folder if the game has conflicting DXC DLLs
5. point `devkit_set_live_shader_path` at that folder
6. call `devkit_load_live_shaders`
7. inspect compiler errors, active replacement count, and `snapshotDrawCount` via `devkit_get_live_shader_status` (or verify with `devkit_get_shader` that the source is `File`)
8. run `devkit_compare_resource_ab` on the target pass or backbuffer to measure exact live-shader vs. vanilla luminance, p99/p99.9, HDR/WCG, NaN, and gamut deltas in two consecutive frames

Keep the live shader folder clean:

- only editable HLSL replacements
- no dumped `.cso`
- no unrelated experiment artifacts

When editing a dumped shader:

- preserve the original register bindings
- keep the vanilla logic visible in comments if you are testing an HDR change
- do not silently delete the original behavior when you still need an SDR reference

## 9. Resource cloning and HDR validation

Resource cloning is often the fastest way to prove whether a scene path can carry HDR.

Important distinction:

- the original view may still be `r8g8b8a8_unorm_srgb`
- the clone can be `r16g16b16a16_float`

That means:

- the original view can look clipped
- the clone can still hold values above `1.0`

Use `devkit_set_resource_clone` to enable cloning, then analyze the same resource with:

- the original view
- `preferClone: true`

This is the authoritative way to answer whether the upgraded path is actually preserving HDR data.

## 10. Dump formats and batch HDR scanning

`devkit_analyze_resource` returns per-channel and luminance `min`/`max`/`mean`, **percentiles** (`luminanceP99`, `luminanceP999`, `maxRgbP99`, `maxRgbP999`), **gamut breakdown** (`bt709`, `dciP3Only`, `bt2020Only`, `outOfBt2020` assuming BT.709 linear), NaN/Inf/negative/>1.0 pixel counts, and can optionally write preview or HDR-preserving files:

- `.png`
  SDR preview only
- `.exr`
  HDR-preserving `RGBA16F` dump

For scanning multiple passes in a single frame without writing files, use `devkit_scan_pass_resources` with `includeOutputChain: true`, `drawIndices`, `resourceViewHandles`, or `resourceHandles`. It returns per-target stats plus an `hdrDropOff` summary (`lastPassWithValuesAbove1`, `firstClippedPassAfterHdr`, `lastPassWithNegativeValues`, `firstPassWithNanOrInf`).

Do not use the PNG alone to decide whether HDR survived. Use:

- the numeric analysis
- the EXR dump
- or the clone readback stats

The PNG is for quick visual inspection, not for HDR truth.

## 11. Current practical caveats

- Some clone and preview workflows may still behave better when the devkit overlay has already touched the resource in the current session. Treat that as a devkit limitation, not as a mod requirement.
- If a game stalls during live shader reload, rebuild the latest devkit. Live shader load and unload are designed to be queued and processed on present, not on the MCP thread.
- Shader decompilation is not guaranteed for every tracked DirectX shader. If `devkit_get_shader` reports the decompilation section as unavailable, use disassembly or a dumped shader with external tools instead.
- If an MCP client loses its transport, that does not necessarily mean the game-side devkit backend is gone. Reconnect the bridge and check `devkit_status`.

## 12. What to turn into a real mod

Devkit MCP is for narrowing the problem and proving a path. The shipping mod should then move the proven behavior into `src/games/{gamename}`:

- `addon.cpp` for resource upgrades and shader registration
- CRC-addressed shader replacements
- manual verification notes

Do not ship a mod that still depends on manual devkit clone toggles or the overlay being open.

The usual progression is:

1. prove the target pass with MCP
2. prove HDR survival with clone readback
3. convert the successful path into addon logic
4. validate the real non-devkit path

## 13. Client configuration examples

These examples assume you built the local debug bridge at:

- `C:\Mods\renodx\build\Debug\renodx-mcp-bridge.exe`

### Codex

Add this to `.codex/config.toml`:

```toml
[mcp_servers.renodx]
command = "C:\\Mods\\renodx\\build\\Debug\\renodx-mcp-bridge.exe"
args = []
```

Typical first prompt:

```text
Use the RenoDX MCP bridge. List the available DevKit connections, connect if needed, queue a snapshot on the active device, then show me the last 10 draws and the first 10 tracked shaders.
```

### Claude Desktop

Add this to `claude_desktop_config.json`:

```json
{
  "mcpServers": {
    "renodx": {
      "type": "stdio",
      "command": "C:\\Mods\\renodx\\build\\Debug\\renodx-mcp-bridge.exe",
      "args": []
    }
  }
}
```

### VS Code / Copilot Chat

Add this to `.vscode/mcp.json` in your workspace:

```json
{
  "servers": {
    "renodx": {
      "command": "C:\\Mods\\renodx\\build\\Debug\\renodx-mcp-bridge.exe",
      "args": []
    }
  }
}
```

Notes:

- VS Code can also keep the same MCP server config in user `settings.json` instead of a workspace `.vscode/mcp.json`.
- GitHub-hosted Copilot coding agent is not a good fit for this bridge. `renodx-mcp-bridge.exe` is a local Windows process that expects a running game and DevKit addon on the same machine.
