# HANDOFF: DevKit fixes + missing roadmap features (2026-11-19)

Goal: fix every DevKit bug found in the roadmap review and implement the missing features of
`docs/DEVKIT_MCP_ROADMAP.md`. Build, deploy, test live in WTTG3 (D3D11), then commit/push to `main`
**only after the user says so** (push straight to main, no branch; commit with
`git -c user.name="Lostpold88" -c user.email=<user email>`; stage only devkit files + `build/Release/renodx-devkit.addon64`,
never `src/games/detroit/*`).

Build: `cmake --build --preset clang-x64-release --target devkit` (CMakeLists changed: reconfigure with `cmake --preset clang-x64` first).
Deploy: copy `build/Release/renodx-devkit.addon64` to
`E:\SteamLibrary\steamapps\common\Welcome to the Game III\WTTGSD\Binaries\Win64\` (game must be closed).

## Done (uncommitted, all in working tree)
- Crash fix: `mcp/deep_analysis.hpp` live CB readback checks `GetLiveResourceInfo` before touching the buffer (UE frees CBs -> driver AV). Built + deployed + live tested.
- `KeepDescriptorTrackingInIdle` default true; decoder supports `r32g32b32a32_float`, `r16g16b16a16_unorm`.
- `TryGetTrackedResourceInfo` ignores destroyed entries; UI CB preview (`RenderConstantBufferViewPreview`) uses tracker instead of `get_resource_desc`.
- CB `size` UINT64_MAX -> null + `fullRange`; `skip_draw_count` atomic; `SetShaderDrawBypass()` shared by UI+MCP, bypass turns Idle tracking on and back off (`bypass_forced_tracking`); snapshot/trace returnToIdle no longer kills active bypass.
- `snapshotCaptureCount` / `snapshotCapturedAtMs` in status + snapshot_summary.
- CMake: `RENODX_BUILD_VERSION` (git describe) + timestamp for devkit and mcp_bridge.
- `devkit_status.tracking`: `hookSubsystems`, `bypassedShaderCount`.
- `get_swapchain_info`: per swapchain `display` (osHdrEnabled, luminance, primaries; refreshed every 2 s outside the device mutex) + `isFlipModel`; top-level `gameHdrMetadata` via MinHook hook on DXGI `SetHDRMetaData` (vtable index 40, `hdr_metadata_hook` namespace, uninstalled on detach).
- Viewport/element count: `OnBindViewports` hot-path hook -> `CommandListData.viewport` -> `DrawDetails.viewport/element_count`;
  `DrawSummary` gets `viewport`, `viewportCoverage`, `elementCount`; fullscreen = false when coverage < 0.95.
  Built.
- Output chain: `elementCount` + `additiveBlend` in `DrawSummary`/`DrawNode`/`ChainPassEntry`; UI boundary requires `element_count <= 6`.
- `classify_postprocess`: roles `pq_encode`, `sharpen_cas` (disassembly scan, depth <= 2), fullscreen additive -> bloom, grouped `ui_hud` entries.
- `analyze_lut`: `pq_shaper_to_sdr` / `log_shaper_to_sdr` (old `log_or_hdr_shaper_to_sdr` is gone).
- `get_live_shader_status`: `diagnostics[]`; `get_trace_log`: `entries` {index,event,args} (`structured=false` -> `lines`).
- `compare_snapshots`: `firstDivergenceIndex`, `outputChainAdded/Removed`. README/roadmap/tool_catalog updated.
- Built + deployed to the WTTG3 folder (2026-11-19 12:44). **Not live tested yet.**

## Next
1. Start WTTG3, queue a snapshot, live test: `classify_postprocess`, `analyze_lut`, `get_live_shader_status` (break a live shader to see diagnostics), `get_trace_log`, `compare_snapshots`, `get_draw` viewport/elementCount.
2. Ask the user to commit/push (stage only devkit files + docs + `build/Release/renodx-devkit.addon64`, never `src/games/detroit/*`).
