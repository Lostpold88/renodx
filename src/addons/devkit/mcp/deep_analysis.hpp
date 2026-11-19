/*
 * Copyright (C) 2026 Carlos Lopez
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <format>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <include/reshade.hpp>

#include "../../../utils/mcp/arguments.hpp"
#include "../../../utils/mcp/types.hpp"
#include "../../../utils/resource.hpp"
#include "../../../utils/trace.hpp"

namespace renodx::addons::devkit::mcp::deep_analysis {

using json = renodx::utils::mcp::json;
using ToolResult = renodx::utils::mcp::ToolResult;
namespace mcp_arguments = renodx::utils::mcp::arguments;

struct PushConstantRecord {
  std::uint32_t layout_param = 0u;
  std::uint32_t first = 0u;
  std::vector<std::uint32_t> values;
};

struct ConstantBufferBinding {
  std::uint32_t slot = 0u;
  std::uint32_t space = 0u;
  std::uint64_t buffer_handle = 0u;
  std::string buffer_handle_hex;
  std::uint64_t offset = 0u;
  std::uint64_t size = 0u;
  bool used_by_active_shader = false;
  std::vector<std::uint8_t> cached_bytes;
};

struct ConstantBufferData {
  std::uint32_t slot = 0u;
  std::uint32_t space = 0u;
  std::string buffer_handle_hex;
  std::uint64_t offset = 0u;
  std::uint64_t requested_size = 0u;
  bool used_by_active_shader = false;
  std::string source;
  std::vector<std::uint8_t> bytes;
  std::optional<std::string> error = std::nullopt;
};

struct DrawConstantsSnapshot {
  std::uint32_t device_index = 0u;
  std::uint32_t draw_index = 0u;
  std::vector<std::string> shader_hashes;
  std::vector<PushConstantRecord> push_constants;
  std::vector<ConstantBufferData> constant_buffers;
};

struct PendingConstantsRequest {
  std::uint32_t device_index = 0u;
  std::uint32_t draw_index = 0u;
  std::uint32_t max_bytes = 4096u;
  std::vector<std::string> shader_hashes;
  std::vector<PushConstantRecord> push_constants;
  std::vector<ConstantBufferBinding> bindings;

  std::mutex mutex;
  std::condition_variable cv;
  bool started = false;
  bool canceled = false;
  bool completed = false;
  std::optional<DrawConstantsSnapshot> snapshot = std::nullopt;
  std::optional<std::string> error = std::nullopt;
};

struct SnapshotProfile {
  std::uint32_t device_index = 0u;
  std::size_t total_draws = 0u;
  std::string swapchain_format;
  std::string swapchain_color_space;
  std::uint32_t swapchain_width = 0u;
  std::uint32_t swapchain_height = 0u;
  std::map<std::string, std::uint32_t> shader_draw_counts;
  std::set<std::string> target_formats;
  std::vector<std::string> output_chain_shaders;
  std::optional<std::uint32_t> ui_start_draw_index = std::nullopt;
};

struct ToolContext {
  std::function<std::uint32_t(const json& arguments)> resolve_device_index;
  std::function<std::shared_ptr<PendingConstantsRequest>(
      std::uint32_t device_index,
      std::uint32_t draw_index,
      std::optional<std::uint32_t> slot_filter,
      std::optional<std::uint32_t> space_filter,
      std::uint32_t max_bytes)>
      prepare_constants_request;
  std::function<bool(std::uint32_t device_index, const std::shared_ptr<PendingConstantsRequest>& request)> enqueue_constants_request;
  std::function<void(std::uint32_t device_index, const std::shared_ptr<PendingConstantsRequest>& request)> cancel_constants_request;
  std::function<SnapshotProfile(std::uint32_t device_index)> capture_snapshot_profile;
  std::function<ToolResult(std::uint32_t device_index, std::optional<bool> return_to_idle)> queue_trace;
};

namespace internal {

inline std::shared_mutex g_saved_constants_mutex;
inline std::unordered_map<std::string, DrawConstantsSnapshot> g_saved_constants;

inline std::shared_mutex g_saved_profiles_mutex;
inline std::unordered_map<std::string, SnapshotProfile> g_saved_profiles;

[[nodiscard]] inline std::string ToLowerAscii(std::string_view value) {
  std::string out;
  out.reserve(value.size());
  for (const auto ch : value) {
    out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
  }
  return out;
}

[[nodiscard]] inline float BitsToFloat(std::uint32_t bits) {
  float out = 0.f;
  std::memcpy(&out, &bits, sizeof(float));
  return out;
}

[[nodiscard]] inline json FloatOrNull(float value) {
  if (std::isfinite(value)) return json(static_cast<double>(value));
  return json(nullptr);
}

[[nodiscard]] inline std::vector<std::uint32_t> BytesToDwords(const std::vector<std::uint8_t>& bytes) {
  const std::size_t count = bytes.size() / sizeof(std::uint32_t);
  std::vector<std::uint32_t> dwords(count, 0u);
  if (count > 0u) {
    std::memcpy(dwords.data(), bytes.data(), count * sizeof(std::uint32_t));
  }
  return dwords;
}

[[nodiscard]] inline json FormatDwordsAsFloat4Rows(const std::vector<std::uint32_t>& dwords) {
  json rows = json::array();
  const std::size_t row_count = (dwords.size() + 3u) / 4u;
  for (std::size_t row = 0u; row < row_count; ++row) {
    json xyzw = json::array();
    json u32s = json::array();
    for (std::size_t lane = 0u; lane < 4u; ++lane) {
      const std::size_t idx = (row * 4u) + lane;
      if (idx < dwords.size()) {
        const auto bits = dwords[idx];
        xyzw.push_back(FloatOrNull(BitsToFloat(bits)));
        u32s.push_back(bits);
      } else {
        xyzw.push_back(nullptr);
        u32s.push_back(nullptr);
      }
    }
    rows.push_back(json{
        {"register", row},
        {"byteOffset", row * 16u},
        {"xyzw", xyzw},
        {"u32", u32s},
    });
  }
  return rows;
}

[[nodiscard]] inline json SerializeDrawConstants(const DrawConstantsSnapshot& snap) {
  json push_json = json::array();
  for (const auto& pc : snap.push_constants) {
    push_json.push_back(json{
        {"layoutParam", pc.layout_param},
        {"firstDword", pc.first},
        {"dwordCount", pc.values.size()},
        {"float4s", FormatDwordsAsFloat4Rows(pc.values)},
    });
  }

  json cbv_json = json::array();
  for (const auto& cb : snap.constant_buffers) {
    const auto dwords = BytesToDwords(cb.bytes);
    cbv_json.push_back(json{
        {"slot", cb.slot},
        {"space", cb.space},
        {"bufferHandle", cb.buffer_handle_hex},
        {"offset", cb.offset},
        {"byteSize", cb.bytes.size()},
        {"usedByActiveShader", cb.used_by_active_shader},
        {"source", cb.source},
        {"error", cb.error.has_value() ? json(cb.error.value()) : json(nullptr)},
        {"float4s", FormatDwordsAsFloat4Rows(dwords)},
    });
  }

  return json{
      {"deviceIndex", snap.device_index},
      {"drawIndex", snap.draw_index},
      {"shaderHashes", snap.shader_hashes},
      {"pushConstantCount", snap.push_constants.size()},
      {"constantBufferCount", snap.constant_buffers.size()},
      {"pushConstants", push_json},
      {"constantBuffers", cbv_json},
  };
}

[[nodiscard]] inline DrawConstantsSnapshot ExecuteConstantsRead(
    const ToolContext& context,
    std::uint32_t device_index,
    std::uint32_t draw_index,
    std::optional<std::uint32_t> slot_filter,
    std::optional<std::uint32_t> space_filter,
    std::uint32_t max_bytes) {
  auto request = context.prepare_constants_request(device_index, draw_index, slot_filter, space_filter, max_bytes);

  bool needs_gpu_readback = false;
  for (const auto& binding : request->bindings) {
    if (binding.cached_bytes.empty() && binding.buffer_handle != 0u) {
      needs_gpu_readback = true;
      break;
    }
  }

  if (!needs_gpu_readback) {
    DrawConstantsSnapshot snap = {
        .device_index = device_index,
        .draw_index = draw_index,
        .shader_hashes = request->shader_hashes,
        .push_constants = request->push_constants,
    };
    snap.constant_buffers.reserve(request->bindings.size());
    for (const auto& binding : request->bindings) {
      const auto clamped_size = std::min<std::size_t>(binding.cached_bytes.size(), max_bytes);
      snap.constant_buffers.push_back(ConstantBufferData{
          .slot = binding.slot,
          .space = binding.space,
          .buffer_handle_hex = binding.buffer_handle_hex,
          .offset = binding.offset,
          .requested_size = clamped_size,
          .used_by_active_shader = binding.used_by_active_shader,
          .source = "snapshot_cache",
          .bytes = std::vector<std::uint8_t>(binding.cached_bytes.begin(), binding.cached_bytes.begin() + static_cast<std::ptrdiff_t>(clamped_size)),
      });
    }
    return snap;
  }

  if (!context.enqueue_constants_request(device_index, request)) {
    throw std::runtime_error(std::format("Failed to queue constant buffer readback for device #{}.", device_index));
  }

  {
    std::unique_lock lock(request->mutex);
    if (!request->cv.wait_for(lock, std::chrono::seconds(15), [&request] { return request->completed || request->started; })) {
      request->canceled = true;
      lock.unlock();
      if (context.cancel_constants_request) {
        context.cancel_constants_request(device_index, request);
      }
      throw std::runtime_error("Timed out waiting for the next present to read back constant buffers.");
    }
    if (!request->completed) {
      request->cv.wait(lock, [&request] { return request->completed; });
    }
  }

  if (request->error.has_value()) {
    throw std::runtime_error(request->error.value());
  }
  if (!request->snapshot.has_value()) {
    throw std::runtime_error("Constant buffer readback completed without data.");
  }
  return request->snapshot.value();
}

}  // namespace internal

inline void ProcessPendingConstantsRequests(
    const std::deque<std::shared_ptr<PendingConstantsRequest>>& requests,
    reshade::api::device* device,
    reshade::api::command_queue* queue) {
  for (const auto& request : requests) {
    {
      std::scoped_lock lock(request->mutex);
      if (request->completed || request->canceled) {
        request->completed = true;
        request->cv.notify_all();
        continue;
      }
      request->started = true;
    }
    request->cv.notify_all();

    try {
      DrawConstantsSnapshot snap = {
          .device_index = request->device_index,
          .draw_index = request->draw_index,
          .shader_hashes = request->shader_hashes,
          .push_constants = request->push_constants,
      };
      snap.constant_buffers.reserve(request->bindings.size());

      for (const auto& binding : request->bindings) {
        if (!binding.cached_bytes.empty()) {
          const auto clamped = std::min<std::size_t>(binding.cached_bytes.size(), request->max_bytes);
          snap.constant_buffers.push_back(ConstantBufferData{
              .slot = binding.slot,
              .space = binding.space,
              .buffer_handle_hex = binding.buffer_handle_hex,
              .offset = binding.offset,
              .requested_size = clamped,
              .used_by_active_shader = binding.used_by_active_shader,
              .source = "snapshot_cache",
              .bytes = std::vector<std::uint8_t>(binding.cached_bytes.begin(), binding.cached_bytes.begin() + static_cast<std::ptrdiff_t>(clamped)),
          });
          continue;
        }

        ConstantBufferData cb_data = {
            .slot = binding.slot,
            .space = binding.space,
            .buffer_handle_hex = binding.buffer_handle_hex,
            .offset = binding.offset,
            .used_by_active_shader = binding.used_by_active_shader,
            .source = "live_gpu_readback",
        };

        const auto buffer = reshade::api::resource{binding.buffer_handle};
        // The handle comes from the snapshot; engines recycle constant buffers, so it may already be freed.
        // Ask the resource tracker instead of the API: touching a freed D3D11 buffer crashes the driver.
        reshade::api::resource_desc desc = {};
        const bool is_live = renodx::utils::resource::GetLiveResourceInfo(buffer, [&](const renodx::utils::resource::ResourceInfo& info) {
          desc = info.desc;
        });
        if (!is_live) {
          cb_data.error = "Bound constant buffer was destroyed since the snapshot; queue a new snapshot.";
          snap.constant_buffers.push_back(std::move(cb_data));
          continue;
        }
        if (desc.type != reshade::api::resource_type::buffer || desc.buffer.size == 0u || binding.offset >= desc.buffer.size) {
          cb_data.error = "Bound constant buffer is no longer valid or offset exceeds buffer size.";
          snap.constant_buffers.push_back(std::move(cb_data));
          continue;
        }

        const std::uint64_t available = desc.buffer.size - binding.offset;
        const std::uint64_t range_size = (binding.size == 0u || binding.size == UINT64_MAX) ? available : std::min(binding.size, available);
        const std::uint64_t read_size = std::min<std::uint64_t>(range_size, request->max_bytes);
        cb_data.requested_size = read_size;

        reshade::api::resource staging = {0u};
        if (!device->create_resource(
                reshade::api::resource_desc(
                    read_size,
                    reshade::api::memory_heap::gpu_to_cpu,
                    reshade::api::resource_usage::copy_dest),
                nullptr,
                reshade::api::resource_usage::copy_dest,
                &staging)) {
          cb_data.error = "Failed to allocate GPU-to-CPU staging buffer for constant buffer readback.";
          snap.constant_buffers.push_back(std::move(cb_data));
          continue;
        }

        auto* cmd_list = queue->get_immediate_command_list();
        cmd_list->copy_buffer_region(buffer, binding.offset, staging, 0u, read_size);
        queue->flush_immediate_command_list();
        queue->wait_idle();

        void* mapped_ptr = nullptr;
        if (device->map_buffer_region(staging, 0u, read_size, reshade::api::map_access::read_only, &mapped_ptr) && mapped_ptr != nullptr) {
          cb_data.bytes.resize(static_cast<std::size_t>(read_size));
          std::memcpy(cb_data.bytes.data(), mapped_ptr, static_cast<std::size_t>(read_size));
          device->unmap_buffer_region(staging);
        } else {
          cb_data.error = "Failed to map staging buffer for constant buffer readback.";
        }
        device->destroy_resource(staging);
        snap.constant_buffers.push_back(std::move(cb_data));
      }

      std::scoped_lock lock(request->mutex);
      request->snapshot = std::move(snap);
      request->completed = true;
    } catch (const std::exception& ex) {
      std::scoped_lock lock(request->mutex);
      request->error = ex.what();
      request->completed = true;
    }
    request->cv.notify_all();
  }
}

[[nodiscard]] inline ToolResult HandleGetDrawConstantsTool(const json& arguments, const ToolContext& context) {
  if (!context.resolve_device_index || !context.prepare_constants_request || !context.enqueue_constants_request) {
    throw std::runtime_error("DeepAnalysisToolContext is not initialized.");
  }

  const auto device_index = context.resolve_device_index(arguments);
  const auto draw_index = mcp_arguments::GetRequired<std::uint32_t>(arguments, "drawIndex");
  const auto slot_filter = mcp_arguments::GetOptional<std::uint32_t>(arguments, "slot");
  const auto space_filter = mcp_arguments::GetOptional<std::uint32_t>(arguments, "space");
  const auto max_bytes = mcp_arguments::ValidateRange(
      "maxBytes",
      mcp_arguments::GetOptional<std::uint32_t>(arguments, "maxBytes").value_or(4096u),
      16u,
      65536u);
  const auto save_as = mcp_arguments::GetOptional<std::string>(arguments, "saveAs");

  const auto snap = internal::ExecuteConstantsRead(context, device_index, draw_index, slot_filter, space_filter, max_bytes);

  if (save_as.has_value() && !save_as->empty()) {
    std::unique_lock lock(internal::g_saved_constants_mutex);
    internal::g_saved_constants[save_as.value()] = snap;
  }

  json result = internal::SerializeDrawConstants(snap);
  if (save_as.has_value() && !save_as->empty()) {
    result["savedAs"] = save_as.value();
  }

  auto text = std::format(
      "Read {} constant buffer(s) and {} push-constant range(s) for draw #{} on device #{}.",
      snap.constant_buffers.size(),
      snap.push_constants.size(),
      draw_index,
      device_index);
  if (save_as.has_value() && !save_as->empty()) {
    text += std::format(" Saved baseline as '{}'.", save_as.value());
  }
  return ToolResult{
      .text = text,
      .structured_content = result,
  };
}

[[nodiscard]] inline ToolResult HandleDiffDrawConstantsTool(const json& arguments, const ToolContext& context) {
  if (!context.resolve_device_index) {
    throw std::runtime_error("DeepAnalysisToolContext is not initialized.");
  }

  const auto baseline_name = mcp_arguments::GetRequired<std::string>(arguments, "baselineName");
  const auto compare_name = mcp_arguments::GetOptional<std::string>(arguments, "compareName");
  const auto draw_index = mcp_arguments::GetOptional<std::uint32_t>(arguments, "drawIndex");
  const auto epsilon = mcp_arguments::GetOptional<double>(arguments, "epsilon").value_or(1e-5);
  const auto max_bytes = mcp_arguments::ValidateRange(
      "maxBytes",
      mcp_arguments::GetOptional<std::uint32_t>(arguments, "maxBytes").value_or(4096u),
      16u,
      65536u);

  DrawConstantsSnapshot baseline = {};
  {
    std::shared_lock lock(internal::g_saved_constants_mutex);
    const auto it = internal::g_saved_constants.find(baseline_name);
    if (it == internal::g_saved_constants.end()) {
      throw std::runtime_error(std::format("No saved constant baseline named '{}'. Call devkit_get_draw_constants with saveAs first.", baseline_name));
    }
    baseline = it->second;
  }

  DrawConstantsSnapshot target = {};
  std::string target_label;
  if (compare_name.has_value() && !compare_name->empty()) {
    std::shared_lock lock(internal::g_saved_constants_mutex);
    const auto it = internal::g_saved_constants.find(compare_name.value());
    if (it == internal::g_saved_constants.end()) {
      throw std::runtime_error(std::format("No saved constant baseline named '{}'.", compare_name.value()));
    }
    target = it->second;
    target_label = compare_name.value();
  } else {
    const auto device_index = context.resolve_device_index(arguments);
    const auto target_draw = draw_index.value_or(baseline.draw_index);
    target = internal::ExecuteConstantsRead(context, device_index, target_draw, std::nullopt, std::nullopt, max_bytes);
    target_label = std::format("live_draw#{}", target_draw);
  }

  static constexpr const char* LANE_NAMES[4] = {"x", "y", "z", "w"};
  json cbv_diffs = json::array();
  for (const auto& base_cb : baseline.constant_buffers) {
    const ConstantBufferData* match_cb = nullptr;
    for (const auto& cand : target.constant_buffers) {
      if (cand.slot == base_cb.slot && cand.space == base_cb.space) {
        match_cb = &cand;
        break;
      }
    }
    if (match_cb == nullptr) continue;

    const auto base_dwords = internal::BytesToDwords(base_cb.bytes);
    const auto target_dwords = internal::BytesToDwords(match_cb->bytes);
    const auto common_count = std::min(base_dwords.size(), target_dwords.size());
    for (std::size_t i = 0u; i < common_count; ++i) {
      if (base_dwords[i] == target_dwords[i]) continue;
      const float f_before = internal::BitsToFloat(base_dwords[i]);
      const float f_after = internal::BitsToFloat(target_dwords[i]);
      const bool both_finite = std::isfinite(f_before) && std::isfinite(f_after);
      if (both_finite && std::abs(static_cast<double>(f_after) - static_cast<double>(f_before)) <= epsilon) {
        continue;
      }
      cbv_diffs.push_back(json{
          {"slot", base_cb.slot},
          {"space", base_cb.space},
          {"register", i / 4u},
          {"lane", LANE_NAMES[i % 4u]},
          {"hlslRef", std::format("cb{}[{}].{}", base_cb.slot, i / 4u, LANE_NAMES[i % 4u])},
          {"byteOffset", i * 4u},
          {"beforeFloat", internal::FloatOrNull(f_before)},
          {"afterFloat", internal::FloatOrNull(f_after)},
          {"deltaFloat", both_finite ? json(static_cast<double>(f_after) - static_cast<double>(f_before)) : json(nullptr)},
          {"beforeU32", base_dwords[i]},
          {"afterU32", target_dwords[i]},
      });
    }
  }

  json push_diffs = json::array();
  const auto common_push = std::min(baseline.push_constants.size(), target.push_constants.size());
  for (std::size_t p = 0u; p < common_push; ++p) {
    const auto& bp = baseline.push_constants[p];
    const auto& tp = target.push_constants[p];
    const auto common_vals = std::min(bp.values.size(), tp.values.size());
    for (std::size_t i = 0u; i < common_vals; ++i) {
      if (bp.values[i] == tp.values[i]) continue;
      const float f_before = internal::BitsToFloat(bp.values[i]);
      const float f_after = internal::BitsToFloat(tp.values[i]);
      const bool both_finite = std::isfinite(f_before) && std::isfinite(f_after);
      if (both_finite && std::abs(static_cast<double>(f_after) - static_cast<double>(f_before)) <= epsilon) {
        continue;
      }
      push_diffs.push_back(json{
          {"layoutParam", bp.layout_param},
          {"dwordIndex", bp.first + i},
          {"byteOffset", (bp.first + i) * 4u},
          {"beforeFloat", internal::FloatOrNull(f_before)},
          {"afterFloat", internal::FloatOrNull(f_after)},
          {"deltaFloat", both_finite ? json(static_cast<double>(f_after) - static_cast<double>(f_before)) : json(nullptr)},
          {"beforeU32", bp.values[i]},
          {"afterU32", tp.values[i]},
      });
    }
  }

  json result = {
      {"baselineName", baseline_name},
      {"target", target_label},
      {"epsilon", epsilon},
      {"cbvDiffCount", cbv_diffs.size()},
      {"pushConstantDiffCount", push_diffs.size()},
      {"cbvDiffs", cbv_diffs},
      {"pushConstantDiffs", push_diffs},
  };

  auto text = std::format(
      "Compared constants '{}' vs '{}': found {} CBV change(s) and {} push-constant change(s).",
      baseline_name,
      target_label,
      cbv_diffs.size(),
      push_diffs.size());
  return ToolResult{
      .text = text,
      .structured_content = result,
  };
}

[[nodiscard]] inline ToolResult HandleCompareSnapshotsTool(const json& arguments, const ToolContext& context) {
  if (!context.resolve_device_index || !context.capture_snapshot_profile) {
    throw std::runtime_error("DeepAnalysisToolContext is not initialized.");
  }

  const auto action = mcp_arguments::GetRequired<std::string>(arguments, "action");
  if (action == "list") {
    json names = json::array();
    {
      std::shared_lock lock(internal::g_saved_profiles_mutex);
      for (const auto& [name, profile] : internal::g_saved_profiles) {
        names.push_back(json{
            {"name", name},
            {"deviceIndex", profile.device_index},
            {"totalDraws", profile.total_draws},
            {"shaderCount", profile.shader_draw_counts.size()},
            {"swapchainFormat", profile.swapchain_format},
            {"swapchainColorSpace", profile.swapchain_color_space},
        });
      }
    }
    return ToolResult{
        .text = std::format("{} saved snapshot profile(s) available.", names.size()),
        .structured_content = json{{"savedSnapshots", names}},
    };
  }

  if (action == "save") {
    const auto name = mcp_arguments::GetRequired<std::string>(arguments, "name");
    const auto device_index = context.resolve_device_index(arguments);
    const auto profile = context.capture_snapshot_profile(device_index);
    {
      std::unique_lock lock(internal::g_saved_profiles_mutex);
      internal::g_saved_profiles[name] = profile;
    }
    return ToolResult{
        .text = std::format(
            "Saved snapshot profile '{}' for device #{} ({} draws, {} unique shaders, swapchain {} / {}).",
            name,
            device_index,
            profile.total_draws,
            profile.shader_draw_counts.size(),
            profile.swapchain_format,
            profile.swapchain_color_space),
        .structured_content = json{
            {"saved", true},
            {"name", name},
            {"deviceIndex", device_index},
            {"totalDraws", profile.total_draws},
            {"shaderCount", profile.shader_draw_counts.size()},
            {"swapchainFormat", profile.swapchain_format},
            {"swapchainColorSpace", profile.swapchain_color_space},
            {"outputChainShaders", profile.output_chain_shaders},
        },
    };
  }

  if (action == "compare") {
    const auto baseline_name = mcp_arguments::GetRequired<std::string>(arguments, "baselineName");
    const auto target_name = mcp_arguments::GetOptional<std::string>(arguments, "targetName");

    SnapshotProfile baseline = {};
    {
      std::shared_lock lock(internal::g_saved_profiles_mutex);
      const auto it = internal::g_saved_profiles.find(baseline_name);
      if (it == internal::g_saved_profiles.end()) {
        throw std::runtime_error(std::format("No saved snapshot profile named '{}'. Save one with action='save' first.", baseline_name));
      }
      baseline = it->second;
    }

    SnapshotProfile target = {};
    std::string target_label = "current_snapshot";
    if (target_name.has_value() && !target_name->empty()) {
      std::shared_lock lock(internal::g_saved_profiles_mutex);
      const auto it = internal::g_saved_profiles.find(target_name.value());
      if (it == internal::g_saved_profiles.end()) {
        throw std::runtime_error(std::format("No saved snapshot profile named '{}'.", target_name.value()));
      }
      target = it->second;
      target_label = target_name.value();
    } else {
      const auto device_index = context.resolve_device_index(arguments);
      target = context.capture_snapshot_profile(device_index);
    }

    json added_shaders = json::array();
    json removed_shaders = json::array();
    json draw_count_changes = json::array();

    for (const auto& [hash, count] : target.shader_draw_counts) {
      const auto it = baseline.shader_draw_counts.find(hash);
      if (it == baseline.shader_draw_counts.end()) {
        added_shaders.push_back(json{{"shaderHash", hash}, {"drawCount", count}});
      } else if (it->second != count) {
        draw_count_changes.push_back(json{
            {"shaderHash", hash},
            {"before", it->second},
            {"after", count},
            {"delta", static_cast<std::int64_t>(count) - static_cast<std::int64_t>(it->second)},
        });
      }
    }
    for (const auto& [hash, count] : baseline.shader_draw_counts) {
      if (!target.shader_draw_counts.contains(hash)) {
        removed_shaders.push_back(json{{"shaderHash", hash}, {"drawCount", count}});
      }
    }

    json added_formats = json::array();
    json removed_formats = json::array();
    for (const auto& fmt : target.target_formats) {
      if (!baseline.target_formats.contains(fmt)) added_formats.push_back(fmt);
    }
    for (const auto& fmt : baseline.target_formats) {
      if (!target.target_formats.contains(fmt)) removed_formats.push_back(fmt);
    }

    // Chains are ordered from the final output pass upward; the divergence index counts from that pass.
    const auto [chain_before_mismatch, chain_after_mismatch] =
        std::ranges::mismatch(baseline.output_chain_shaders, target.output_chain_shaders);
    const bool chains_differ =
        chain_before_mismatch != baseline.output_chain_shaders.end() || chain_after_mismatch != target.output_chain_shaders.end();
    const std::set<std::string> chain_before_set(baseline.output_chain_shaders.begin(), baseline.output_chain_shaders.end());
    const std::set<std::string> chain_after_set(target.output_chain_shaders.begin(), target.output_chain_shaders.end());
    json chain_added = json::array();
    json chain_removed = json::array();
    for (const auto& hash : chain_after_set) {
      if (!chain_before_set.contains(hash)) chain_added.push_back(hash);
    }
    for (const auto& hash : chain_before_set) {
      if (!chain_after_set.contains(hash)) chain_removed.push_back(hash);
    }

    const bool swapchain_changed =
        baseline.swapchain_format != target.swapchain_format
        || baseline.swapchain_color_space != target.swapchain_color_space
        || baseline.swapchain_width != target.swapchain_width
        || baseline.swapchain_height != target.swapchain_height;

    json result = {
        {"baselineName", baseline_name},
        {"target", target_label},
        {"totalDrawsBefore", baseline.total_draws},
        {"totalDrawsAfter", target.total_draws},
        {"swapchainChanged", swapchain_changed},
        {"swapchainBefore", {
                                {"format", baseline.swapchain_format},
                                {"colorSpace", baseline.swapchain_color_space},
                                {"width", baseline.swapchain_width},
                                {"height", baseline.swapchain_height},
                            }},
        {"swapchainAfter", {
                               {"format", target.swapchain_format},
                               {"colorSpace", target.swapchain_color_space},
                               {"width", target.swapchain_width},
                               {"height", target.swapchain_height},
                           }},
        {"addedShaders", added_shaders},
        {"removedShaders", removed_shaders},
        {"drawCountChanges", draw_count_changes},
        {"addedTargetFormats", added_formats},
        {"removedTargetFormats", removed_formats},
        {"outputChainBefore", baseline.output_chain_shaders},
        {"outputChainAfter", target.output_chain_shaders},
        {"firstDivergenceIndex", chains_differ ? json(chain_before_mismatch - baseline.output_chain_shaders.begin()) : json(nullptr)},
        {"outputChainAdded", chain_added},
        {"outputChainRemoved", chain_removed},
        {"uiStartDrawBefore", baseline.ui_start_draw_index.has_value() ? json(baseline.ui_start_draw_index.value()) : json(nullptr)},
        {"uiStartDrawAfter", target.ui_start_draw_index.has_value() ? json(target.ui_start_draw_index.value()) : json(nullptr)},
    };

    auto text = std::format(
        "Compared snapshots '{}' vs '{}': +{} added shader(s), -{} removed shader(s), {} draw-count change(s), swapchainChanged={}.",
        baseline_name,
        target_label,
        added_shaders.size(),
        removed_shaders.size(),
        draw_count_changes.size(),
        swapchain_changed ? "true" : "false");
    return ToolResult{
        .text = text,
        .structured_content = result,
    };
  }

  throw std::runtime_error("action must be 'save', 'list', or 'compare'.");
}

[[nodiscard]] inline ToolResult HandleQueueTraceTool(const json& arguments, const ToolContext& context) {
  if (!context.resolve_device_index || !context.queue_trace) {
    throw std::runtime_error("DeepAnalysisToolContext is not initialized.");
  }

  const auto device_index = context.resolve_device_index(arguments);
  const auto return_to_idle = mcp_arguments::GetOptional<bool>(arguments, "returnToIdle");
  return context.queue_trace(device_index, return_to_idle);
}

[[nodiscard]] inline ToolResult HandleGetTraceLogTool(const json& arguments) {
  const auto filter = mcp_arguments::GetOptional<std::string>(arguments, "filter");
  const auto limit = mcp_arguments::ValidateRange(
      "limit",
      mcp_arguments::GetOptional<std::uint32_t>(arguments, "limit").value_or(200u),
      1u,
      5000u);
  const auto offset = mcp_arguments::ValidateRange(
      "offset",
      mcp_arguments::GetOptional<std::uint32_t>(arguments, "offset").value_or(0u),
      0u,
      (std::numeric_limits<std::uint32_t>::max)());

  const auto snapshot = renodx::utils::trace::GetCapturedTraceSnapshot();
  const auto lowered_filter = filter.has_value() ? internal::ToLowerAscii(filter.value()) : std::string{};

  // Index into the captured trace, so entries stay addressable after filtering.
  std::vector<std::size_t> matched;
  matched.reserve(snapshot.lines.size());
  for (std::size_t line_index = 0u; line_index < snapshot.lines.size(); ++line_index) {
    if (!lowered_filter.empty() && internal::ToLowerAscii(snapshot.lines[line_index]).find(lowered_filter) == std::string::npos) {
      continue;
    }
    matched.push_back(line_index);
  }

  const auto total_matched = matched.size();
  const auto clamped_offset = std::min<std::size_t>(offset, total_matched);
  const auto end_index = std::min<std::size_t>(clamped_offset + limit, total_matched);
  const auto structured = mcp_arguments::GetOptional<bool>(arguments, "structured").value_or(true);
  json paged = json::array();
  for (std::size_t i = clamped_offset; i < end_index; ++i) {
    const auto& line = snapshot.lines[matched[i]];
    if (!structured) {
      paged.push_back(line);
      continue;
    }
    // Trace lines look like `event(arg, arg, ...)`; anything else is kept whole as the event.
    const auto open_paren = line.find('(');
    const auto close_paren = line.rfind(')');
    const bool has_args = open_paren != std::string::npos && close_paren != std::string::npos && close_paren > open_paren;
    paged.push_back(json{
        {"index", matched[i]},
        {"event", has_args ? line.substr(0u, open_paren) : line},
        {"args", has_args ? line.substr(open_paren + 1u, close_paren - open_paren - 1u) : std::string{}},
    });
  }

  json result = {
      {"generation", snapshot.generation},
      {"captureRunning", renodx::utils::trace::trace_capture_to_memory.load()},
      {"truncated", snapshot.truncated},
      {"totalCapturedLines", snapshot.lines.size()},
      {"totalMatchedLines", total_matched},
      {"offset", clamped_offset},
      {"limit", limit},
      {"returned", paged.size()},
      {structured ? "entries" : "lines", paged},
  };
  if (filter.has_value()) {
    result["filter"] = filter.value();
  }

  auto text = std::format(
      "Returned {} trace line(s) (offset {} of {} matched; {} total captured, generation {}).",
      paged.size(),
      clamped_offset,
      total_matched,
      snapshot.lines.size(),
      snapshot.generation);
  return ToolResult{
      .text = text,
      .structured_content = result,
  };
}

}  // namespace renodx::addons::devkit::mcp::deep_analysis
