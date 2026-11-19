/*
 * Copyright (C) 2026 Carlos Lopez
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <include/reshade.hpp>

#include "../../../utils/mcp/arguments.hpp"
#include "../../../utils/mcp/types.hpp"
#include "../../../utils/resource.hpp"
#include "output_chain.hpp"

namespace renodx::addons::devkit::mcp::upgrade_candidates {

using json = renodx::utils::mcp::json;
using ToolResult = renodx::utils::mcp::ToolResult;
namespace mcp_arguments = renodx::utils::mcp::arguments;

struct TrackedResourceMeta {
  std::uint64_t resource_handle = 0u;
  std::string resource_handle_hex;
  std::string type;
  reshade::api::resource_desc desc = {};
  std::string format_name;
  bool is_backbuffer = false;
  bool is_hdr10_backbuffer = false;
  bool clone_enabled = false;
};

struct CopyOperationRecord {
  std::uint32_t draw_index = 0u;
  std::string method;
  std::uint64_t source_handle = 0u;
  std::uint64_t destination_handle = 0u;
};

struct UpgradeAnalysisInput {
  std::uint32_t device_index = 0u;
  std::string api;
  reshade::api::resource_desc backbuffer_desc = {};
  std::string backbuffer_format;
  std::string backbuffer_color_space;
  bool backbuffer_is_hdr = false;
  output_chain::SnapshotGraph graph;
  std::unordered_map<std::uint64_t, TrackedResourceMeta> resources_by_handle;
  std::vector<CopyOperationRecord> copy_operations;
};

struct ToolContext {
  std::function<std::uint32_t(const json& arguments)> resolve_device_index;
  std::function<UpgradeAnalysisInput(std::uint32_t device_index)> capture_upgrade_analysis_input;
};

[[nodiscard]] inline ToolResult HandleCheckUpgradeCandidatesTool(const json& arguments, const ToolContext& context) {
  if (!context.resolve_device_index || !context.capture_upgrade_analysis_input) {
    throw std::runtime_error("UpgradeCandidatesToolContext is not initialized.");
  }

  const auto has_usage_flag = [](reshade::api::resource_usage usage, reshade::api::resource_usage flag) {
    return (static_cast<std::uint32_t>(usage) & static_cast<std::uint32_t>(flag)) != 0u;
  };

  const auto device_index = context.resolve_device_index(arguments);
  const auto max_depth = mcp_arguments::ValidateRange(
      "maxDepth",
      mcp_arguments::GetOptional<std::uint32_t>(arguments, "maxDepth").value_or(64u),
      1u,
      256u);
  const auto include_all_snapshot_targets =
      mcp_arguments::GetOptional<bool>(arguments, "includeAllSnapshotTargets").value_or(false);
  const auto include_backbuffers =
      mcp_arguments::GetOptional<bool>(arguments, "includeBackbuffers").value_or(false);

  const auto input = context.capture_upgrade_analysis_input(device_index);
  if (input.graph.draws.empty()) {
    return ToolResult{
        .text = std::format(
            "Device #{} has no captured draws in the snapshot cache. Queue a snapshot first.",
            device_index),
        .structured_content = json{
            {"deviceIndex", device_index},
            {"api", input.api},
            {"candidates", json::array()},
            {"suggestions", json::array()},
            {"recommendedAddonSnippet", ""},
        },
    };
  }

  const auto trace = output_chain::TraceOutputChain(input.graph, max_depth, false);

  std::unordered_set<std::uint32_t> chain_draw_set;
  chain_draw_set.reserve(trace.chain.size());
  for (const auto& pass : trace.chain) {
    chain_draw_set.insert(pass.draw_index);
  }

  struct ResourceSnapshotStats {
    bool in_output_chain = false;
    bool seen_rtv = false;
    bool seen_uav = false;
    bool seen_srv = false;
    bool seen_copy_source = false;
    bool seen_copy_dest = false;
    std::set<std::uint32_t> writer_draws;
    std::set<std::uint32_t> reader_draws;
    std::set<std::string> writer_shaders;
    std::set<std::string> observed_view_formats;
    std::uint32_t first_chain_order = 0xFFFFFFFFu;
  };

  std::unordered_map<std::uint64_t, ResourceSnapshotStats> stats_by_resource;
  std::uint32_t chain_order_counter = 0u;
  for (const auto& draw : input.graph.draws) {
    const bool in_chain = chain_draw_set.contains(draw.draw_index);
    for (const auto& endpoint : draw.inputs) {
      if (endpoint.resource_handle == 0u) continue;
      auto& stats = stats_by_resource[endpoint.resource_handle];
      if (!endpoint.format.empty() && endpoint.format != "buffer") {
        stats.observed_view_formats.insert(endpoint.format);
      }
      stats.reader_draws.insert(draw.draw_index);
      if (endpoint.role == "copy_src") {
        stats.seen_copy_source = true;
      } else {
        stats.seen_srv = true;
      }
    }
    for (const auto& endpoint : draw.outputs) {
      if (endpoint.resource_handle == 0u) continue;
      auto& stats = stats_by_resource[endpoint.resource_handle];
      if (!endpoint.format.empty() && endpoint.format != "buffer") {
        stats.observed_view_formats.insert(endpoint.format);
      }
      stats.writer_draws.insert(draw.draw_index);
      for (const auto& hash : draw.shader_hashes) {
        stats.writer_shaders.insert(hash);
      }
      if (in_chain) {
        stats.in_output_chain = true;
        if (stats.first_chain_order == 0xFFFFFFFFu) {
          stats.first_chain_order = chain_order_counter++;
        }
      }
      if (endpoint.role.starts_with("rtv:")) {
        stats.seen_rtv = true;
      } else if (endpoint.role.starts_with("uav:")) {
        stats.seen_uav = true;
      } else if (endpoint.role == "copy_dst") {
        stats.seen_copy_dest = true;
      }
    }
  }

  const auto backbuffer_width = input.backbuffer_desc.texture.width;
  const auto backbuffer_height = input.backbuffer_desc.texture.height;
  const float backbuffer_aspect = (backbuffer_width > 0u && backbuffer_height > 0u)
                                      ? (static_cast<float>(backbuffer_width) / static_cast<float>(backbuffer_height))
                                      : 0.f;
  const bool api_prefers_cloning = (input.api == "d3d12" || input.api == "vulkan");

  // Collect candidate resource handles in deterministic order.
  std::vector<std::uint64_t> candidate_handles;
  std::unordered_set<std::uint64_t> candidate_handle_set;
  for (const auto& [handle, stats] : stats_by_resource) {
    if (!include_all_snapshot_targets && !stats.in_output_chain) continue;
    if (stats.writer_draws.empty()) continue;

    const auto meta_it = input.resources_by_handle.find(handle);
    if (meta_it == input.resources_by_handle.end()) continue;
    const auto& meta = meta_it->second;
    if (meta.desc.type == reshade::api::resource_type::buffer
        || meta.desc.type == reshade::api::resource_type::unknown) {
      continue;
    }
    if (meta.is_backbuffer && meta.is_hdr10_backbuffer) continue;
    if (meta.is_backbuffer && !include_backbuffers) continue;
    const bool can_write_gpu =
        has_usage_flag(meta.desc.usage, reshade::api::resource_usage::render_target)
        || has_usage_flag(meta.desc.usage, reshade::api::resource_usage::unordered_access)
        || stats.seen_copy_dest;
    if (!meta.is_backbuffer && !can_write_gpu) continue;
    bool is_candidate_format = false;
    switch (meta.desc.texture.format) {
      case reshade::api::format::r8g8b8a8_typeless:
      case reshade::api::format::r8g8b8a8_unorm:
      case reshade::api::format::r8g8b8a8_unorm_srgb:
      case reshade::api::format::b8g8r8a8_typeless:
      case reshade::api::format::b8g8r8a8_unorm:
      case reshade::api::format::b8g8r8a8_unorm_srgb:
      case reshade::api::format::r8g8b8x8_unorm:
      case reshade::api::format::r8g8b8x8_unorm_srgb:
      case reshade::api::format::b8g8r8x8_typeless:
      case reshade::api::format::b8g8r8x8_unorm:
      case reshade::api::format::b8g8r8x8_unorm_srgb:
      case reshade::api::format::r10g10b10a2_typeless:
      case reshade::api::format::r10g10b10a2_unorm:
      case reshade::api::format::b10g10r10a2_typeless:
      case reshade::api::format::b10g10r10a2_unorm:
      case reshade::api::format::r11g11b10_float:
      case reshade::api::format::r16g16b16a16_unorm:
        is_candidate_format = true;
        break;
      default:
        break;
    }
    if (!is_candidate_format) continue;

    candidate_handles.push_back(handle);
    candidate_handle_set.insert(handle);
  }

  std::ranges::sort(candidate_handles, [&](std::uint64_t lhs, std::uint64_t rhs) {
    const auto& lhs_stats = stats_by_resource[lhs];
    const auto& rhs_stats = stats_by_resource[rhs];
    if (lhs_stats.in_output_chain != rhs_stats.in_output_chain) {
      return lhs_stats.in_output_chain > rhs_stats.in_output_chain;
    }
    if (lhs_stats.first_chain_order != rhs_stats.first_chain_order) {
      return lhs_stats.first_chain_order < rhs_stats.first_chain_order;
    }
    return lhs < rhs;
  });

  const auto decode_usage_flags = [&has_usage_flag](reshade::api::resource_usage usage) {
    std::vector<std::string> names;
    const auto check = [&](reshade::api::resource_usage flag, std::string_view label) {
      if (has_usage_flag(usage, flag)) {
        names.emplace_back(label);
      }
    };
    check(reshade::api::resource_usage::render_target, "render_target");
    check(reshade::api::resource_usage::unordered_access, "unordered_access");
    check(reshade::api::resource_usage::shader_resource, "shader_resource");
    check(reshade::api::resource_usage::depth_stencil, "depth_stencil");
    check(reshade::api::resource_usage::copy_source, "copy_source");
    check(reshade::api::resource_usage::copy_dest, "copy_dest");
    check(reshade::api::resource_usage::resolve_source, "resolve_source");
    check(reshade::api::resource_usage::resolve_dest, "resolve_dest");
    return names;
  };

  struct RuleGroupKey {
    reshade::api::format old_format = reshade::api::format::unknown;
    std::string sizing_mode;  // "backbuffer_exact", "backbuffer_aspect", "explicit_dimensions"
    std::uint32_t explicit_width = 0u;
    std::uint32_t explicit_height = 0u;
    std::uint32_t usage_include_mask = 0u;

    bool operator<(const RuleGroupKey& other) const {
      return std::tie(old_format, sizing_mode, explicit_width, explicit_height, usage_include_mask)
             < std::tie(other.old_format, other.sizing_mode, other.explicit_width, other.explicit_height, other.usage_include_mask);
    }
  };

  struct RuleGroupAccumulator {
    std::string old_format_name;
    bool recommend_view_cloning = false;
    std::vector<std::string> candidate_handles;
    std::set<std::string> reasons;
    std::set<std::string> risk_codes;
  };

  std::map<RuleGroupKey, RuleGroupAccumulator> rule_groups;
  json candidates_json = json::array();
  bool global_recommend_use_resource_cloning = api_prefers_cloning && !candidate_handles.empty();
  std::size_t high_risk_count = 0u;
  std::size_t medium_risk_count = 0u;

  for (const auto handle : candidate_handles) {
    const auto& meta = input.resources_by_handle.at(handle);
    const auto& stats = stats_by_resource.at(handle);
    const auto width = meta.desc.texture.width;
    const auto height = meta.desc.texture.height;
    const auto depth_or_layers = meta.desc.texture.depth_or_layers;
    const auto mip_levels = meta.desc.texture.levels;
    const auto sample_count = meta.desc.texture.samples;
    const auto fmt = meta.desc.texture.format;

    const bool matches_bb_size = (backbuffer_width > 0u && backbuffer_height > 0u
                                  && width == backbuffer_width && height == backbuffer_height);
    const float aspect_ratio = (height > 0u) ? (static_cast<float>(width) / static_cast<float>(height)) : 0.f;
    const bool matches_bb_aspect = (backbuffer_aspect > 0.f && aspect_ratio > 0.f
                                    && std::abs(aspect_ratio - backbuffer_aspect) <= 0.0001f);

    std::string bottleneck_reason = "unorm_clips_above_1_and_below_0";
    if (fmt == reshade::api::format::r11g11b10_float) {
      bottleneck_reason = "r11g11b10f_clips_below_0_and_low_precision";
    } else if (fmt == reshade::api::format::r10g10b10a2_unorm
               || fmt == reshade::api::format::b10g10r10a2_unorm
               || fmt == reshade::api::format::r10g10b10a2_typeless
               || fmt == reshade::api::format::b10g10r10a2_typeless) {
      bottleneck_reason = "rgb10a2_unorm_clips_above_1_and_below_0";
    }

    json risks = json::array();
    std::set<std::string> risk_codes;
    bool recommend_view_cloning = api_prefers_cloning;
    const auto add_risk = [&](std::string_view code, std::string_view severity, std::string message) {
      if (!risk_codes.insert(std::string(code)).second) return;
      if (severity == "high") {
        ++high_risk_count;
      } else if (severity == "medium") {
        ++medium_risk_count;
      }
      risks.push_back(json{
          {"code", code},
          {"severity", severity},
          {"message", std::move(message)},
      });
    };

    // Check copy peers across the entire snapshot for format-mismatch risks.
    json copy_peers = json::array();
    for (const auto& copy_op : input.copy_operations) {
      const bool is_src = (copy_op.source_handle == handle);
      const bool is_dst = (copy_op.destination_handle == handle);
      if (!is_src && !is_dst) continue;
      const auto peer_handle = (is_src ? copy_op.destination_handle : copy_op.source_handle);
      if (peer_handle == 0u || peer_handle == handle) continue;

      const auto peer_it = input.resources_by_handle.find(peer_handle);
      const std::string peer_hex = (peer_it != input.resources_by_handle.end())
                                       ? peer_it->second.resource_handle_hex
                                       : std::format("0x{:016X}", peer_handle);
      const std::string peer_format = (peer_it != input.resources_by_handle.end())
                                          ? peer_it->second.format_name
                                          : "unknown";
      const bool peer_is_bb = (peer_it != input.resources_by_handle.end()) && peer_it->second.is_backbuffer;
      const bool peer_is_candidate = candidate_handle_set.contains(peer_handle);
      const bool peer_is_already_rgba16f =
          (peer_it != input.resources_by_handle.end()
           && peer_it->second.desc.texture.format == reshade::api::format::r16g16b16a16_float);
      const bool format_mismatch_risk = (!peer_is_candidate && !peer_is_already_rgba16f);

      copy_peers.push_back(json{
          {"drawIndex", copy_op.draw_index},
          {"method", copy_op.method},
          {"direction", is_src ? "to" : "from"},
          {"peerHandle", peer_hex},
          {"peerFormat", peer_format},
          {"peerIsBackbuffer", peer_is_bb},
          {"peerIsCandidate", peer_is_candidate},
          {"formatMismatchRisk", format_mismatch_risk},
      });

      if (format_mismatch_risk) {
        recommend_view_cloning = true;
        global_recommend_use_resource_cloning = true;
        add_risk(
            "copy_format_mismatch",
            "high",
            std::format(
                "Copy {} {} ({}) at draw #{}: upgrading this resource to r16g16b16a16_float changes bit depth (32bpp -> 64bpp). Enable use_resource_cloning = true or also upgrade the peer/swapchain.",
                is_src ? "to" : "from",
                peer_is_bb ? "backbuffer" : peer_hex,
                peer_format,
                copy_op.draw_index));
      }
    }

    if (sample_count > 1u) {
      recommend_view_cloning = true;
      global_recommend_use_resource_cloning = true;
      add_risk(
          "msaa_target",
          "high",
          std::format(
              "Multisampled target ({}x MSAA): ResolveTextureRegion requires matching source and destination formats.",
              sample_count));
    }

    if (meta.desc.heap == reshade::api::memory_heap::cpu_to_gpu
        || meta.desc.heap == reshade::api::memory_heap::gpu_to_cpu
        || meta.desc.heap == reshade::api::memory_heap::cpu_only
        || (static_cast<std::uint32_t>(meta.desc.flags) & static_cast<std::uint32_t>(reshade::api::resource_flags::shared)) != 0u) {
      add_risk(
          "cpu_access_or_shared",
          "high",
          "Resource uses a CPU-accessible heap or shared handle; upgrading its format can break CPU stride or cross-API sharing.");
    }

    bool has_srgb_view = (fmt == reshade::api::format::r8g8b8a8_unorm_srgb
                          || fmt == reshade::api::format::b8g8r8a8_unorm_srgb
                          || fmt == reshade::api::format::r8g8b8x8_unorm_srgb
                          || fmt == reshade::api::format::b8g8r8x8_unorm_srgb);
    bool has_non_color_view = false;
    for (const auto& view_fmt : stats.observed_view_formats) {
      if (view_fmt.find("_srgb") != std::string::npos) {
        has_srgb_view = true;
      }
      if (view_fmt.find("_uint") != std::string::npos
          || view_fmt.find("_sint") != std::string::npos
          || view_fmt == "r32_float") {
        has_non_color_view = true;
      }
    }
    if (has_non_color_view) {
      add_risk(
          "non_color_view_aliasing",
          "high",
          "Resource is viewed through an integer or 32-bit raw format view; upgrading to 64-bit r16g16b16a16_float will break bit-cast aliasing.");
    }
    if (has_srgb_view) {
      recommend_view_cloning = true;
      add_risk(
          "srgb_view_gamma_loss",
          "medium",
          "Resource uses sRGB format or sRGB views; r16g16b16a16_float has no hardware sRGB view, so verify shader gamma encode/decode or use_resource_view_cloning.");
    }

    const bool has_uav = stats.seen_uav || has_usage_flag(meta.desc.usage, reshade::api::resource_usage::unordered_access);
    if (has_uav) {
      recommend_view_cloning = true;
      add_risk(
          "uav_usage",
          "medium",
          "Resource has unordered_access (UAV) usage; ensure shaders do not pack/unpack 32-bit formats manually and consider use_resource_view_cloning = true.");
    }

    if (mip_levels > 1u) {
      recommend_view_cloning = true;
      add_risk(
          "mipmapped_target",
          "low",
          std::format(
              "Mipmapped texture ({} mip levels); verify mip generation and per-mip view compatibility.",
              mip_levels));
    }

    if (depth_or_layers > 1u) {
      add_risk(
          "array_or_cube_target",
          "low",
          std::format(
              "Texture array or cubemap ({} layers/slices); ResourceUpgradeInfo leaves depth unconstrained by default.",
              depth_or_layers));
    }

    if (!matches_bb_size && !matches_bb_aspect) {
      add_risk(
          "non_backbuffer_aspect_ratio",
          "low",
          std::format(
              "Dimensions {}x{} do not match backbuffer aspect ratio ({}x{}); rule uses explicit dimensions.",
              width,
              height,
              backbuffer_width,
              backbuffer_height));
    }

    if (recommend_view_cloning) {
      global_recommend_use_resource_cloning = true;
      add_risk(
          "missing_use_resource_cloning",
          "medium",
          "Set renodx::mods::swapchain::use_resource_cloning = true in addon.cpp (recommended for DX12/Vulkan, copy compatibility, or view cloning).");
    }

    std::vector<std::uint32_t> writer_draws(stats.writer_draws.begin(), stats.writer_draws.end());
    std::vector<std::uint32_t> reader_draws(stats.reader_draws.begin(), stats.reader_draws.end());
    std::vector<std::string> shader_hashes(stats.writer_shaders.begin(), stats.writer_shaders.end());
    std::vector<std::string> view_formats(stats.observed_view_formats.begin(), stats.observed_view_formats.end());

    candidates_json.push_back(json{
        {"resourceHandle", meta.resource_handle_hex},
        {"type", meta.type},
        {"resourceFormat", meta.format_name},
        {"observedViewFormats", view_formats},
        {"recommendedFormat", "r16g16b16a16_float"},
        {"bottleneckReason", bottleneck_reason},
        {"width", width},
        {"height", height},
        {"depthOrLayers", depth_or_layers},
        {"mipLevels", mip_levels},
        {"sampleCount", sample_count},
        {"aspectRatio", aspect_ratio},
        {"matchesBackbufferSize", matches_bb_size},
        {"matchesBackbufferAspectRatio", matches_bb_aspect},
        {"isBackbuffer", meta.is_backbuffer},
        {"cloneEnabled", meta.clone_enabled},
        {"inOutputChain", stats.in_output_chain},
        {"usageMask", static_cast<std::uint32_t>(meta.desc.usage)},
        {"usageFlags", decode_usage_flags(meta.desc.usage)},
        {"snapshotRoles", {
                              {"seenRtv", stats.seen_rtv},
                              {"seenUav", stats.seen_uav},
                              {"seenSrv", stats.seen_srv},
                              {"seenCopySource", stats.seen_copy_source},
                              {"seenCopyDest", stats.seen_copy_dest},
                          }},
        {"writerDrawIndices", writer_draws},
        {"readerDrawIndices", reader_draws},
        {"shaderHashes", shader_hashes},
        {"copyPeers", copy_peers},
        {"risks", risks},
    });

    // Build ResourceUpgradeInfo suggestion group for non-backbuffer targets using actual creation usage flags.
    if (!meta.is_backbuffer) {
      std::uint32_t usage_include_mask = 0u;
      if (has_usage_flag(meta.desc.usage, reshade::api::resource_usage::render_target)) {
        usage_include_mask |= static_cast<std::uint32_t>(reshade::api::resource_usage::render_target);
      }
      if (has_usage_flag(meta.desc.usage, reshade::api::resource_usage::unordered_access)) {
        usage_include_mask |= static_cast<std::uint32_t>(reshade::api::resource_usage::unordered_access);
      }
      if (usage_include_mask == 0u && has_usage_flag(meta.desc.usage, reshade::api::resource_usage::copy_dest)) {
        usage_include_mask = static_cast<std::uint32_t>(reshade::api::resource_usage::copy_dest);
      }

      RuleGroupKey key = {
          .old_format = fmt,
          .sizing_mode = matches_bb_size
                             ? "backbuffer_exact"
                             : (matches_bb_aspect ? "backbuffer_aspect" : "explicit_dimensions"),
          .explicit_width = (!matches_bb_size && !matches_bb_aspect) ? width : 0u,
          .explicit_height = (!matches_bb_size && !matches_bb_aspect) ? height : 0u,
          .usage_include_mask = usage_include_mask,
      };
      auto& group = rule_groups[key];
      group.old_format_name = meta.format_name;
      group.recommend_view_cloning = group.recommend_view_cloning || recommend_view_cloning;
      group.candidate_handles.push_back(meta.resource_handle_hex);
      group.reasons.insert(bottleneck_reason);
      group.risk_codes.insert(risk_codes.begin(), risk_codes.end());
    }
  }

  // Merge exact-backbuffer and aspect-ratio groups of the same format + usage if both exist,
  // since aspect_ratio = BACK_BUFFER covers both exact and sub-res targets cleanly.
  std::vector<RuleGroupKey> keys_to_merge;
  for (const auto& [key, group] : rule_groups) {
    (void)group;
    if (key.sizing_mode != "backbuffer_exact") continue;
    RuleGroupKey aspect_key = key;
    aspect_key.sizing_mode = "backbuffer_aspect";
    if (rule_groups.contains(aspect_key)) {
      keys_to_merge.push_back(key);
    }
  }
  for (const auto& exact_key : keys_to_merge) {
    RuleGroupKey aspect_key = exact_key;
    aspect_key.sizing_mode = "backbuffer_aspect";
    auto& dst = rule_groups[aspect_key];
    const auto& src = rule_groups[exact_key];
    dst.recommend_view_cloning = dst.recommend_view_cloning || src.recommend_view_cloning;
    dst.candidate_handles.insert(dst.candidate_handles.end(), src.candidate_handles.begin(), src.candidate_handles.end());
    dst.reasons.insert(src.reasons.begin(), src.reasons.end());
    dst.risk_codes.insert(src.risk_codes.begin(), src.risk_codes.end());
    rule_groups.erase(exact_key);
  }

  // Evaluate blast radius with the real ResourceUpgradeInfo::CheckResourceDesc and format C++ snippets.
  json suggestions_json = json::array();
  std::string combined_cpp;
  if (global_recommend_use_resource_cloning && !rule_groups.empty()) {
    combined_cpp += "renodx::mods::swapchain::use_resource_cloning = true;\n\n";
  }

  for (const auto& [key, group] : rule_groups) {
    renodx::utils::resource::ResourceUpgradeInfo test_rule = {
        .old_format = key.old_format,
        .new_format = reshade::api::format::r16g16b16a16_float,
        .use_resource_view_cloning = group.recommend_view_cloning,
        .usage_include = static_cast<reshade::api::resource_usage>(key.usage_include_mask),
    };
    if (key.sizing_mode == "backbuffer_aspect") {
      test_rule.aspect_ratio = renodx::utils::resource::ResourceUpgradeInfo::BACK_BUFFER;
    } else if (key.sizing_mode == "explicit_dimensions") {
      test_rule.dimensions = {
          .width = static_cast<std::int16_t>(std::min<std::uint32_t>(key.explicit_width, 32767u)),
          .height = static_cast<std::int16_t>(std::min<std::uint32_t>(key.explicit_height, 32767u)),
      };
    }

    std::size_t matching_tracked = 0u;
    for (const auto& [res_handle, res_meta] : input.resources_by_handle) {
      (void)res_handle;
      if (res_meta.is_backbuffer) continue;
      if (res_meta.desc.type == reshade::api::resource_type::buffer
          || res_meta.desc.type == reshade::api::resource_type::unknown) {
        continue;
      }
      if (test_rule.CheckResourceDesc(res_meta.desc, input.backbuffer_desc)) {
        ++matching_tracked;
      }
    }

    const std::size_t extra_matches =
        (matching_tracked > group.candidate_handles.size())
            ? (matching_tracked - group.candidate_handles.size())
            : 0u;

    const bool has_rtv = has_usage_flag(test_rule.usage_include, reshade::api::resource_usage::render_target);
    const bool has_uav = has_usage_flag(test_rule.usage_include, reshade::api::resource_usage::unordered_access);
    const bool has_copy_dest = has_usage_flag(test_rule.usage_include, reshade::api::resource_usage::copy_dest);
    std::string usage_expr;
    if (has_rtv && has_uav) {
      usage_expr = "reshade::api::resource_usage::render_target | reshade::api::resource_usage::unordered_access";
    } else if (has_uav) {
      usage_expr = "reshade::api::resource_usage::unordered_access";
    } else if (has_rtv) {
      usage_expr = "reshade::api::resource_usage::render_target";
    } else if (has_copy_dest) {
      usage_expr = "reshade::api::resource_usage::copy_dest";
    }

    // Emit designated initializers in ResourceUpgradeInfo declaration order to avoid compiler warnings.
    std::string snippet = "renodx::mods::swapchain::swap_chain_upgrade_targets.push_back({\n";
    snippet += std::format("    .old_format = reshade::api::format::{},\n", group.old_format_name);
    snippet += "    .new_format = reshade::api::format::r16g16b16a16_float,\n";
    if (group.recommend_view_cloning) {
      snippet += "    .use_resource_view_cloning = true,\n";
    }
    if (key.sizing_mode == "backbuffer_aspect") {
      snippet += "    .aspect_ratio = renodx::mods::swapchain::SwapChainUpgradeTarget::BACK_BUFFER,\n";
    } else if (key.sizing_mode == "explicit_dimensions") {
      snippet += std::format("    .dimensions = {{.width = {}, .height = {}}},\n", key.explicit_width, key.explicit_height);
    }
    if (!usage_expr.empty()) {
      snippet += std::format("    .usage_include = {},\n", usage_expr);
    }
    snippet += "});";

    combined_cpp += snippet;
    combined_cpp += "\n";

    std::vector<std::string> group_risk_codes(group.risk_codes.begin(), group.risk_codes.end());
    if (extra_matches >= 10u) {
      group_risk_codes.emplace_back("broad_rule_blast_radius");
    }

    suggestions_json.push_back(json{
        {"oldFormat", group.old_format_name},
        {"newFormat", "r16g16b16a16_float"},
        {"sizingMode", key.sizing_mode},
        {"useResourceViewCloning", group.recommend_view_cloning},
        {"usageInclude", usage_expr},
        {"candidateCount", group.candidate_handles.size()},
        {"candidateResourceHandles", group.candidate_handles},
        {"matchingTrackedResourceCount", matching_tracked},
        {"extraUnrelatedMatchCount", extra_matches},
        {"riskCodes", group_risk_codes},
        {"cppSnippet", snippet},
    });
  }

  const bool swapchain_upgrade_needed = (!input.backbuffer_is_hdr && !input.backbuffer_format.empty());
  json result = {
      {"deviceIndex", device_index},
      {"api", input.api},
      {"scope", include_all_snapshot_targets ? "all_snapshot_targets" : "output_chain"},
      {"outputChainLength", trace.chain.size()},
      {"swapchain", {
                        {"format", input.backbuffer_format},
                        {"colorSpace", input.backbuffer_color_space},
                        {"width", backbuffer_width},
                        {"height", backbuffer_height},
                        {"isHdr", input.backbuffer_is_hdr},
                        {"upgradeNeeded", swapchain_upgrade_needed},
                    }},
      {"recommendUseResourceCloning", global_recommend_use_resource_cloning},
      {"candidateCount", candidates_json.size()},
      {"suggestionCount", suggestions_json.size()},
      {"highRiskCount", high_risk_count},
      {"mediumRiskCount", medium_risk_count},
      {"candidates", candidates_json},
      {"suggestions", suggestions_json},
      {"recommendedAddonSnippet", combined_cpp},
  };

  auto text = std::format(
      "Checked {} on device #{} ({}): found {} upgrade candidate(s), {} ResourceUpgradeInfo suggestion(s), {} high-risk and {} medium-risk finding(s).",
      include_all_snapshot_targets ? "all snapshot targets" : "output chain",
      device_index,
      input.api,
      candidates_json.size(),
      suggestions_json.size(),
      high_risk_count,
      medium_risk_count);
  return ToolResult{
      .text = text,
      .structured_content = result,
  };
}

}  // namespace renodx::addons::devkit::mcp::upgrade_candidates
