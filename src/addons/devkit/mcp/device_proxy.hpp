/*
 * Copyright (C) 2026 Carlos Lopez
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <algorithm>
#include <cctype>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

#include "../../../utils/mcp/arguments.hpp"
#include "../../../utils/mcp/types.hpp"

namespace renodx::addons::devkit::mcp::device_proxy {

using json = renodx::utils::mcp::json;
using ToolResult = renodx::utils::mcp::ToolResult;
namespace mcp_arguments = renodx::utils::mcp::arguments;

struct DeviceProxyState {
  std::uint32_t device_index = 0u;
  std::string api;
  std::string mode;
  std::string output_mode;
  std::uint32_t output_mode_index = 2u;
  std::string target_format;
  std::string intermediate_format;
  std::string target_color_space;
  std::string state;
  bool shared_proxy_active = false;
  bool teardown_pending = false;
  bool reenable_pending = false;
  bool external_proxy_active = false;
  std::string hwnd_route;
  bool separate_hwnd_forced = false;
  bool mode_locked = false;
  bool proxy_device_ready = false;
  bool proxy_swapchain_ready = false;
  std::string proxy_hwnd_override;
  bool output_window_created = false;
  bool primary_swapchain_is_flip = false;
  bool is_d3d9 = false;
  bool is_d3d9_ex = false;
  bool controls_blocked = false;
  std::string dx9_ex_upgrade_state;
  bool proxy_wait_idle = false;
  bool force_dx9_ex = false;
  bool force_disable_flip = false;
  bool use_custom_shaders = false;
  bool custom_shaders_ready = false;
  std::size_t custom_vertex_shader_bytes = 0u;
  std::size_t custom_pixel_shader_bytes = 0u;
  std::optional<std::string> custom_shader_compile_error = std::nullopt;
  std::string custom_vertex_shader_file;
  std::string custom_pixel_shader_file;
};

// NOLINTNEXTLINE(readability-identifier-naming)
inline void to_json(json& j, const DeviceProxyState& value) {
  j = {
      {"deviceIndex", value.device_index},
      {"api", value.api},
      {"mode", value.mode},
      {"outputMode", value.output_mode},
      {"outputModeIndex", value.output_mode_index},
      {"targetFormat", value.target_format},
      {"intermediateFormat", value.intermediate_format},
      {"targetColorSpace", value.target_color_space},
      {"state", value.state},
      {"sharedProxyActive", value.shared_proxy_active},
      {"teardownPending", value.teardown_pending},
      {"reenablePending", value.reenable_pending},
      {"externalProxyActive", value.external_proxy_active},
      {"hwndRoute", value.hwnd_route},
      {"separateHwndForced", value.separate_hwnd_forced},
      {"modeLocked", value.mode_locked},
      {"proxyDeviceReady", value.proxy_device_ready},
      {"proxySwapchainReady", value.proxy_swapchain_ready},
      {"proxyHwndOverride", value.proxy_hwnd_override},
      {"outputWindowCreated", value.output_window_created},
      {"primarySwapchainIsFlip", value.primary_swapchain_is_flip},
      {"isD3D9", value.is_d3d9},
      {"isD3D9Ex", value.is_d3d9_ex},
      {"controlsBlocked", value.controls_blocked},
      {"dx9ExUpgradeState", value.dx9_ex_upgrade_state},
      {"proxyWaitIdle", value.proxy_wait_idle},
      {"forceDx9Ex", value.force_dx9_ex},
      {"forceDisableFlip", value.force_disable_flip},
      {"useCustomShaders", value.use_custom_shaders},
      {"customShadersReady", value.custom_shaders_ready},
      {"customVertexShaderBytes", value.custom_vertex_shader_bytes},
      {"customPixelShaderBytes", value.custom_pixel_shader_bytes},
      {"customShaderCompileError",
       value.custom_shader_compile_error.has_value()
           ? json(value.custom_shader_compile_error.value())
           : json(nullptr)},
      {"customVertexShaderFile", value.custom_vertex_shader_file},
      {"customPixelShaderFile", value.custom_pixel_shader_file},
  };
}

struct DeviceProxyUpdateParams {
  std::optional<std::string> mode = std::nullopt;
  std::optional<std::string> output_mode = std::nullopt;
  std::optional<bool> proxy_wait_idle = std::nullopt;
  std::optional<bool> use_custom_shaders = std::nullopt;
  std::optional<bool> compile_custom_shaders = std::nullopt;
  std::optional<bool> force_dx9_ex = std::nullopt;
  std::optional<bool> force_disable_flip = std::nullopt;
};

struct PendingDeviceProxyRequest {
  std::uint32_t device_index = 0u;
  DeviceProxyUpdateParams params = {};

  std::mutex mutex;
  std::condition_variable cv;
  bool started = false;
  bool canceled = false;
  bool completed = false;
  std::optional<ToolResult> result = std::nullopt;
};

struct ToolContext {
  std::function<std::uint32_t(const json& arguments)> resolve_device_index;
  std::function<DeviceProxyState(std::uint32_t device_index)> get_state;
  std::function<bool(std::uint32_t device_index, const std::shared_ptr<PendingDeviceProxyRequest>& request)>
      enqueue_update_request;
  std::function<void(std::uint32_t device_index, const std::shared_ptr<PendingDeviceProxyRequest>& request)>
      cancel_update_request;
};

[[nodiscard]] inline ToolResult HandleGetDeviceProxyStateTool(const json& arguments, const ToolContext& context) {
  if (!context.resolve_device_index || !context.get_state) {
    throw std::runtime_error("DeviceProxyToolContext is not initialized.");
  }

  const auto device_index = context.resolve_device_index(arguments);
  const auto state = context.get_state(device_index);
  auto text = std::format(
      "Device #{} ({}) swapchain proxy is {} (mode={}, outputMode={}, target={} / {}, route={}).",
      device_index,
      state.api,
      state.state,
      state.mode,
      state.output_mode,
      state.target_format,
      state.target_color_space,
      state.hwnd_route);
  return ToolResult{
      .text = text,
      .structured_content = state,
  };
}

[[nodiscard]] inline ToolResult HandleSetDeviceProxyModeTool(const json& arguments, const ToolContext& context) {
  if (!context.resolve_device_index || !context.enqueue_update_request || !context.cancel_update_request) {
    throw std::runtime_error("DeviceProxyToolContext is not initialized.");
  }

  const auto to_lower_ascii = [](std::string_view value) {
    std::string lowered;
    lowered.reserve(value.size());
    for (const auto character : value) {
      lowered.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(character))));
    }
    return lowered;
  };

  DeviceProxyUpdateParams params = {
      .mode = mcp_arguments::GetOptional<std::string>(arguments, "mode"),
      .output_mode = mcp_arguments::GetOptional<std::string>(arguments, "outputMode"),
      .proxy_wait_idle = mcp_arguments::GetOptional<bool>(arguments, "proxyWaitIdle"),
      .use_custom_shaders = mcp_arguments::GetOptional<bool>(arguments, "useCustomShaders"),
      .compile_custom_shaders = mcp_arguments::GetOptional<bool>(arguments, "compileCustomShaders"),
      .force_dx9_ex = mcp_arguments::GetOptional<bool>(arguments, "forceDx9Ex"),
      .force_disable_flip = mcp_arguments::GetOptional<bool>(arguments, "forceDisableFlip"),
  };

  if (params.mode.has_value()) {
    auto normalized = to_lower_ascii(params.mode.value());
    if (normalized == "off" || normalized == "disabled") {
      normalized = "none";
    }
    if (normalized != "none" && normalized != "current_window" && normalized != "new_window") {
      throw std::runtime_error(
          std::format("mode '{}' is invalid. Expected 'none', 'current_window', or 'new_window'.", params.mode.value()));
    }
    params.mode = std::move(normalized);
  }

  if (params.output_mode.has_value()) {
    auto normalized = to_lower_ascii(params.output_mode.value());
    if (normalized == "sdr") {
      normalized = "srgb";
    }
    if (normalized != "srgb" && normalized != "hdr10" && normalized != "scrgb") {
      throw std::runtime_error(
          std::format("outputMode '{}' is invalid. Expected 'srgb', 'hdr10', or 'scrgb'.", params.output_mode.value()));
    }
    params.output_mode = std::move(normalized);
  }

  if (!params.mode.has_value()
      && !params.output_mode.has_value()
      && !params.proxy_wait_idle.has_value()
      && !params.use_custom_shaders.has_value()
      && !params.compile_custom_shaders.has_value()
      && !params.force_dx9_ex.has_value()
      && !params.force_disable_flip.has_value()) {
    throw std::runtime_error(
        "Specify at least one setting (mode, outputMode, proxyWaitIdle, useCustomShaders, compileCustomShaders, forceDx9Ex, or forceDisableFlip).");
  }

  const auto device_index = context.resolve_device_index(arguments);
  auto request = std::make_shared<PendingDeviceProxyRequest>();
  request->device_index = device_index;
  request->params = std::move(params);

  if (!context.enqueue_update_request(device_index, request)) {
    throw std::runtime_error(std::format("Device #{} is no longer available.", device_index));
  }

  std::unique_lock request_lock(request->mutex);
  const bool finished = request->cv.wait_for(
      request_lock,
      std::chrono::seconds(5),
      [&request]() { return request->completed; });
  if (!finished) {
    request->canceled = true;
    request_lock.unlock();
    context.cancel_update_request(device_index, request);
    return ToolResult{
        .text = std::format(
            "Timed out waiting for device #{} to present and apply device proxy changes.",
            device_index),
        .structured_content = json{
            {"deviceIndex", device_index},
            {"error", "Timed out waiting for present to apply device proxy changes."},
        },
        .is_error = true,
    };
  }

  if (request->result.has_value()) {
    return request->result.value();
  }

  return ToolResult{
      .text = "Device proxy update completed without a result.",
      .structured_content = json{
          {"deviceIndex", device_index},
          {"error", "Device proxy update completed without a result."},
      },
      .is_error = true,
  };
}

}  // namespace renodx::addons::devkit::mcp::device_proxy
