/*
 * Copyright (C) 2026 Carlos Lopez
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "../../../utils/mcp/types.hpp"

namespace renodx::addons::devkit::mcp::tool_catalog {
inline renodx::utils::mcp::InputSchemaProperty MakeFlexibleHandleProperty() {
  return renodx::utils::mcp::InputSchemaProperty{
      .types = {"string"},
  };
}

inline renodx::utils::mcp::InputSchemaProperty MakeFlexibleUploadPathsProperty() {
  return renodx::utils::mcp::InputSchemaProperty{
      .types = {"array"},
      .item_types = {"string"},
      .item_enum_values = {
          "create_resource",
          "update_texture_region",
          "copy_buffer_to_texture",
      },
  };
}

inline renodx::utils::mcp::InputSchemaProperty MakeTextureReplaceRulesProperty() {
  using InputSchemaProperty = renodx::utils::mcp::InputSchemaProperty;

  return InputSchemaProperty{
      .types = {"array"},
      .item_types = {"object"},
      .item_properties = {
          {"name", {.types = {"string"}}},
          {"enabled", {.types = {"boolean"}}},
          {"uploadPaths",
           {
               MakeFlexibleUploadPathsProperty(),
           }},
          {"subresource", {.types = {"integer"}, .minimum = 0}},
          {"requireFullUpdate", {.types = {"boolean"}}},
          {"requireUploadHeapSource", {.types = {"boolean"}}},
          {"format", {.types = {"integer"}, .minimum = 0}},
          {"usageInclude", {.types = {"integer"}, .minimum = 0}},
          {"usageExclude", {.types = {"integer"}, .minimum = 0}},
          {"width", {.types = {"integer"}}},
          {"height", {.types = {"integer"}}},
          {"depthOrLayers", {.types = {"integer"}}},
          {"allowDynamic", {.types = {"boolean"}}},
          {"allowMultisampled", {.types = {"boolean"}}},
      },
  };
}

inline const renodx::utils::mcp::ToolCatalog METADATA = {
    {
        "devkit_status",
        {
            .title = "Devkit Status",
            .description = "Summarize tracked devices, selected device state, hot-path tracking mode (Active/Idle), and snapshot activity.",
            .annotations = {
                .read_only_hint = true,
            },
        },
    },
    {
        "devkit_set_tracking_active",
        {
            .title = "Set Tracking Active",
            .description = "Switch DevKit hot-path hooks between Active (full tracking) and Idle (low-overhead) mode on the next present.",
            .input_schema = {
                .properties = {
                    {"active", {.types = {"boolean"}}},
                },
                .required = {"active"},
            },
        },
    },
    {
        "devkit_get_swapchain_info",
        {
            .title = "Get Swapchain Info",
            .description = "Return graphics API, swapchain backbuffer format/dimensions/handles, color space, flip/present flags, HDR signaling metadata, and device proxy state for a device.",
            .input_schema = {
                .properties = {
                    {"deviceIndex", {.types = {"integer"}, .minimum = 0}},
                },
            },
            .annotations = {
                .read_only_hint = true,
            },
        },
    },
    {
        "devkit_list_resources",
        {
            .title = "List Resources",
            .description = "List tracked GPU resources for a device, with optional filters for resource type, format, minimum dimensions, backbuffer status, and snapshot RTV/UAV/SRV usage.",
            .input_schema = {
                .properties = {
                    {"deviceIndex", {.types = {"integer"}, .minimum = 0}},
                    {"type", {.types = {"string"}}},
                    {"format", {.types = {"string"}}},
                    {"minWidth", {.types = {"integer"}, .minimum = 0}},
                    {"minHeight", {.types = {"integer"}, .minimum = 0}},
                    {"seenRtv", {.types = {"boolean"}}},
                    {"seenUav", {.types = {"boolean"}}},
                    {"seenSrv", {.types = {"boolean"}}},
                    {"isBackbuffer", {.types = {"boolean"}}},
                    {"limit", {.types = {"integer"}, .minimum = 1, .maximum = 5000}},
                    {"offset", {.types = {"integer"}, .minimum = 0}},
                },
            },
            .annotations = {
                .read_only_hint = true,
            },
        },
    },
    {
        "devkit_set_shader_bypass",
        {
            .title = "Set Shader Bypass",
            .description = "Enable or disable draw/dispatch bypass for a tracked shader hash to isolate UI, bloom, vignette, or tonemap passes live.",
            .input_schema = {
                .properties = {
                    {"deviceIndex", {.types = {"integer"}, .minimum = 0}},
                    {"shaderHash", MakeFlexibleHandleProperty()},
                    {"bypass", {.types = {"boolean"}}},
                },
                .required = {"shaderHash", "bypass"},
            },
        },
    },
    {
        "devkit_select_device",
        {
            .title = "Select Device",
            .description = "Change the currently selected devkit device by index.",
            .input_schema = {
                .properties = {
                    {"deviceIndex", {.types = {"integer"}, .minimum = 0}},
                },
                .required = {"deviceIndex"},
            },
        },
    },
    {
        "devkit_list_shaders",
        {
            .title = "List Shaders",
            .description = "List tracked shaders for a device, optionally filtered by stage or active draw bypass.",
            .input_schema = {
                .properties = {
                    {"deviceIndex", {.types = {"integer"}, .minimum = 0}},
                    {"stage", {.types = {"string"}}},
                    {"bypassedOnly", {.types = {"boolean"}}},
                    {"limit", {.types = {"integer"}, .minimum = 1, .maximum = 5000}},
                    {"offset", {.types = {"integer"}, .minimum = 0}},
                },
            },
            .annotations = {
                .read_only_hint = true,
            },
        },
    },
    {
        "devkit_get_shader",
        {
            .title = "Get Shader",
            .description = "Return metadata for a tracked shader and optionally include disassembly/decompilation text (with line paging) and snapshot binding context (tN/uN/rtvN/cbN). DXBC uses configured cmd_Decompiler.exe; DXIL uses the internal DXC decompiler.",
            .input_schema = {
                .properties = {
                    {"deviceIndex", {.types = {"integer"}, .minimum = 0}},
                    {"shaderHash", MakeFlexibleHandleProperty()},
                    {"view", {.types = {"string"}, .enum_values = {"summary", "disassembly", "decompilation", "all"}}},
                    {"views", {.types = {"array"}, .item_types = {"string"}, .item_enum_values = {"disassembly", "decompilation"}}},
                    {"maxTextLength", {.types = {"integer"}, .minimum = 1}},
                    {"startLine", {.types = {"integer"}, .minimum = 1}},
                    {"lineCount", {.types = {"integer"}, .minimum = 1}},
                    {"includeSnapshotContext", {.types = {"boolean"}}},
                },
                .required = {"shaderHash"},
            },
            .annotations = {
                .read_only_hint = true,
            },
        },
    },
    {
        "devkit_dump_shader",
        {
            .title = "Dump Shader",
            .description = "Write a tracked shader's original bytecode to disk for external decompilers or archiving.",
            .input_schema = {
                .properties = {
                    {"deviceIndex", {.types = {"integer"}, .minimum = 0}},
                    {"shaderHash", MakeFlexibleHandleProperty()},
                    {"outputPath", {.types = {"string"}}},
                },
                .required = {"shaderHash"},
            },
        },
    },
    {
        "devkit_set_tools_path",
        {
            .title = "Set Tools Path",
            .description = "Query or change the directory used for DirectX and Vulkan shader tools from the repo bin directory.",
            .input_schema = {
                .properties = {
                    {"path", {.types = {"string"}}},
                },
            },
        },
    },
    {
        "devkit_set_live_shader_path",
        {
            .title = "Set Live Shader Path",
            .description = "Query or change the directory used by the devkit live shader compiler.",
            .input_schema = {
                .properties = {
                    {"path", {.types = {"string"}}},
                },
            },
        },
    },
    {
        "devkit_load_live_shaders",
        {
            .title = "Load Live Shaders",
            .description = "Compile and activate shader replacements from the configured live shader directory for the selected device.",
            .input_schema = {
                .properties = {
                    {"deviceIndex", {.types = {"integer"}, .minimum = 0}},
                },
            },
        },
    },
    {
        "devkit_unload_live_shaders",
        {
            .title = "Unload Live Shaders",
            .description = "Remove active live shader replacements from the selected device.",
            .input_schema = {
                .properties = {
                    {"deviceIndex", {.types = {"integer"}, .minimum = 0}},
                },
            },
        },
    },
    {
        "devkit_get_live_shader_status",
        {
            .title = "Get Live Shader Status",
            .description = "Return live shader directory, active runtime replacement count, per-shader compilation status and exact compiler error messages, and snapshot draw counts proving whether each replacement executed.",
            .input_schema = {
                .properties = {
                    {"deviceIndex", {.types = {"integer"}, .minimum = 0}},
                },
            },
            .annotations = {
                .read_only_hint = true,
            },
        },
    },
    {
        "devkit_list_draws",
        {
            .title = "List Draws",
            .description = "List draw records from the selected device's current snapshot cache, with optional filters for method, shader hash, stage, blend/backbuffer/fullscreen flags, and binding/render-target counts.",
            .input_schema = {
                .properties = {
                    {"deviceIndex", {.types = {"integer"}, .minimum = 0}},
                    {"method", {.types = {"string"}}},
                    {"shaderHash", MakeFlexibleHandleProperty()},
                    {"activeShaderStage", {.types = {"string"}}},
                    {"writesBackbuffer", {.types = {"boolean"}}},
                    {"isFullscreenPass", {.types = {"boolean"}}},
                    {"blendEnabled", {.types = {"boolean"}}},
                    {"minSrvCount", {.types = {"integer"}, .minimum = 0}},
                    {"minUavCount", {.types = {"integer"}, .minimum = 0}},
                    {"minConstantCount", {.types = {"integer"}, .minimum = 0}},
                    {"minRenderTargetCount", {.types = {"integer"}, .minimum = 0}},
                    {"minPipelineCount", {.types = {"integer"}, .minimum = 0}},
                    {"limit", {.types = {"integer"}, .minimum = 1, .maximum = 1000}},
                    {"offset", {.types = {"integer"}, .minimum = 0}},
                },
            },
            .annotations = {
                .read_only_hint = true,
            },
        },
    },
    {
        "devkit_get_draw",
        {
            .title = "Get Draw",
            .description = "Return full details for a single captured draw, including bindings and blend state.",
            .input_schema = {
                .properties = {
                    {"deviceIndex", {.types = {"integer"}, .minimum = 0}},
                    {"drawIndex", {.types = {"integer"}, .minimum = 0}},
                },
                .required = {"drawIndex"},
            },
            .annotations = {
                .read_only_hint = true,
            },
        },
    },
    {
        "devkit_get_draw_constants",
        {
            .title = "Get Draw Constants",
            .description = "Read bound constant buffers (via snapshot cache or live GPU staging buffer readback) and captured push constants for a draw, formatted as float4/uint32 registers. Optionally save under saveAs for diffing.",
            .input_schema = {
                .properties = {
                    {"deviceIndex", {.types = {"integer"}, .minimum = 0}},
                    {"drawIndex", {.types = {"integer"}, .minimum = 0}},
                    {"slot", {.types = {"integer"}, .minimum = 0}},
                    {"space", {.types = {"integer"}, .minimum = 0}},
                    {"maxBytes", {.types = {"integer"}, .minimum = 16, .maximum = 65536}},
                    {"saveAs", {.types = {"string"}}},
                },
                .required = {"drawIndex"},
            },
            .annotations = {
                .read_only_hint = true,
            },
        },
    },
    {
        "devkit_diff_draw_constants",
        {
            .title = "Diff Draw Constants",
            .description = "Compare a previously saved constant baseline (from devkit_get_draw_constants with saveAs) against another saved baseline or a fresh live read of drawIndex, pinpointing exact changed CBV/push-constant offsets (e.g. after moving an in-game HDR or gamma slider).",
            .input_schema = {
                .properties = {
                    {"deviceIndex", {.types = {"integer"}, .minimum = 0}},
                    {"baselineName", {.types = {"string"}}},
                    {"compareName", {.types = {"string"}}},
                    {"drawIndex", {.types = {"integer"}, .minimum = 0}},
                    {"epsilon", {.types = {"number"}}},
                    {"maxBytes", {.types = {"integer"}, .minimum = 16, .maximum = 65536}},
                },
                .required = {"baselineName"},
            },
            .annotations = {
                .read_only_hint = true,
            },
        },
    },
    {
        "devkit_compare_resource_ab",
        {
            .title = "Compare Resource A/B (Live Shaders vs Vanilla)",
            .description = "Automatically read back a resource with live shaders active, suspend live shaders for 1 frame to read back the vanilla output, restore live shaders, and return exact deltas for luminance, p99/p99.9, HDR (>1.0), WCG (<0.0), NaNs, and gamut coverage.",
            .input_schema = {
                .properties = {
                    {"deviceIndex", {.types = {"integer"}, .minimum = 0}},
                    {"resourceViewHandle", MakeFlexibleHandleProperty()},
                    {"resourceHandle", MakeFlexibleHandleProperty()},
                    {"preferClone", {.types = {"boolean"}}},
                },
            },
        },
    },
    {
        "devkit_analyze_resource",
        {
            .title = "Analyze Resource",
            .description = "Copy a tracked 2D or 3D texture resource view (or its clone when requested) into a temporary readback texture, summarize its contents (min/max/mean, p99/p99.9 percentiles, gamut breakdown, NaN/Inf/HDR/WCG counts), and optionally save a PNG preview or RGBA16F EXR dump (3D textures are unwrapped into a horizontal strip).",
            .input_schema = {
                .properties = {
                    {"deviceIndex", {.types = {"integer"}, .minimum = 0}},
                    {"resourceViewHandle", MakeFlexibleHandleProperty()},
                    {"outputPath", {.types = {"string"}}},
                    {"previewMode", {.types = {"string"}, .enum_values = {"clamped", "neutwo"}}},
                    {"preferClone", {.types = {"boolean"}}},
                },
                .required = {"resourceViewHandle"},
            },
            .annotations = {
                .read_only_hint = true,
            },
        },
    },
    {
        "devkit_analyze_lut",
        {
            .title = "Analyze 3D Texture / LUT",
            .description = "Read back a tracked 3D texture (texture_3d) or 2D strip LUT (e.g. 256x16, 1024x32), analyze value range, SDR/HDR clipping, neutral axis curve (black lift, white point, midgray, monotonicity, baked tonemap heuristic), identity deviation, primary/secondary hue & saturation shifts, estimated input space, and optionally export .cube, .json, or unwrapped .png/.exr.",
            .input_schema = {
                .properties = {
                    {"deviceIndex", {.types = {"integer"}, .minimum = 0}},
                    {"resourceViewHandle", MakeFlexibleHandleProperty()},
                    {"resourceHandle", MakeFlexibleHandleProperty()},
                    {"preferClone", {.types = {"boolean"}}},
                    {"layout", {.types = {"string"}, .enum_values = {"auto", "texture_3d", "strip_2d_horizontal", "strip_2d_vertical"}}},
                    {"lutSize", {.types = {"integer"}, .minimum = 2, .maximum = 128}},
                    {"exportCubePath", {.types = {"string"}}},
                    {"exportJsonPath", {.types = {"string"}}},
                    {"exportPreviewPath", {.types = {"string"}}},
                },
            },
            .annotations = {
                .read_only_hint = true,
            },
        },
    },
    {
        "devkit_dump_resource_with_hash",
        {
            .title = "Dump Resource With Hash",
            .description = "Read back a tracked resource view, compute a raw-byte CRC32, prefer a matching tracked upload CRC when available, and dump a hash-named PNG for replacement workflows.",
            .input_schema = {
                .properties = {
                    {"deviceIndex", {.types = {"integer"}, .minimum = 0}},
                    {"resourceViewHandle", MakeFlexibleHandleProperty()},
                    {"outputDirectory", {.types = {"string"}}},
                    {"namePrefix", {.types = {"string"}}},
                    {"preferClone", {.types = {"boolean"}}},
                },
                .required = {"resourceViewHandle", "outputDirectory"},
            },
        },
    },
    {
        "devkit_set_resource_clone",
        {
            .title = "Set Resource Clone",
            .description = "Enable or disable devkit clone hotswap on a tracked texture resource for live comparison.",
            .input_schema = {
                .properties = {
                    {"deviceIndex", {.types = {"integer"}, .minimum = 0}},
                    {"resourceHandle", MakeFlexibleHandleProperty()},
                    {"enabled", {.types = {"boolean"}}},
                },
                .required = {"resourceHandle", "enabled"},
            },
        },
    },
    {
        "devkit_replace_resource_with_file",
        {
            .title = "Replace Resource With File",
            .description = "Enable clone hotswap for a tracked resource or view, create a clone if needed, and upload a PNG into that clone for immediate live replacement.",
            .input_schema = {
                .properties = {
                    {"deviceIndex", {.types = {"integer"}, .minimum = 0}},
                    {"resourceViewHandle", MakeFlexibleHandleProperty()},
                    {"resourceHandle", MakeFlexibleHandleProperty()},
                    {"path", {.types = {"string"}}},
                },
                .required = {"path"},
            },
        },
    },
    {
        "devkit_set_texture_replace_enabled",
        {
            .title = "Set Texture Replace Enabled",
            .description = "Enable or disable devkit boot/create texture replacement interception globally.",
            .input_schema = {
                .properties = {
                    {"enabled", {.types = {"boolean"}}},
                },
                .required = {"enabled"},
            },
        },
    },
    {
        "devkit_reload_texture_replace_boot_cache",
        {
            .title = "Reload Texture Replace Boot Cache",
            .description = "Reload pre-decoded PNG replacements from the devkit boot directory for future create_resource initial-data replacements.",
        },
    },
    {
        "devkit_set_texture_replace_rules",
        {
            .title = "Set Texture Replace Rules",
            .description = "Replace the texture-replacement rule list for a device. Rule fields are descriptor/path filters used before any replacement provider is invoked.",
            .input_schema = {
                .properties = {
                    {"deviceIndex", {.types = {"integer"}, .minimum = 0}},
                    {"rules", MakeTextureReplaceRulesProperty()},
                },
                .required = {"rules"},
            },
        },
    },
    {
        "devkit_get_texture_replace_state",
        {
            .title = "Get Texture Replace State",
            .description = "Return boot/create texture-replacement enable state, provider state, active rules, observation count, and observed upload stats for a device.",
            .input_schema = {
                .properties = {
                    {"deviceIndex", {.types = {"integer"}, .minimum = 0}},
                },
            },
            .annotations = {
                .read_only_hint = true,
            },
        },
    },
    {
        "devkit_list_texture_replace_observations",
        {
            .title = "List Texture Replace Observations",
            .description = "List identified texture upload observations with CRC, format, dimensions, upload path, and hit counters.",
            .input_schema = {
                .properties = {
                    {"deviceIndex", {.types = {"integer"}, .minimum = 0}},
                    {"limit", {.types = {"integer"}, .minimum = 1, .maximum = 5000}},
                    {"offset", {.types = {"integer"}, .minimum = 0}},
                },
            },
            .annotations = {
                .read_only_hint = true,
            },
        },
    },
    {
        "devkit_clear_texture_replace_observations",
        {
            .title = "Clear Texture Replace Observations",
            .description = "Clear captured texture upload observations for a device and reset observation hit counters.",
            .input_schema = {
                .properties = {
                    {"deviceIndex", {.types = {"integer"}, .minimum = 0}},
                },
            },
        },
    },
    {
        "devkit_dump_texture_replace_observation",
        {
            .title = "Dump Texture Replace Observation",
            .description = "Dump captured bytes for an identified texture observation to disk for offline editing. Use .png for RGBA8/BGRA8 families, .bin for raw bytes, or pass the boot directory to use the canonical boot/create replacement filename.",
            .input_schema = {
                .properties = {
                    {"deviceIndex", {.types = {"integer"}, .minimum = 0}},
                    {"observationIndex", {.types = {"integer"}, .minimum = 0}},
                    {"outputPath", {.types = {"string"}}},
                },
                .required = {"observationIndex", "outputPath"},
            },
        },
    },
    {
        "devkit_snapshot_summary",
        {
            .title = "Snapshot Summary",
            .description = "Summarize cached snapshot counts for a device.",
            .input_schema = {
                .properties = {
                    {"deviceIndex", {.types = {"integer"}, .minimum = 0}},
                },
            },
            .annotations = {
                .read_only_hint = true,
            },
        },
    },
    {
        "devkit_queue_snapshot",
        {
            .title = "Queue Snapshot",
            .description = "Queue a new snapshot capture for a device on the next present. When returnToIdle is true (default when currently Idle), automatically returns DevKit to Idle mode after capture completes.",
            .input_schema = {
                .properties = {
                    {"deviceIndex", {.types = {"integer"}, .minimum = 0}},
                    {"returnToIdle", {.types = {"boolean"}}},
                },
            },
        },
    },
    {
        "devkit_trace_output_chain",
        {
            .title = "Trace Output Chain",
            .description = "Walk backwards from the swapchain backbuffer(s) through snapshot RTV/UAV/Copy to SRV dependencies to identify the post-processing chain, UI/HUD boundary, and UNORM/R11G11B10F/RGB10A2 format bottlenecks.",
            .input_schema = {
                .properties = {
                    {"deviceIndex", {.types = {"integer"}, .minimum = 0}},
                    {"maxDepth", {.types = {"integer"}, .minimum = 1, .maximum = 256}},
                    {"includeUiDraws", {.types = {"boolean"}}},
                },
            },
            .annotations = {
                .read_only_hint = true,
            },
        },
    },
    {
        "devkit_classify_postprocess",
        {
            .title = "Classify Post-Process Chain",
            .description = "Heuristically classify passes in the traced output chain (LUT/color grade, tonemap candidate, bloom/blur pyramid, sharpen/final output, HDR10/scRGB/SDR swapchain write, copy/resolve, UI/HUD) and surface detected 3D/2D LUT handles and upgrade recommendations.",
            .input_schema = {
                .properties = {
                    {"deviceIndex", {.types = {"integer"}, .minimum = 0}},
                    {"maxDepth", {.types = {"integer"}, .minimum = 1, .maximum = 256}},
                },
            },
            .annotations = {
                .read_only_hint = true,
            },
        },
    },
    {
        "devkit_scan_pass_resources",
        {
            .title = "Scan Pass Resources",
            .description = "Perform a single-frame batch GPU readback across multiple 2D texture resources (explicit handles, draw indices, or the traced output chain) and report min/max/mean/p99/p99.9, HDR (>1.0), negative WCG (<0.0), NaN/Inf, gamut breakdown (BT.709/DCI-P3/BT.2020), and HDR drop-off summary.",
            .input_schema = {
                .properties = {
                    {"deviceIndex", {.types = {"integer"}, .minimum = 0}},
                    {"resourceViewHandles", {.types = {"array"}, .item_types = {"string"}}},
                    {"resourceHandles", {.types = {"array"}, .item_types = {"string"}}},
                    {"drawIndices", {.types = {"array"}, .item_types = {"integer"}}},
                    {"includeOutputChain", {.types = {"boolean"}}},
                    {"maxResources", {.types = {"integer"}, .minimum = 1, .maximum = 32}},
                    {"preferClone", {.types = {"boolean"}}},
                },
            },
            .annotations = {
                .read_only_hint = true,
            },
        },
    },
    {
        "devkit_compare_snapshots",
        {
            .title = "Compare Snapshots",
            .description = "Save, list, or compare snapshot profiles (e.g. HDR ON vs OFF, CAS ON vs OFF, menu vs gameplay) to detect added/removed shaders, draw-count shifts, swapchain changes, and output-chain differences.",
            .input_schema = {
                .properties = {
                    {"action", {.types = {"string"}, .enum_values = {"save", "list", "compare"}}},
                    {"deviceIndex", {.types = {"integer"}, .minimum = 0}},
                    {"name", {.types = {"string"}}},
                    {"baselineName", {.types = {"string"}}},
                    {"targetName", {.types = {"string"}}},
                },
                .required = {"action"},
            },
        },
    },
    {
        "devkit_queue_trace",
        {
            .title = "Queue 1-Frame API Trace",
            .description = "Queue a 1-frame graphics API trace into DevKit's in-memory trace buffer (and ReShade.log). Supports returnToIdle (default true when currently Idle) and readback via devkit_get_trace_log.",
            .input_schema = {
                .properties = {
                    {"deviceIndex", {.types = {"integer"}, .minimum = 0}},
                    {"returnToIdle", {.types = {"boolean"}}},
                },
            },
        },
    },
    {
        "devkit_get_trace_log",
        {
            .title = "Get Trace Log",
            .description = "Retrieve lines from the most recent in-memory 1-frame API trace captured by devkit_queue_trace, with optional case-insensitive substring filtering and pagination.",
            .input_schema = {
                .properties = {
                    {"filter", {.types = {"string"}}},
                    {"limit", {.types = {"integer"}, .minimum = 1, .maximum = 5000}},
                    {"offset", {.types = {"integer"}, .minimum = 0}},
                },
            },
            .annotations = {
                .read_only_hint = true,
            },
        },
    },
    {
        "devkit_get_device_proxy_state",
        {
            .title = "Get Device Proxy State",
            .description = "Return full SwapChain Device Proxy state (mode, outputMode sRGB/HDR10/scRGB, HWND route, flip/D3D9Ex compatibility, runtime readiness, and custom FXC shader compilation status).",
            .input_schema = {
                .properties = {
                    {"deviceIndex", {.types = {"integer"}, .minimum = 0}},
                },
            },
            .annotations = {
                .read_only_hint = true,
            },
        },
    },
    {
        "devkit_set_device_proxy_mode",
        {
            .title = "Set Device Proxy Mode",
            .description = "Configure SwapChain Device Proxy mode (none, current_window, new_window), output mode (srgb, hdr10, scrgb), proxyWaitIdle, custom FXC proxy shaders, and D3D9Ex/flip boot compatibility flags safely on the present thread.",
            .input_schema = {
                .properties = {
                    {"deviceIndex", {.types = {"integer"}, .minimum = 0}},
                    {"mode", {.types = {"string"}, .enum_values = {"none", "current_window", "new_window"}}},
                    {"outputMode", {.types = {"string"}, .enum_values = {"srgb", "hdr10", "scrgb"}}},
                    {"proxyWaitIdle", {.types = {"boolean"}}},
                    {"useCustomShaders", {.types = {"boolean"}}},
                    {"compileCustomShaders", {.types = {"boolean"}}},
                    {"forceDx9Ex", {.types = {"boolean"}}},
                    {"forceDisableFlip", {.types = {"boolean"}}},
                },
            },
        },
    },
    {
        "devkit_check_upgrade_candidates",
        {
            .title = "Check Resource Upgrade Candidates",
            .description = "Analyze RGBA8/BGRA8/R11G11B10F/RGB10A2 targets in the post-processing chain (or full snapshot), inspect usage/MSAA/mips/array/aspect/copy-mismatch/view-aliasing risks, check blast radius against tracked resources, and generate ready-to-paste ResourceUpgradeInfo C++ snippets for addon.cpp.",
            .input_schema = {
                .properties = {
                    {"deviceIndex", {.types = {"integer"}, .minimum = 0}},
                    {"maxDepth", {.types = {"integer"}, .minimum = 1, .maximum = 256}},
                    {"includeAllSnapshotTargets", {.types = {"boolean"}}},
                    {"includeBackbuffers", {.types = {"boolean"}}},
                },
            },
            .annotations = {
                .read_only_hint = true,
            },
        },
    },
};

}  // namespace renodx::addons::devkit::mcp::tool_catalog
