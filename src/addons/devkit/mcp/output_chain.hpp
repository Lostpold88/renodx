/*
 * Copyright (C) 2026 Carlos Lopez
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <format>
#include <functional>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../../../utils/mcp/arguments.hpp"
#include "../../../utils/mcp/types.hpp"

namespace renodx::addons::devkit::mcp::output_chain {

using json = renodx::utils::mcp::json;
using ToolResult = renodx::utils::mcp::ToolResult;
namespace mcp_arguments = renodx::utils::mcp::arguments;

struct ResourceEndpoint {
  std::uint64_t resource_handle = 0u;
  std::uint64_t resource_view_handle = 0u;
  std::string resource_handle_hex;
  std::string resource_view_handle_hex;
  std::string role;
  std::string type;
  std::string format;
  std::uint32_t width = 0u;
  std::uint32_t height = 0u;
  std::uint32_t depth_or_layers = 0u;
  bool is_backbuffer = false;
  bool is_hdr10_backbuffer = false;
  bool is_float = false;
  bool is_unorm = false;
  bool is_r11g11b10f = false;
  bool is_rgb10a2_unorm = false;
  bool supports_negative = false;
};

// NOLINTNEXTLINE(readability-identifier-naming)
inline void to_json(json& j, const ResourceEndpoint& value) {
  j = {
      {"resourceHandle", value.resource_handle_hex},
      {"resourceViewHandle", value.resource_view_handle == 0u ? json(nullptr) : json(value.resource_view_handle_hex)},
      {"role", value.role},
      {"type", value.type},
      {"format", value.format},
      {"width", value.width},
      {"height", value.height},
      {"depthOrLayers", value.depth_or_layers},
      {"isBackbuffer", value.is_backbuffer},
      {"isHdr10Backbuffer", value.is_hdr10_backbuffer},
      {"isFloat", value.is_float},
      {"isUnorm", value.is_unorm},
      {"isR11G11B10F", value.is_r11g11b10f},
      {"isRgb10A2Unorm", value.is_rgb10a2_unorm},
      {"supportsNegative", value.supports_negative},
  };
}

struct DrawNode {
  std::uint32_t draw_index = 0u;
  std::string method;
  std::optional<std::string> active_shader_stage = std::nullopt;
  std::vector<std::string> shader_hashes;
  bool blend_enabled = false;
  bool writes_backbuffer = false;
  bool is_fullscreen_pass = false;
  std::vector<ResourceEndpoint> inputs;
  std::vector<ResourceEndpoint> outputs;
};

struct ChainPassEntry {
  std::uint32_t draw_index = 0u;
  std::uint32_t depth_from_output = 0u;
  std::string method;
  std::optional<std::string> active_shader_stage = std::nullopt;
  std::vector<std::string> shader_hashes;
  bool blend_enabled = false;
  bool writes_backbuffer = false;
  bool is_fullscreen_pass = false;
  std::vector<std::uint32_t> upstream_draw_indices;
  std::vector<ResourceEndpoint> inputs;
  std::vector<ResourceEndpoint> outputs;
};

// NOLINTNEXTLINE(readability-identifier-naming)
inline void to_json(json& j, const ChainPassEntry& value) {
  j = {
      {"drawIndex", value.draw_index},
      {"depthFromOutput", value.depth_from_output},
      {"method", value.method},
      {"activeShaderStage", value.active_shader_stage.has_value() ? json(value.active_shader_stage.value()) : json(nullptr)},
      {"shaderHashes", value.shader_hashes},
      {"blendEnabled", value.blend_enabled},
      {"writesBackbuffer", value.writes_backbuffer},
      {"isFullscreenPass", value.is_fullscreen_pass},
      {"upstreamDrawIndices", value.upstream_draw_indices},
      {"inputs", value.inputs},
      {"outputs", value.outputs},
  };
}

struct FormatBottleneck {
  std::uint32_t draw_index = 0u;
  std::vector<std::string> shader_hashes;
  std::string resource_handle_hex;
  std::optional<std::string> resource_view_handle_hex = std::nullopt;
  std::string format;
  std::uint32_t width = 0u;
  std::uint32_t height = 0u;
  bool is_backbuffer = false;
  std::string reason;
};

// NOLINTNEXTLINE(readability-identifier-naming)
inline void to_json(json& j, const FormatBottleneck& value) {
  j = {
      {"drawIndex", value.draw_index},
      {"shaderHashes", value.shader_hashes},
      {"resourceHandle", value.resource_handle_hex},
      {"resourceViewHandle", value.resource_view_handle_hex.has_value() ? json(value.resource_view_handle_hex.value()) : json(nullptr)},
      {"format", value.format},
      {"width", value.width},
      {"height", value.height},
      {"isBackbuffer", value.is_backbuffer},
      {"reason", value.reason},
  };
}

struct UiDrawEntry {
  std::uint32_t draw_index = 0u;
  std::string method;
  std::vector<std::string> shader_hashes;
  bool blend_enabled = false;
  bool is_fullscreen_pass = false;
};

// NOLINTNEXTLINE(readability-identifier-naming)
inline void to_json(json& j, const UiDrawEntry& value) {
  j = {
      {"drawIndex", value.draw_index},
      {"method", value.method},
      {"shaderHashes", value.shader_hashes},
      {"blendEnabled", value.blend_enabled},
      {"isFullscreenPass", value.is_fullscreen_pass},
  };
}

struct SnapshotGraph {
  std::uint32_t device_index = 0u;
  std::vector<std::uint64_t> backbuffer_handles;
  std::vector<std::string> backbuffer_handle_hexes;
  std::vector<DrawNode> draws;
};

struct TraceResult {
  std::uint32_t device_index = 0u;
  std::size_t total_snapshot_draws = 0u;
  std::vector<std::string> backbuffer_handles;
  std::vector<std::uint32_t> seed_draw_indices;
  std::optional<std::uint32_t> ui_start_draw_index = std::nullopt;
  std::vector<UiDrawEntry> ui_draws;
  std::vector<ChainPassEntry> chain;
  std::vector<FormatBottleneck> format_bottlenecks;
};

struct ToolContext {
  std::function<std::uint32_t(const json& arguments)> resolve_device_index;
  std::function<SnapshotGraph(std::uint32_t device_index)> capture_snapshot_graph;
};

[[nodiscard]] inline TraceResult TraceOutputChain(
    const SnapshotGraph& graph,
    std::uint32_t max_depth,
    bool include_ui_draws) {
  TraceResult trace = {
      .device_index = graph.device_index,
      .total_snapshot_draws = graph.draws.size(),
      .backbuffer_handles = graph.backbuffer_handle_hexes,
  };

  if (graph.draws.empty()) {
    return trace;
  }

  std::unordered_set<std::uint64_t> backbuffer_set(
      graph.backbuffer_handles.begin(),
      graph.backbuffer_handles.end());

  std::unordered_map<std::uint64_t, std::vector<std::uint32_t>> producers_by_resource;
  std::vector<std::uint32_t> backbuffer_writer_indices;
  std::optional<std::uint32_t> last_opaque_fullscreen_backbuffer_draw = std::nullopt;

  for (const auto& draw : graph.draws) {
    bool writes_bb = draw.writes_backbuffer;
    for (const auto& output : draw.outputs) {
      if (output.resource_handle != 0u) {
        producers_by_resource[output.resource_handle].push_back(draw.draw_index);
        if (output.is_backbuffer || backbuffer_set.contains(output.resource_handle)) {
          writes_bb = true;
        }
      }
    }
    if (writes_bb) {
      backbuffer_writer_indices.push_back(draw.draw_index);
      const bool is_copy_method = (draw.method == "CopyResource" || draw.method == "CopyTextureRegion");
      if (is_copy_method || (draw.is_fullscreen_pass && !draw.blend_enabled)) {
        last_opaque_fullscreen_backbuffer_draw = draw.draw_index;
      }
    }
  }

  if (backbuffer_writer_indices.empty()) {
    return trace;
  }

  if (last_opaque_fullscreen_backbuffer_draw.has_value()) {
    trace.seed_draw_indices.push_back(last_opaque_fullscreen_backbuffer_draw.value());
    for (const auto bb_draw_index : backbuffer_writer_indices) {
      if (bb_draw_index <= last_opaque_fullscreen_backbuffer_draw.value()) continue;
      if (!trace.ui_start_draw_index.has_value()) {
        trace.ui_start_draw_index = bb_draw_index;
      }
      if (include_ui_draws) {
        const auto& ui_draw = graph.draws[bb_draw_index];
        trace.ui_draws.push_back(UiDrawEntry{
            .draw_index = ui_draw.draw_index,
            .method = ui_draw.method,
            .shader_hashes = ui_draw.shader_hashes,
            .blend_enabled = ui_draw.blend_enabled,
            .is_fullscreen_pass = ui_draw.is_fullscreen_pass,
        });
      }
    }
  } else {
    trace.seed_draw_indices.push_back(backbuffer_writer_indices.back());
  }

  std::unordered_map<std::uint32_t, std::uint32_t> depth_by_draw;
  std::unordered_map<std::uint32_t, std::set<std::uint32_t>> upstream_by_draw;
  std::deque<std::uint32_t> work_queue;

  for (const auto seed : trace.seed_draw_indices) {
    depth_by_draw[seed] = 0u;
    work_queue.push_back(seed);
  }

  while (!work_queue.empty()) {
    const auto current_index = work_queue.front();
    work_queue.pop_front();
    const auto current_depth = depth_by_draw[current_index];
    if (current_depth >= max_depth) continue;

    const auto& current_draw = graph.draws[current_index];
    for (const auto& input : current_draw.inputs) {
      if (input.resource_handle == 0u) continue;
      const auto prod_it = producers_by_resource.find(input.resource_handle);
      if (prod_it == producers_by_resource.end()) continue;

      const auto& writers = prod_it->second;
      const auto bound = std::lower_bound(writers.begin(), writers.end(), current_index);
      if (bound == writers.begin()) continue;

      const auto producer_index = *std::prev(bound);
      upstream_by_draw[current_index].insert(producer_index);
      if (!depth_by_draw.contains(producer_index)) {
        depth_by_draw[producer_index] = current_depth + 1u;
        work_queue.push_back(producer_index);
      }
    }
  }

  std::vector<std::uint32_t> sorted_draw_indices;
  sorted_draw_indices.reserve(depth_by_draw.size());
  for (const auto& [draw_index, depth] : depth_by_draw) {
    (void)depth;
    sorted_draw_indices.push_back(draw_index);
  }
  std::ranges::sort(sorted_draw_indices);

  trace.chain.reserve(sorted_draw_indices.size());
  for (const auto draw_index : sorted_draw_indices) {
    const auto& draw = graph.draws[draw_index];
    std::vector<std::uint32_t> upstream_list;
    if (const auto up_it = upstream_by_draw.find(draw_index); up_it != upstream_by_draw.end()) {
      upstream_list.assign(up_it->second.begin(), up_it->second.end());
    }

    for (const auto& output : draw.outputs) {
      std::string reason;
      if (output.is_rgb10a2_unorm) {
        if (!(output.is_backbuffer && output.is_hdr10_backbuffer)) {
          reason = "rgb10a2_unorm_clips_above_1_and_below_0";
        }
      } else if (output.is_unorm) {
        reason = "unorm_clips_above_1_and_below_0";
      } else if (output.is_r11g11b10f) {
        reason = "r11g11b10f_clips_below_0";
      }
      if (!reason.empty()) {
        trace.format_bottlenecks.push_back(FormatBottleneck{
            .draw_index = draw.draw_index,
            .shader_hashes = draw.shader_hashes,
            .resource_handle_hex = output.resource_handle_hex,
            .resource_view_handle_hex = output.resource_view_handle == 0u
                                            ? std::nullopt
                                            : std::optional<std::string>(output.resource_view_handle_hex),
            .format = output.format,
            .width = output.width,
            .height = output.height,
            .is_backbuffer = output.is_backbuffer,
            .reason = std::move(reason),
        });
      }
    }

    trace.chain.push_back(ChainPassEntry{
        .draw_index = draw.draw_index,
        .depth_from_output = depth_by_draw[draw_index],
        .method = draw.method,
        .active_shader_stage = draw.active_shader_stage,
        .shader_hashes = draw.shader_hashes,
        .blend_enabled = draw.blend_enabled,
        .writes_backbuffer = draw.writes_backbuffer,
        .is_fullscreen_pass = draw.is_fullscreen_pass,
        .upstream_draw_indices = std::move(upstream_list),
        .inputs = draw.inputs,
        .outputs = draw.outputs,
    });
  }

  return trace;
}

[[nodiscard]] inline ToolResult HandleTraceOutputChainTool(const json& arguments, const ToolContext& context) {
  if (!context.resolve_device_index || !context.capture_snapshot_graph) {
    throw std::runtime_error("OutputChainToolContext is not initialized.");
  }

  const auto device_index = context.resolve_device_index(arguments);
  const auto max_depth = mcp_arguments::ValidateRange(
      "maxDepth",
      mcp_arguments::GetOptional<std::uint32_t>(arguments, "maxDepth").value_or(64u),
      1u,
      256u);
  const auto include_ui_draws = mcp_arguments::GetOptional<bool>(arguments, "includeUiDraws").value_or(true);

  const auto graph = context.capture_snapshot_graph(device_index);
  if (graph.draws.empty()) {
    return ToolResult{
        .text = std::format("Device #{} has no captured draws in the snapshot cache. Queue a snapshot first.", device_index),
        .structured_content = json{
            {"deviceIndex", device_index},
            {"totalSnapshotDraws", 0u},
            {"backbufferHandles", graph.backbuffer_handle_hexes},
            {"chain", json::array()},
            {"formatBottlenecks", json::array()},
        },
    };
  }

  const auto trace = TraceOutputChain(graph, max_depth, include_ui_draws);

  json result = {
      {"deviceIndex", trace.device_index},
      {"totalSnapshotDraws", trace.total_snapshot_draws},
      {"backbufferHandles", trace.backbuffer_handles},
      {"seedDrawIndices", trace.seed_draw_indices},
      {"uiStartDrawIndex", trace.ui_start_draw_index.has_value() ? json(trace.ui_start_draw_index.value()) : json(nullptr)},
      {"uiDrawCount", trace.ui_draws.size()},
      {"chainLength", trace.chain.size()},
      {"chain", trace.chain},
      {"formatBottlenecks", trace.format_bottlenecks},
  };
  if (include_ui_draws) {
    result["uiDraws"] = trace.ui_draws;
  }

  auto text = std::format(
      "Traced {} pass(es) leading to the backbuffer on device #{} (out of {} snapshot draws), found {} format bottleneck(s)",
      trace.chain.size(),
      device_index,
      trace.total_snapshot_draws,
      trace.format_bottlenecks.size());
  if (trace.ui_start_draw_index.has_value()) {
    text += std::format(", and detected UI/HUD draws starting at draw #{}.", trace.ui_start_draw_index.value());
  } else {
    text += ".";
  }

  return ToolResult{
      .text = text,
      .structured_content = result,
  };
}

[[nodiscard]] inline bool IsLikelyLutEndpoint(const ResourceEndpoint& endpoint) {
  if (endpoint.type == "texture_3d"
      && endpoint.width >= 4u && endpoint.width <= 128u
      && endpoint.height >= 4u && endpoint.height <= 128u
      && endpoint.depth_or_layers >= 4u && endpoint.depth_or_layers <= 128u) {
    return true;
  }
  if (endpoint.type == "texture_2d") {
    if (endpoint.height >= 8u && endpoint.height <= 128u
        && endpoint.width == endpoint.height * endpoint.height) {
      return true;
    }
    if (endpoint.width >= 8u && endpoint.width <= 128u
        && endpoint.height == endpoint.width * endpoint.width) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] inline ToolResult HandleClassifyPostprocessTool(const json& arguments, const ToolContext& context) {
  if (!context.resolve_device_index || !context.capture_snapshot_graph) {
    throw std::runtime_error("OutputChainToolContext is not initialized.");
  }

  const auto device_index = context.resolve_device_index(arguments);
  const auto max_depth = mcp_arguments::ValidateRange(
      "maxDepth",
      mcp_arguments::GetOptional<std::uint32_t>(arguments, "maxDepth").value_or(64u),
      1u,
      256u);

  const auto graph = context.capture_snapshot_graph(device_index);
  if (graph.draws.empty()) {
    return ToolResult{
        .text = std::format("Device #{} has no captured draws in the snapshot cache. Queue a snapshot first.", device_index),
        .structured_content = json{
            {"deviceIndex", device_index},
            {"classifiedPasses", json::array()},
            {"detectedLutResources", json::array()},
        },
    };
  }

  const auto trace = TraceOutputChain(graph, max_depth, true);

  std::uint32_t backbuffer_width = 0u;
  std::uint32_t backbuffer_height = 0u;
  for (const auto& pass : trace.chain) {
    for (const auto& output : pass.outputs) {
      if (output.is_backbuffer && output.width > 0u && output.height > 0u) {
        backbuffer_width = output.width;
        backbuffer_height = output.height;
        break;
      }
    }
    if (backbuffer_width > 0u) break;
  }

  json classified_passes = json::array();
  json detected_luts = json::array();
  std::unordered_set<std::uint64_t> seen_lut_handles;
  json tonemap_candidates = json::array();

  for (const auto& pass : trace.chain) {
    std::vector<std::string> roles;
    std::vector<std::string> evidence;

    const bool is_copy = (pass.method == "CopyResource" || pass.method == "CopyTextureRegion");
    if (is_copy) {
      roles.emplace_back("copy_or_resolve");
      evidence.push_back(std::format("method={}", pass.method));
    }

    bool reads_lut = false;
    bool reads_float_fullres = false;
    bool reads_subres_float = false;
    std::size_t fullres_input_count = 0u;
    for (const auto& input : pass.inputs) {
      if (IsLikelyLutEndpoint(input)) {
        reads_lut = true;
        evidence.push_back(std::format(
            "reads LUT-shaped {} ({}x{}x{} {})",
            input.type,
            input.width,
            input.height,
            input.depth_or_layers,
            input.format));
        if (input.resource_handle != 0u && seen_lut_handles.insert(input.resource_handle).second) {
          detected_luts.push_back(json{
              {"drawIndex", pass.draw_index},
              {"shaderHashes", pass.shader_hashes},
              {"resourceHandle", input.resource_handle_hex},
              {"resourceViewHandle", input.resource_view_handle == 0u ? json(nullptr) : json(input.resource_view_handle_hex)},
              {"type", input.type},
              {"format", input.format},
              {"width", input.width},
              {"height", input.height},
              {"depthOrLayers", input.depth_or_layers},
          });
        }
      }
      if (backbuffer_width > 0u && input.width == backbuffer_width && input.height == backbuffer_height) {
        ++fullres_input_count;
        if (input.is_float) {
          reads_float_fullres = true;
        }
      } else if (backbuffer_width > 0u && input.is_float && input.width > 0u && input.height > 0u
                 && (input.width <= (backbuffer_width / 2u) || input.height <= (backbuffer_height / 2u))) {
        reads_subres_float = true;
      }
    }

    bool writes_unorm = false;
    bool writes_float_subres = false;
    bool writes_hdr10_bb = false;
    bool writes_scrgb_bb = false;
    bool writes_sdr_bb = false;
    for (const auto& output : pass.outputs) {
      if (output.is_backbuffer) {
        if (output.is_hdr10_backbuffer) {
          writes_hdr10_bb = true;
        } else if (output.is_float) {
          writes_scrgb_bb = true;
        } else {
          writes_sdr_bb = true;
        }
      }
      if (output.is_unorm) {
        writes_unorm = true;
      }
      if (backbuffer_width > 0u && output.is_float && output.width > 0u && output.height > 0u
          && (output.width <= (backbuffer_width / 2u) || output.height <= (backbuffer_height / 2u))) {
        writes_float_subres = true;
      }
    }

    if (writes_hdr10_bb) {
      roles.emplace_back("hdr10_output");
      evidence.emplace_back("writes HDR10 PQ/HLG backbuffer");
    } else if (writes_scrgb_bb) {
      roles.emplace_back("scrgb_output");
      evidence.emplace_back("writes floating-point scRGB backbuffer");
    } else if (writes_sdr_bb) {
      roles.emplace_back("sdr_swapchain_output");
      evidence.emplace_back("writes UNORM SDR backbuffer");
    }

    if (reads_lut) {
      roles.emplace_back("lut_or_color_grade");
    }
    if (writes_float_subres || (reads_subres_float && !reads_float_fullres && !writes_hdr10_bb && !writes_sdr_bb)) {
      roles.emplace_back("bloom_or_blur_pyramid");
      evidence.emplace_back("operates on half/quarter-res float target");
    }
    if (!is_copy && (reads_lut || (reads_float_fullres && writes_unorm) || (reads_float_fullres && reads_subres_float))) {
      roles.emplace_back("tonemap_or_grade_candidate");
      if (reads_float_fullres && writes_unorm) {
        evidence.emplace_back("converts full-res float input to UNORM output");
      }
      tonemap_candidates.push_back(json{
          {"drawIndex", pass.draw_index},
          {"shaderHashes", pass.shader_hashes},
          {"depthFromOutput", pass.depth_from_output},
      });
    }
    if (!is_copy && pass.depth_from_output <= 1u && fullres_input_count == 1u && pass.is_fullscreen_pass && !reads_lut) {
      roles.emplace_back("sharpen_or_post_copy");
      evidence.emplace_back("single full-res input pass at end of output chain");
    }
    if (roles.empty()) {
      roles.emplace_back("scene_postprocess");
    }

    classified_passes.push_back(json{
        {"drawIndex", pass.draw_index},
        {"depthFromOutput", pass.depth_from_output},
        {"method", pass.method},
        {"shaderHashes", pass.shader_hashes},
        {"roles", roles},
        {"evidence", evidence},
        {"writesBackbuffer", pass.writes_backbuffer},
        {"isFullscreenPass", pass.is_fullscreen_pass},
        {"blendEnabled", pass.blend_enabled},
    });
  }

  json result = {
      {"deviceIndex", device_index},
      {"chainLength", trace.chain.size()},
      {"uiStartDrawIndex", trace.ui_start_draw_index.has_value() ? json(trace.ui_start_draw_index.value()) : json(nullptr)},
      {"uiDrawCount", trace.ui_draws.size()},
      {"formatBottleneckCount", trace.format_bottlenecks.size()},
      {"tonemapCandidates", tonemap_candidates},
      {"detectedLutResources", detected_luts},
      {"classifiedPasses", classified_passes},
      {"formatBottlenecks", trace.format_bottlenecks},
  };

  auto text = std::format(
      "Classified {} output-chain pass(es) on device #{}: {} tonemap/grade candidate(s), {} LUT resource(s), {} format bottleneck(s).",
      trace.chain.size(),
      device_index,
      tonemap_candidates.size(),
      detected_luts.size(),
      trace.format_bottlenecks.size());
  return ToolResult{
      .text = text,
      .structured_content = result,
  };
}

}  // namespace renodx::addons::devkit::mcp::output_chain
