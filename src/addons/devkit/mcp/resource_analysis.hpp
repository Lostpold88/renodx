/*
 * Copyright (C) 2026 Carlos Lopez
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <deque>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <iomanip>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <include/reshade.hpp>

#include "../../../utils/exr.hpp"
#include "../../../utils/mcp/types.hpp"
#include "../../../utils/mcp/arguments.hpp"
#include "../../../utils/png.hpp"
#include "../../../utils/resource.hpp"
#include "resource_handle.hpp"
#include "resource_view_summary.hpp"

namespace renodx::addons::devkit::mcp::resource_analysis {

using json = renodx::utils::mcp::json;
using ToolResult = renodx::utils::mcp::ToolResult;
namespace mcp_arguments = renodx::utils::mcp::arguments;

enum class PreviewMode : std::uint8_t {
  CLAMPED = 0,
  NEUTWO = 1,
};

enum class OutputFormat : std::uint8_t {
  PNG = 0,
  EXR = 1,
};

struct ScanTargetSpec {
  reshade::api::resource_view resource_view = {0u};
  reshade::api::resource resource = {0u};
  std::optional<std::uint32_t> draw_index = std::nullopt;
  std::vector<std::string> shader_hashes;
  std::string label;
};

struct LutAnalysisSpec {
  std::string layout = "auto";
  std::optional<std::uint32_t> lut_size = std::nullopt;
  std::optional<std::filesystem::path> export_cube_path = std::nullopt;
  std::optional<std::filesystem::path> export_json_path = std::nullopt;
  std::optional<std::filesystem::path> export_preview_path = std::nullopt;
};

struct AbCompareSpec {
  bool active = false;
  std::uint8_t phase = 0u;
  bool suspended_live_shaders = false;
  std::optional<ToolResult> with_live_result = std::nullopt;
};

struct PendingRequest {
  reshade::api::resource_view resource_view = {0u};
  reshade::api::resource resource = {0u};
  std::optional<std::vector<ScanTargetSpec>> batch_targets = std::nullopt;
  std::optional<LutAnalysisSpec> lut_analysis = std::nullopt;
  std::optional<AbCompareSpec> ab_compare = std::nullopt;
  std::optional<std::filesystem::path> output_path = std::nullopt;
  std::optional<std::filesystem::path> output_directory = std::nullopt;
  std::optional<std::string> name_prefix = std::nullopt;
  PreviewMode preview_mode = PreviewMode::CLAMPED;
  std::string preview_mode_name = "clamped";
  bool prefer_clone = false;
  std::mutex mutex;
  std::condition_variable cv;
  bool started = false;
  bool canceled = false;
  bool completed = false;
  std::optional<ToolResult> result = std::nullopt;
};

struct ToolContext {
  std::function<std::uint32_t(const json& arguments)> resolve_device_index;
  std::function<bool(std::uint32_t device_index, const std::shared_ptr<PendingRequest>& request)> enqueue_request;
  std::function<void(std::uint32_t device_index, const std::shared_ptr<PendingRequest>& request)> cancel_request;
  std::function<std::vector<ScanTargetSpec>(
      std::uint32_t device_index,
      const std::vector<std::uint32_t>& draw_indices,
      bool include_output_chain)>
      resolve_scan_targets;
};

struct ProcessingContext {
  std::function<std::string(std::uint64_t value)> format_handle;
  std::function<std::string(reshade::api::format format)> format_format;
  std::function<resource_view_summary::ResourceViewSummary(reshade::api::resource_view resource_view, reshade::api::device* device)> build_resource_view_summary;
  // True for a backbuffer whose swapchain color space is HDR10 (PQ-encoded BT.2020).
  std::function<bool(reshade::api::device* device, reshade::api::resource resource)> is_hdr10_pq_backbuffer;
  std::function<bool(reshade::api::device* device)> suspend_live_shaders;
  std::function<void(reshade::api::device* device)> restore_live_shaders;
  std::function<void(std::uint32_t device_index, const std::shared_ptr<PendingRequest>& request)> requeue_request;
};

namespace internal {

[[nodiscard]] inline std::string FormatHandle(const ProcessingContext& context, std::uint64_t value) {
  if (context.format_handle) {
    return context.format_handle(value);
  }
  return std::format("0x{:016X}", value);
}

[[nodiscard]] inline std::string FormatFormat(const ProcessingContext& context, reshade::api::format format) {
  if (context.format_format) {
    return context.format_format(format);
  }
  return std::format("{}", static_cast<std::uint32_t>(format));
}

[[nodiscard]] inline std::optional<renodx::utils::resource::ResourceViewInfo> FindTrackedResourceViewInfo(reshade::api::resource_view resource_view) {
  if (resource_view.handle == 0u) return std::nullopt;
  std::optional<renodx::utils::resource::ResourceViewInfo> result = std::nullopt;
  renodx::utils::resource::GetResourceViewInfo(resource_view, [&result](const renodx::utils::resource::ResourceViewInfo& info) {
    result = info;
  });
  return result;
}

struct AnalysisTarget {
  reshade::api::resource_view requested_view = {0u};
  reshade::api::resource_view resolved_view = {0u};
  reshade::api::resource_view_desc resolved_view_desc;
  reshade::api::resource resolved_resource = {0u};
  reshade::api::resource_desc resolved_resource_desc;
  bool used_clone = false;
};

struct ScalarStats {
  std::optional<double> minimum = std::nullopt;
  std::optional<double> maximum = std::nullopt;
  std::optional<double> mean = std::nullopt;

  ScalarStats() = default;

  ScalarStats(double minimum_value, double maximum_value, double sum, std::uint64_t count) {
    if (count == 0u) {
      return;
    }

    minimum = minimum_value;
    maximum = maximum_value;
    mean = sum / static_cast<double>(count);
  }
};

// NOLINTNEXTLINE(readability-identifier-naming)
inline void to_json(json& j, const ScalarStats& value) {
  j = {
      {"min", value.minimum.has_value() ? json(value.minimum.value()) : json(nullptr)},
      {"max", value.maximum.has_value() ? json(value.maximum.value()) : json(nullptr)},
      {"mean", value.mean.has_value() ? json(value.mean.value()) : json(nullptr)},
  };
}

struct RatioStats {
  std::uint64_t count = 0u;
  double ratio = 0.0;

  RatioStats() = default;

  RatioStats(std::uint64_t matching_count, std::uint64_t total_count)
      : count(matching_count),
        ratio(total_count == 0u ? 0.0 : static_cast<double>(matching_count) / static_cast<double>(total_count)) {}
};

// NOLINTNEXTLINE(readability-identifier-naming)
inline void to_json(json& j, const RatioStats& value) {
  j = {
      {"count", value.count},
      {"ratio", value.ratio},
  };
}

[[nodiscard]] inline AnalysisTarget ResolveAnalysisTarget(
    reshade::api::resource_view resource_view,
    reshade::api::resource raw_resource,
    reshade::api::device* device,
    bool prefer_clone) {
  if (resource_view.handle == 0u && raw_resource.handle != 0u) {
    reshade::api::resource resolved_resource = {0u};
    reshade::api::resource_desc resolved_resource_desc = {};
    bool clone_enabled = false;
    reshade::api::resource clone_resource = {0u};
    reshade::api::resource_desc clone_resource_desc = {};
    bool resource_destroyed = false;
    reshade::api::device* resource_device = nullptr;
    const auto found_resource_info = renodx::utils::resource::GetResourceInfo(raw_resource, [&](const renodx::utils::resource::ResourceInfo& info) {
      resolved_resource = info.resource;
      resolved_resource_desc = info.desc;
      clone_enabled = info.clone_enabled;
      clone_resource = info.clone;
      clone_resource_desc = info.clone_desc;
      resource_destroyed = info.destroyed;
      resource_device = info.device;
    });
    if (!found_resource_info || resource_destroyed) {
      throw std::runtime_error(std::format("resourceHandle {} is not currently tracked.", raw_resource.handle));
    }
    if (resource_device != device) {
      throw std::runtime_error("resourceHandle does not belong to the selected device.");
    }

    reshade::api::resource_view_desc synthetic_view_desc = {};
    synthetic_view_desc.type = (resolved_resource_desc.type == reshade::api::resource_type::texture_3d)
                                   ? reshade::api::resource_view_type::texture_3d
                                   : reshade::api::resource_view_type::texture_2d;
    synthetic_view_desc.format = resolved_resource_desc.texture.format;
    synthetic_view_desc.texture.first_level = 0u;
    synthetic_view_desc.texture.level_count = 1u;
    synthetic_view_desc.texture.first_layer = 0u;
    synthetic_view_desc.texture.layer_count = 1u;

    AnalysisTarget target = {
        .requested_view = {0u},
        .resolved_view = {0u},
        .resolved_view_desc = synthetic_view_desc,
        .resolved_resource = resolved_resource,
        .resolved_resource_desc = resolved_resource_desc,
        .used_clone = false,
    };

    if (prefer_clone) {
      if (!clone_enabled) {
        throw std::runtime_error("preferClone was requested, but clone hotswap is not enabled for the backing resource.");
      }
      if (clone_resource.handle == 0u) {
        throw std::runtime_error("preferClone was requested, but the clone resource is not currently instantiated.");
      }
      target.resolved_resource = clone_resource;
      target.resolved_resource_desc = clone_resource_desc;
      target.resolved_view_desc.format = clone_resource_desc.texture.format;
      target.used_clone = true;
    }

    return target;
  }

  const auto resource_view_info = FindTrackedResourceViewInfo(resource_view);
  if (!resource_view_info.has_value() || resource_view_info->destroyed) {
    throw std::runtime_error(std::format("resourceViewHandle {} is not currently tracked.", resource_view.handle));
  }
  if (resource_view_info->device != device) {
    throw std::runtime_error("resourceViewHandle does not belong to the selected device.");
  }

  reshade::api::resource resolved_resource = {0u};
  reshade::api::resource_desc resolved_resource_desc = {};
  bool clone_enabled = false;
  reshade::api::resource clone_resource = {0u};
  reshade::api::resource_desc clone_resource_desc = {};
  bool resource_destroyed = false;
  // A clone view describes the clone resource, so reading the original resource with that view's format overruns the readback.
  const auto backing_resource = (resource_view_info->is_clone ? resource_view_info->clone_resource : resource_view_info->original_resource);
  const auto found_resource_info = renodx::utils::resource::GetResourceInfo(backing_resource, [&](const renodx::utils::resource::ResourceInfo& info) {
    resolved_resource = info.resource;
    resolved_resource_desc = info.desc;
    clone_enabled = info.clone_enabled;
    clone_resource = info.clone;
    clone_resource_desc = info.clone_desc;
    resource_destroyed = info.destroyed;
  });
  if (!found_resource_info || resource_destroyed) {
    throw std::runtime_error("The selected resource view points to a destroyed resource.");
  }

  AnalysisTarget target = {
      .requested_view = resource_view,
      .resolved_view = resource_view,
      .resolved_view_desc = resource_view_info->desc,
      .resolved_resource = resolved_resource,
      .resolved_resource_desc = resolved_resource_desc,
      .used_clone = false,
  };

  if (prefer_clone) {
    if (!clone_enabled) {
      throw std::runtime_error("preferClone was requested, but clone hotswap is not enabled for the backing resource.");
    }
    if (resource_view_info->clone.handle == 0u || clone_resource.handle == 0u) {
      throw std::runtime_error("preferClone was requested, but the clone view/resource is not currently instantiated. Render a frame after enabling clone hotswap and try again.");
    }

    target.resolved_view = resource_view_info->clone;
    target.resolved_view_desc = resource_view_info->clone_desc;
    target.resolved_resource = clone_resource;
    target.resolved_resource_desc = clone_resource_desc;
    target.used_clone = true;
  }

  return target;
}

[[nodiscard]] inline float DecodeFloat16(uint16_t value) {
  const auto sign = static_cast<int>((value >> 15) & 0x1u);
  const auto exponent = static_cast<int>((value >> 10) & 0x1Fu);
  const auto mantissa = static_cast<uint32_t>(value & 0x03FFu);

  float decoded = 0.f;
  if (exponent == 0) {
    if (mantissa != 0u) {
      decoded = std::ldexp(static_cast<float>(mantissa), -24);
    }
  } else if (exponent == 0x1F) {
    decoded = mantissa == 0u
                  ? std::numeric_limits<float>::infinity()
                  : std::numeric_limits<float>::quiet_NaN();
  } else {
    decoded = std::ldexp(1.f + (static_cast<float>(mantissa) / 1024.f), exponent - 15);
  }

  return sign != 0 ? -decoded : decoded;
}

[[nodiscard]] inline float DecodeUnsignedFloat(uint32_t value, uint32_t mantissa_bits) {
  const auto exponent = static_cast<int>(value >> mantissa_bits);
  const auto mantissa = value & ((1u << mantissa_bits) - 1u);

  if (exponent == 0) {
    if (mantissa == 0u) return 0.f;
    return std::ldexp(static_cast<float>(mantissa), -14 - static_cast<int>(mantissa_bits));
  }
  if (exponent == 0x1F) {
    return mantissa == 0u
               ? std::numeric_limits<float>::infinity()
               : std::numeric_limits<float>::quiet_NaN();
  }

  return std::ldexp(1.f + (static_cast<float>(mantissa) / static_cast<float>(1u << mantissa_bits)), exponent - 15);
}

[[nodiscard]] inline float DecodeR11Component(uint32_t packed) {
  return DecodeUnsignedFloat(packed & 0x7FFu, 6u);
}

[[nodiscard]] inline float DecodeR10Component(uint32_t packed) {
  return DecodeUnsignedFloat(packed & 0x3FFu, 5u);
}

[[nodiscard]] inline float SrgbToLinear(float value) {
  if (value <= 0.04045f) {
    return value / 12.92f;
  }
  return std::pow((value + 0.055f) / 1.055f, 2.4f);
}

[[nodiscard]] inline float LinearToSrgb(float value) {
  if (value <= 0.0031308f) {
    return value * 12.92f;
  }
  return (1.055f * std::pow(value, 1.f / 2.4f)) - 0.055f;
}

[[nodiscard]] inline std::string_view GetPreviewModeName(PreviewMode preview_mode) {
  switch (preview_mode) {
    case PreviewMode::CLAMPED: return "clamped";
    case PreviewMode::NEUTWO:  return "neutwo";
    default:                   return "clamped";
  }
}

[[nodiscard]] inline OutputFormat GetOutputFormat(const std::filesystem::path& output_path) {
  return output_path.extension() == L".exr" ? OutputFormat::EXR : OutputFormat::PNG;
}

[[nodiscard]] inline std::string FormatCrc32(uint32_t crc32) {
  return std::format("0x{:08X}", crc32);
}

[[nodiscard]] inline std::string SanitizeFileStem(std::string_view value) {
  std::string result = {};
  result.reserve(value.size());

  bool previous_was_separator = false;
  for (const char character : value) {
    const bool is_alphanumeric = (character >= '0' && character <= '9')
                                 || (character >= 'A' && character <= 'Z')
                                 || (character >= 'a' && character <= 'z');
    const bool keep_character = is_alphanumeric || character == '-' || character == '.';
    if (keep_character) {
      result.push_back(character);
      previous_was_separator = false;
      continue;
    }

    if (!previous_was_separator) {
      result.push_back('_');
      previous_was_separator = true;
    }
  }

  while (!result.empty() && result.front() == '_') {
    result.erase(result.begin());
  }
  while (!result.empty() && result.back() == '_') {
    result.pop_back();
  }

  if (result.empty()) {
    return "resource";
  }
  return result;
}

[[nodiscard]] inline std::string MakeSuggestedFileName(
    std::string_view prefix,
    std::string_view format_name,
    uint32_t width,
    uint32_t height,
    uint32_t crc32,
    std::string_view extension) {
  return std::format(
      "{}_{}_{}x{}_{}.{}",
      SanitizeFileStem(prefix),
      SanitizeFileStem(format_name),
      width,
      height,
      FormatCrc32(crc32),
      extension);
}

[[nodiscard]] inline uint32_t ComputeMappedTextureCrc32(
    reshade::api::format format,
    const reshade::api::subresource_data& mapped_data,
    uint32_t width,
    uint32_t height) {
  const auto row_pitch = reshade::api::format_row_pitch(format, width);
  const auto slice_pitch = reshade::api::format_slice_pitch(format, row_pitch, height);
  if (row_pitch == 0u || slice_pitch == 0u || mapped_data.data == nullptr) {
    return 0u;
  }

  if (mapped_data.row_pitch == row_pitch) {
    return renodx::utils::hash::ComputeCRC32(static_cast<const std::uint8_t*>(mapped_data.data), slice_pitch);
  }

  const auto row_count = std::max<std::size_t>(1u, slice_pitch / row_pitch);
  auto crc = 0xFFFFFFFFu;
  const auto* bytes = static_cast<const std::uint8_t*>(mapped_data.data);
  for (std::size_t row_index = 0; row_index < row_count; ++row_index) {
    crc = renodx::utils::hash::UpdateCRC32(
        crc,
        bytes + (static_cast<std::size_t>(mapped_data.row_pitch) * row_index),
        row_pitch);
  }
  return renodx::utils::hash::FinalizeCRC32(crc);
}

[[nodiscard]] inline bool TryBuildRawPngPixels(
    reshade::api::format format,
    const reshade::api::subresource_data& mapped_data,
    uint32_t width,
    uint32_t height,
    std::vector<std::uint8_t>& rgba_pixels) {
  if (mapped_data.data == nullptr || width == 0u || height == 0u) return false;

  rgba_pixels.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u);
  for (uint32_t y = 0; y < height; ++y) {
    const auto* row = static_cast<const std::uint8_t*>(mapped_data.data)
                      + (static_cast<std::size_t>(mapped_data.row_pitch) * y);
    auto* out_row = rgba_pixels.data() + (static_cast<std::size_t>(width) * static_cast<std::size_t>(y) * 4u);
    switch (format) {
      case reshade::api::format::r8g8b8a8_unorm:
      case reshade::api::format::r8g8b8a8_unorm_srgb:
        std::memcpy(out_row, row, static_cast<std::size_t>(width) * 4u);
        break;
      case reshade::api::format::b8g8r8a8_unorm:
      case reshade::api::format::b8g8r8a8_unorm_srgb:
        for (uint32_t x = 0; x < width; ++x) {
          const auto* pixel = row + (static_cast<std::size_t>(x) * 4u);
          auto* out_pixel = out_row + (static_cast<std::size_t>(x) * 4u);
          out_pixel[0] = pixel[2];
          out_pixel[1] = pixel[1];
          out_pixel[2] = pixel[0];
          out_pixel[3] = pixel[3];
        }
        break;
      case reshade::api::format::r8g8b8x8_unorm:
      case reshade::api::format::r8g8b8x8_unorm_srgb:
        for (uint32_t x = 0; x < width; ++x) {
          const auto* pixel = row + (static_cast<std::size_t>(x) * 4u);
          auto* out_pixel = out_row + (static_cast<std::size_t>(x) * 4u);
          out_pixel[0] = pixel[0];
          out_pixel[1] = pixel[1];
          out_pixel[2] = pixel[2];
          out_pixel[3] = 255u;
        }
        break;
      case reshade::api::format::b8g8r8x8_unorm:
      case reshade::api::format::b8g8r8x8_unorm_srgb:
        for (uint32_t x = 0; x < width; ++x) {
          const auto* pixel = row + (static_cast<std::size_t>(x) * 4u);
          auto* out_pixel = out_row + (static_cast<std::size_t>(x) * 4u);
          out_pixel[0] = pixel[2];
          out_pixel[1] = pixel[1];
          out_pixel[2] = pixel[0];
          out_pixel[3] = 255u;
        }
        break;
      default:
        rgba_pixels.clear();
        return false;
    }
  }

  return true;
}

[[nodiscard]] inline json UploadSignatureToJson(const renodx::utils::resource::ResourceUploadSignature& value) {
  return json{
      {"source", renodx::utils::resource::ResourceUploadSourceName(value.source)},
      {"subresource", value.subresource},
      {"crc32", FormatCrc32(value.crc32)},
      {"crc32Value", value.crc32},
      {"format", std::format("{}", static_cast<std::uint32_t>(value.format))},
      {"width", value.width},
      {"height", value.height},
      {"depthOrLayers", value.depth_or_layers},
      {"rowPitch", value.row_pitch},
      {"slicePitch", value.slice_pitch},
      {"sourceSize", value.source_size},
      {"fullUpdate", value.full_update},
  };
}

[[nodiscard]] inline PreviewMode ParsePreviewMode(std::string_view value) {
  if (value == "clamped") return PreviewMode::CLAMPED;
  if (value == "neutwo") return PreviewMode::NEUTWO;

  throw std::runtime_error(std::format("previewMode '{}' is invalid. Expected 'clamped' or 'neutwo'.", value));
}

[[nodiscard]] inline float ToneMapNeutwo(float value) {
  value = std::max(value, 0.f);
  return value / std::sqrt(std::fma(value, value, 1.f));
}

[[nodiscard]] inline float ComputeNeutwoMaxChannelScale(const float rgb[3]) {
  const auto max_channel = std::max(std::abs(rgb[0]), std::max(std::abs(rgb[1]), std::abs(rgb[2])));
  if (max_channel <= 0.f) return 1.f;

  const auto new_max = ToneMapNeutwo(max_channel);
  return new_max / max_channel;
}

[[nodiscard]] inline std::uint8_t EncodePreviewChannel(float value) {
  value = std::clamp(LinearToSrgb(value), 0.f, 1.f);
  return static_cast<uint8_t>(std::lround(value * 255.f));
}

inline void EncodePreviewPixel(
    const float rgba[4],
    PreviewMode preview_mode,
    std::uint8_t out_rgba[4]) {
  if (!std::isfinite(rgba[0]) || !std::isfinite(rgba[1]) || !std::isfinite(rgba[2])) {
    out_rgba[0] = 255u;
    out_rgba[1] = 0u;
    out_rgba[2] = 255u;
    out_rgba[3] = 255u;
    return;
  }

  float rgb[3] = {rgba[0], rgba[1], rgba[2]};
  if (preview_mode == PreviewMode::NEUTWO) {
    const auto scale = ComputeNeutwoMaxChannelScale(rgb);
    rgb[0] *= scale;
    rgb[1] *= scale;
    rgb[2] *= scale;
  } else {
    rgb[0] = std::clamp(rgb[0], 0.f, 1.f);
    rgb[1] = std::clamp(rgb[1], 0.f, 1.f);
    rgb[2] = std::clamp(rgb[2], 0.f, 1.f);
  }

  out_rgba[0] = EncodePreviewChannel(std::max(rgb[0], 0.f));
  out_rgba[1] = EncodePreviewChannel(std::max(rgb[1], 0.f));
  out_rgba[2] = EncodePreviewChannel(std::max(rgb[2], 0.f));

  const auto alpha = std::isfinite(rgba[3]) ? std::clamp(rgba[3], 0.f, 1.f) : 1.f;
  out_rgba[3] = static_cast<uint8_t>(std::lround(alpha * 255.f));
}

[[nodiscard]] inline bool WritePngPreview(
    const std::filesystem::path& output_path,
    uint32_t width,
    uint32_t height,
    const std::vector<uint8_t>& rgba_pixels) {
  return renodx::utils::png::WriteRgba8(output_path, width, height, rgba_pixels);
}

[[nodiscard]] inline bool DecodePixelToLinear(
    reshade::api::format format,
    const uint8_t* pixel,
    float rgba[4],
    uint32_t& channel_count) {
  channel_count = 4u;

  switch (format) {
    case reshade::api::format::r8g8b8a8_unorm: {
      rgba[0] = pixel[0] / 255.f;
      rgba[1] = pixel[1] / 255.f;
      rgba[2] = pixel[2] / 255.f;
      rgba[3] = pixel[3] / 255.f;
      return true;
    }
    case reshade::api::format::r8g8b8a8_unorm_srgb: {
      rgba[0] = SrgbToLinear(pixel[0] / 255.f);
      rgba[1] = SrgbToLinear(pixel[1] / 255.f);
      rgba[2] = SrgbToLinear(pixel[2] / 255.f);
      rgba[3] = pixel[3] / 255.f;
      return true;
    }
    case reshade::api::format::b8g8r8a8_unorm: {
      rgba[0] = pixel[2] / 255.f;
      rgba[1] = pixel[1] / 255.f;
      rgba[2] = pixel[0] / 255.f;
      rgba[3] = pixel[3] / 255.f;
      return true;
    }
    case reshade::api::format::b8g8r8a8_unorm_srgb: {
      rgba[0] = SrgbToLinear(pixel[2] / 255.f);
      rgba[1] = SrgbToLinear(pixel[1] / 255.f);
      rgba[2] = SrgbToLinear(pixel[0] / 255.f);
      rgba[3] = pixel[3] / 255.f;
      return true;
    }
    case reshade::api::format::r8g8b8x8_unorm: {
      rgba[0] = pixel[0] / 255.f;
      rgba[1] = pixel[1] / 255.f;
      rgba[2] = pixel[2] / 255.f;
      rgba[3] = 1.f;
      return true;
    }
    case reshade::api::format::r8g8b8x8_unorm_srgb: {
      rgba[0] = SrgbToLinear(pixel[0] / 255.f);
      rgba[1] = SrgbToLinear(pixel[1] / 255.f);
      rgba[2] = SrgbToLinear(pixel[2] / 255.f);
      rgba[3] = 1.f;
      return true;
    }
    case reshade::api::format::b8g8r8x8_unorm: {
      rgba[0] = pixel[2] / 255.f;
      rgba[1] = pixel[1] / 255.f;
      rgba[2] = pixel[0] / 255.f;
      rgba[3] = 1.f;
      return true;
    }
    case reshade::api::format::b8g8r8x8_unorm_srgb: {
      rgba[0] = SrgbToLinear(pixel[2] / 255.f);
      rgba[1] = SrgbToLinear(pixel[1] / 255.f);
      rgba[2] = SrgbToLinear(pixel[0] / 255.f);
      rgba[3] = 1.f;
      return true;
    }
    case reshade::api::format::r10g10b10a2_unorm: {
      const auto packed = *reinterpret_cast<const uint32_t*>(pixel);
      rgba[0] = ((packed >> 0) & 0x3FFu) / 1023.f;
      rgba[1] = ((packed >> 10) & 0x3FFu) / 1023.f;
      rgba[2] = ((packed >> 20) & 0x3FFu) / 1023.f;
      rgba[3] = ((packed >> 30) & 0x3u) / 3.f;
      return true;
    }
    case reshade::api::format::b10g10r10a2_unorm: {
      const auto packed = *reinterpret_cast<const uint32_t*>(pixel);
      rgba[2] = ((packed >> 0) & 0x3FFu) / 1023.f;
      rgba[1] = ((packed >> 10) & 0x3FFu) / 1023.f;
      rgba[0] = ((packed >> 20) & 0x3FFu) / 1023.f;
      rgba[3] = ((packed >> 30) & 0x3u) / 3.f;
      return true;
    }
    case reshade::api::format::r11g11b10_float: {
      const auto packed = *reinterpret_cast<const uint32_t*>(pixel);
      rgba[0] = DecodeR11Component((packed >> 0) & 0x7FFu);
      rgba[1] = DecodeR11Component((packed >> 11) & 0x7FFu);
      rgba[2] = DecodeR10Component((packed >> 22) & 0x3FFu);
      rgba[3] = 1.f;
      return true;
    }
    case reshade::api::format::r16_float: {
      rgba[0] = DecodeFloat16(*reinterpret_cast<const uint16_t*>(pixel));
      rgba[1] = 0.f;
      rgba[2] = 0.f;
      rgba[3] = 1.f;
      channel_count = 1u;
      return true;
    }
    case reshade::api::format::r16g16b16a16_float: {
      const auto* packed = reinterpret_cast<const uint16_t*>(pixel);
      rgba[0] = DecodeFloat16(packed[0]);
      rgba[1] = DecodeFloat16(packed[1]);
      rgba[2] = DecodeFloat16(packed[2]);
      rgba[3] = DecodeFloat16(packed[3]);
      return true;
    }
    case reshade::api::format::r16g16b16a16_unorm: {
      const auto* packed = reinterpret_cast<const uint16_t*>(pixel);
      rgba[0] = packed[0] / 65535.f;
      rgba[1] = packed[1] / 65535.f;
      rgba[2] = packed[2] / 65535.f;
      rgba[3] = packed[3] / 65535.f;
      return true;
    }
    case reshade::api::format::r32g32b32a32_float: {
      std::memcpy(rgba, pixel, sizeof(float) * 4u);
      return true;
    }
    default:
      return false;
  }
}

// Color LUTs map RGB to RGB; 1- and 2-channel volumes (local exposure grids, histograms) are not color LUTs.
[[nodiscard]] inline bool HasFewerThanThreeChannels(reshade::api::format format) {
  switch (format) {
    case reshade::api::format::r8_unorm:
    case reshade::api::format::r8g8_unorm:
    case reshade::api::format::r16_unorm:
    case reshade::api::format::r16_float:
    case reshade::api::format::r16g16_unorm:
    case reshade::api::format::r16g16_float:
    case reshade::api::format::r32_float:
    case reshade::api::format::r32g32_float:
      return true;
    default:
      return false;
  }
}

struct MappedTextureReadback {
  reshade::api::device* device = nullptr;
  reshade::api::resource staging = {0u};
  bool used_buffer = false;
  bool mapped = false;
  reshade::api::subresource_data data = {};

  MappedTextureReadback() = default;
  MappedTextureReadback(const MappedTextureReadback&) = delete;
  MappedTextureReadback& operator=(const MappedTextureReadback&) = delete;

  ~MappedTextureReadback() {
    Release();
  }

  void Release() {
    if (device != nullptr && staging.handle != 0u) {
      if (mapped) {
        if (used_buffer) {
          device->unmap_buffer_region(staging);
        } else {
          device->unmap_texture_region(staging, 0u);
        }
        mapped = false;
      }
      device->destroy_resource(staging);
      staging = {0u};
    }
    data = {};
    used_buffer = false;
  }

  [[nodiscard]] bool Acquire(
      reshade::api::device* target_device,
      reshade::api::command_queue* queue,
      reshade::api::resource source,
      uint32_t subresource,
      bool is_3d,
      uint32_t width,
      uint32_t height,
      uint32_t depth,
      reshade::api::format intermediate_format,
      std::string* out_error) {
    Release();
    device = target_device;

    const bool created_texture = device->create_resource(
        reshade::api::resource_desc(
            is_3d ? reshade::api::resource_type::texture_3d : reshade::api::resource_type::texture_2d,
            width,
            height,
            static_cast<uint16_t>(depth),
            1u,
            intermediate_format,
            1u,
            reshade::api::memory_heap::gpu_to_cpu,
            reshade::api::resource_usage::copy_dest),
        nullptr,
        reshade::api::resource_usage::copy_dest,
        &staging);

    if (created_texture) {
      used_buffer = false;
      auto* cmd_list = queue->get_immediate_command_list();
      cmd_list->copy_texture_region(source, subresource, nullptr, staging, 0u, nullptr);
      queue->flush_immediate_command_list();
      queue->wait_idle();

      if (!device->map_texture_region(staging, 0u, nullptr, reshade::api::map_access::read_only, &data)) {
        if (out_error != nullptr) *out_error = "Failed to map the texture readback staging resource.";
        Release();
        return false;
      }
      mapped = true;
      return true;
    }

    // Fallback for APIs (e.g. Vulkan / D3D12) where gpu_to_cpu texture_3d staging resources are not supported:
    const uint32_t bytes_per_pixel = reshade::api::format_row_pitch(intermediate_format, 1u);
    const uint32_t raw_row_pitch = reshade::api::format_row_pitch(intermediate_format, width);
    if (bytes_per_pixel == 0u || raw_row_pitch == 0u) {
      if (out_error != nullptr) *out_error = "Unsupported texture format for buffer staging readback.";
      return false;
    }
    const uint32_t aligned_row_pitch = (device->get_api() == reshade::api::device_api::d3d12)
                                           ? ((raw_row_pitch + 255u) & ~255u)
                                           : raw_row_pitch;
    const uint32_t row_length_texels = aligned_row_pitch / bytes_per_pixel;
    const uint32_t slice_pitch_bytes = aligned_row_pitch * height;
    const uint64_t buffer_size = static_cast<uint64_t>(slice_pitch_bytes) * static_cast<uint64_t>(depth);

    if (!device->create_resource(
            reshade::api::resource_desc(
                buffer_size,
                reshade::api::memory_heap::gpu_to_cpu,
                reshade::api::resource_usage::copy_dest),
            nullptr,
            reshade::api::resource_usage::copy_dest,
            &staging)) {
      if (out_error != nullptr) *out_error = "Failed to create a GPU-to-CPU staging texture or buffer for readback.";
      return false;
    }

    used_buffer = true;
    auto* cmd_list = queue->get_immediate_command_list();
    cmd_list->copy_texture_to_buffer(source, subresource, nullptr, staging, 0u, row_length_texels, height);
    queue->flush_immediate_command_list();
    queue->wait_idle();

    void* mapped_ptr = nullptr;
    if (!device->map_buffer_region(staging, 0u, UINT64_MAX, reshade::api::map_access::read_only, &mapped_ptr)
        || mapped_ptr == nullptr) {
      if (out_error != nullptr) *out_error = "Failed to map the buffer readback staging resource.";
      Release();
      return false;
    }

    mapped = true;
    data.data = mapped_ptr;
    data.row_pitch = aligned_row_pitch;
    data.slice_pitch = slice_pitch_bytes;
    return true;
  }
};

[[nodiscard]] inline ToolResult PerformResourceAnalysis(
    const ProcessingContext& context,
    std::uint32_t device_index,
    reshade::api::device* device,
    reshade::api::command_queue* queue,
    reshade::api::resource_view resource_view,
    reshade::api::resource raw_resource,
    const std::optional<std::filesystem::path>& output_path,
    const std::optional<std::filesystem::path>& output_directory,
    const std::optional<std::string>& name_prefix,
    PreviewMode preview_mode,
    std::string_view preview_mode_name,
    bool prefer_clone) {
  AnalysisTarget target = {};
  try {
    target = ResolveAnalysisTarget(resource_view, raw_resource, device, prefer_clone);
  } catch (const std::exception& exception) {
    auto error = std::string(exception.what());
    if (error.starts_with("resourceViewHandle ")) {
      error = std::format("resourceViewHandle {} is not currently tracked.", FormatHandle(context, resource_view.handle));
    } else if (error.starts_with("resourceHandle ")) {
      error = std::format("resourceHandle {} is not currently tracked.", FormatHandle(context, raw_resource.handle));
    }
    return ToolResult{
        .text = error,
        .structured_content = json{{"error", error}},
        .is_error = true,
    };
  }

  const bool is_3d = (target.resolved_resource_desc.type == reshade::api::resource_type::texture_3d);
  if (target.resolved_resource_desc.type != reshade::api::resource_type::texture_2d
      && target.resolved_resource_desc.type != reshade::api::resource_type::surface
      && !is_3d) {
    return ToolResult{
        .text = "Only 2D and 3D texture resources are currently supported.",
        .structured_content = json{{"error", "Only 2D and 3D texture resources are currently supported."}},
        .is_error = true,
    };
  }
  if (target.resolved_resource_desc.texture.samples != 1u) {
    return ToolResult{
        .text = "Multisampled resources are not currently supported.",
        .structured_content = json{{"error", "Multisampled resources are not currently supported."}},
        .is_error = true,
    };
  }

  const auto summary_view = target.used_clone ? target.resolved_view : resource_view;
  const auto resource_view_summary_value = (context.build_resource_view_summary && summary_view.handle != 0u)
                                               ? std::optional<resource_view_summary::ResourceViewSummary>(
                                                     context.build_resource_view_summary(summary_view, device))
                                               : std::nullopt;
  const auto upload_resource = [&]() {
    if (resource_view.handle != 0u) {
      if (const auto resource_from_view = renodx::utils::resource::GetResourceFromView(resource_view);
          resource_from_view.handle != 0u) {
        return resource_from_view;
      }
    }
    return target.resolved_resource;
  }();

  const auto first_level = target.resolved_view_desc.texture.first_level;
  const auto first_layer = is_3d ? 0u : target.resolved_view_desc.texture.first_layer;
  const auto levels = std::max<uint32_t>(target.resolved_resource_desc.texture.levels, 1u);
  const auto subresource = is_3d ? first_level : (first_level + (first_layer * levels));
  const auto width = std::max<uint32_t>(target.resolved_resource_desc.texture.width >> first_level, 1u);
  const auto height = std::max<uint32_t>(target.resolved_resource_desc.texture.height >> first_level, 1u);
  const auto depth = is_3d
                         ? std::max<uint32_t>(static_cast<uint32_t>(target.resolved_resource_desc.texture.depth_or_layers) >> first_level, 1u)
                         : 1u;
  const auto preview_width = width * depth;
  const auto intermediate_format = reshade::api::format_to_default_typed(target.resolved_resource_desc.texture.format, 0);
  const auto analysis_format = target.resolved_view_desc.format != reshade::api::format::unknown
                                   ? target.resolved_view_desc.format
                                   : intermediate_format;

  MappedTextureReadback readback;
  std::string readback_error;
  if (!readback.Acquire(
          device,
          queue,
          target.resolved_resource,
          subresource,
          is_3d,
          width,
          height,
          depth,
          intermediate_format,
          &readback_error)) {
    return ToolResult{
        .text = readback_error,
        .structured_content = json{{"error", readback_error}},
        .is_error = true,
    };
  }

  const bool pq_encoded = context.is_hdr10_pq_backbuffer && context.is_hdr10_pq_backbuffer(device, target.resolved_resource);

  ToolResult tool_result = {
      .text = "Failed to map the readback resource.",
      .structured_content = json{{"error", "Failed to map the readback resource."}},
      .is_error = true,
  };
  const auto& mapped_data = readback.data;
  {
    double channel_min[4] = {
        std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::infinity(),
    };
    double channel_max[4] = {
        -std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity(),
    };
    double channel_sum[4] = {0.0, 0.0, 0.0, 0.0};
    double luminance_min = std::numeric_limits<double>::infinity();
    double luminance_max = -std::numeric_limits<double>::infinity();
    double luminance_sum = 0.0;
    uint32_t channel_count = 4u;
    uint64_t pixel_count = 0u;
    uint64_t finite_pixel_count = 0u;
    uint64_t nan_pixel_count = 0u;
    uint64_t inf_pixel_count = 0u;
    uint64_t pixels_all_rgb_zero = 0u;
    uint64_t pixels_any_negative = 0u;
    uint64_t pixels_any_at_or_above_one = 0u;
    uint64_t pixels_any_above_one = 0u;
    uint64_t pixels_any_above_two = 0u;
    uint64_t pixels_any_above_four = 0u;
    uint64_t gamut_bt709_pixels = 0u;
    uint64_t gamut_dci_p3_pixels = 0u;
    uint64_t gamut_bt2020_pixels = 0u;
    uint64_t gamut_out_of_bt2020_pixels = 0u;
    const uint64_t total_expected_pixels = static_cast<uint64_t>(width) * static_cast<uint64_t>(height) * static_cast<uint64_t>(depth);
    const uint64_t percentile_sample_stride = std::max<uint64_t>(1u, total_expected_pixels / 65536u);
    std::vector<float> luminance_samples;
    std::vector<float> max_rgb_samples;
    luminance_samples.reserve(static_cast<size_t>(std::min<uint64_t>(total_expected_pixels, 65536u) + 16u));
    max_rgb_samples.reserve(static_cast<size_t>(std::min<uint64_t>(total_expected_pixels, 65536u) + 16u));
    bool supported_format = true;
    std::vector<std::uint8_t> preview_pixels;
    std::vector<std::uint8_t> raw_png_pixels;
    std::vector<float> exr_pixels;
    const auto output_format = output_path.has_value()
                                   ? GetOutputFormat(output_path.value())
                                   : OutputFormat::PNG;
    const auto resource_format_name = FormatFormat(context, intermediate_format);
    const auto live_raw_crc32 = ComputeMappedTextureCrc32(intermediate_format, mapped_data, width, height);
    const auto initial_upload = renodx::utils::resource::GetInitialUploadSignature(upload_resource);
    const auto latest_upload = renodx::utils::resource::GetLatestUploadSignature(upload_resource);
    std::string preferred_crc32_source = "live_readback";
    uint32_t preferred_crc32 = live_raw_crc32;
    if (initial_upload.has_value()
        && initial_upload->subresource == subresource
        && initial_upload->format == target.resolved_resource_desc.texture.format
        && initial_upload->width == width
        && initial_upload->height == height
        && initial_upload->full_update) {
      preferred_crc32 = initial_upload->crc32;
      preferred_crc32_source = "initial_upload";
    } else if (latest_upload.has_value()
               && latest_upload->subresource == subresource
               && latest_upload->format == target.resolved_resource_desc.texture.format
               && latest_upload->width == width
               && latest_upload->height == height
               && latest_upload->full_update) {
      preferred_crc32 = latest_upload->crc32;
      preferred_crc32_source = "latest_upload";
    }
    if (output_path.has_value() && output_format == OutputFormat::PNG) {
      preview_pixels.resize(static_cast<size_t>(preview_width) * static_cast<size_t>(height) * 4u);
    }
    if (output_path.has_value() && output_format == OutputFormat::EXR) {
      if (is_3d || analysis_format != reshade::api::format::r16g16b16a16_float) {
        exr_pixels.resize(static_cast<size_t>(preview_width) * static_cast<size_t>(height) * 4u);
      }
    }

    const auto slice_pitch = (mapped_data.slice_pitch != 0u)
                                 ? static_cast<size_t>(mapped_data.slice_pitch)
                                 : (static_cast<size_t>(mapped_data.row_pitch) * static_cast<size_t>(height));
    const auto pixel_stride = static_cast<size_t>(reshade::api::format_row_pitch(intermediate_format, 1));
    for (uint32_t z = 0; z < depth && supported_format; ++z) {
      const auto* slice = static_cast<const std::uint8_t*>(mapped_data.data) + (slice_pitch * z);
      for (uint32_t y = 0; y < height && supported_format; ++y) {
        const auto* row = slice + (static_cast<size_t>(mapped_data.row_pitch) * y);
        for (uint32_t x = 0; x < width; ++x) {
          float rgba[4] = {0.f, 0.f, 0.f, 1.f};
          uint32_t decoded_channel_count = 4u;
          const auto* pixel = row + (pixel_stride * x);
          if (!DecodePixelToLinear(analysis_format, pixel, rgba, decoded_channel_count)) {
            supported_format = false;
            break;
          }
          const auto preview_x = static_cast<size_t>(z) * static_cast<size_t>(width) + static_cast<size_t>(x);
          const auto preview_offset = ((static_cast<size_t>(y) * static_cast<size_t>(preview_width)) + preview_x) * 4u;
          if (!preview_pixels.empty()) {
            EncodePreviewPixel(rgba, preview_mode, preview_pixels.data() + preview_offset);
          }
          if (!exr_pixels.empty()) {
            auto* out_pixel = exr_pixels.data() + preview_offset;
            out_pixel[0] = rgba[0];
            out_pixel[1] = rgba[1];
            out_pixel[2] = rgba[2];
            out_pixel[3] = rgba[3];
          }
        ++pixel_count;
        channel_count = decoded_channel_count;

        bool has_nan = false;
        bool has_inf = false;
        for (uint32_t channel = 0; channel < 4u; ++channel) {
          has_nan |= std::isnan(rgba[channel]);
          has_inf |= std::isinf(rgba[channel]);
        }
        if (has_nan) ++nan_pixel_count;
        if (has_inf) ++inf_pixel_count;
        if (has_nan || has_inf) continue;

        ++finite_pixel_count;

        const auto any_negative = rgba[0] < 0.f || rgba[1] < 0.f || rgba[2] < 0.f;
        const auto any_at_or_above_one = rgba[0] >= 1.f || rgba[1] >= 1.f || rgba[2] >= 1.f;
        const auto any_above_one = rgba[0] > 1.f || rgba[1] > 1.f || rgba[2] > 1.f;
        const auto any_above_two = rgba[0] > 2.f || rgba[1] > 2.f || rgba[2] > 2.f;
        const auto any_above_four = rgba[0] > 4.f || rgba[1] > 4.f || rgba[2] > 4.f;
        const auto all_rgb_zero = rgba[0] == 0.f && rgba[1] == 0.f && rgba[2] == 0.f;

        pixels_any_negative += any_negative ? 1u : 0u;
        pixels_any_at_or_above_one += any_at_or_above_one ? 1u : 0u;
        pixels_any_above_one += any_above_one ? 1u : 0u;
        pixels_any_above_two += any_above_two ? 1u : 0u;
        pixels_any_above_four += any_above_four ? 1u : 0u;
        pixels_all_rgb_zero += all_rgb_zero ? 1u : 0u;

        for (uint32_t channel = 0; channel < 4u; ++channel) {
          channel_min[channel] = std::min(channel_min[channel], static_cast<double>(rgba[channel]));
          channel_max[channel] = std::max(channel_max[channel], static_cast<double>(rgba[channel]));
          channel_sum[channel] += rgba[channel];
        }

        const auto luminance = (0.2126 * rgba[0]) + (0.7152 * rgba[1]) + (0.0722 * rgba[2]);
        luminance_min = std::min(luminance_min, luminance);
        luminance_max = std::max(luminance_max, luminance);
        luminance_sum += luminance;

        if (((finite_pixel_count - 1u) % percentile_sample_stride) == 0u) {
          luminance_samples.push_back(static_cast<float>(luminance));
          max_rgb_samples.push_back(std::max({rgba[0], rgba[1], rgba[2]}));
        }

        // PQ-encoded values are BT.2020 code values; decode to linear and convert to BT.709 so the same
        // negative-channel test measures how much of the image leaves BT.709.
        std::array<double, 3> gamut_rgb = {rgba[0], rgba[1], rgba[2]};
        if (pq_encoded) {
          constexpr double PQ_M1 = 0.1593017578125;
          constexpr double PQ_M2 = 78.84375;
          constexpr double PQ_C1 = 0.8359375;
          constexpr double PQ_C2 = 18.8515625;
          constexpr double PQ_C3 = 18.6875;
          std::array<double, 3> bt2020_linear = {};
          for (std::size_t channel = 0u; channel < 3u; ++channel) {
            const double encoded_power = std::pow(std::clamp(static_cast<double>(rgba[channel]), 0.0, 1.0), 1.0 / PQ_M2);
            bt2020_linear[channel] = std::pow(std::max(encoded_power - PQ_C1, 0.0) / (PQ_C2 - (PQ_C3 * encoded_power)), 1.0 / PQ_M1);
          }
          gamut_rgb = {
              (1.66049100 * bt2020_linear[0]) - (0.58764114 * bt2020_linear[1]) - (0.07284986 * bt2020_linear[2]),
              (-0.12455047 * bt2020_linear[0]) + (1.13289990 * bt2020_linear[1]) - (0.00834942 * bt2020_linear[2]),
              (-0.01815076 * bt2020_linear[0]) - (0.10057890 * bt2020_linear[1]) + (1.11872970 * bt2020_linear[2]),
          };
        }

        constexpr double GAMUT_EPS = -1e-4;
        if (gamut_rgb[0] >= GAMUT_EPS && gamut_rgb[1] >= GAMUT_EPS && gamut_rgb[2] >= GAMUT_EPS) {
          ++gamut_bt709_pixels;
        } else {
          const double p3_r = (0.82246197 * gamut_rgb[0]) + (0.17753803 * gamut_rgb[1]);
          const double p3_g = (0.03319420 * gamut_rgb[0]) + (0.96680580 * gamut_rgb[1]);
          const double p3_b = (0.01708263 * gamut_rgb[0]) + (0.07239744 * gamut_rgb[1]) + (0.91051993 * gamut_rgb[2]);
          if (p3_r >= GAMUT_EPS && p3_g >= GAMUT_EPS && p3_b >= GAMUT_EPS) {
            ++gamut_dci_p3_pixels;
          } else {
            const double bt2020_r = (0.62740390 * gamut_rgb[0]) + (0.32928304 * gamut_rgb[1]) + (0.04331307 * gamut_rgb[2]);
            const double bt2020_g = (0.06909729 * gamut_rgb[0]) + (0.91954040 * gamut_rgb[1]) + (0.01136232 * gamut_rgb[2]);
            const double bt2020_b = (0.01639144 * gamut_rgb[0]) + (0.08801331 * gamut_rgb[1]) + (0.89559525 * gamut_rgb[2]);
            if (bt2020_r >= GAMUT_EPS && bt2020_g >= GAMUT_EPS && bt2020_b >= GAMUT_EPS) {
              ++gamut_bt2020_pixels;
            } else {
              ++gamut_out_of_bt2020_pixels;
            }
          }
        }
      }
      }
    }

    if (!supported_format) {
      readback.Release();
      const auto error = std::format("Readback is not implemented for format {}.", FormatFormat(context, analysis_format));
      tool_result = ToolResult{
          .text = error,
          .structured_content = json{{"error", error}},
          .is_error = true,
      };
    } else {
      json output_json = nullptr;
      json hash_dump_json = nullptr;
      std::optional<std::string> output_error = std::nullopt;
      if (output_path.has_value()) {
        std::error_code filesystem_error;
        const auto parent_path = output_path->parent_path();
        if (!parent_path.empty()) {
          std::filesystem::create_directories(parent_path, filesystem_error);
        }
        if (filesystem_error) {
          output_error = std::format(
              "Failed to create output directory '{}': {}",
              output_path->parent_path().string(),
              filesystem_error.message());
        } else {
          switch (output_format) {
            case OutputFormat::PNG:
              if (!WritePngPreview(output_path.value(), preview_width, height, preview_pixels)) {
                output_error = std::format(
                    "Failed to write preview PNG to '{}'.",
                    output_path->string());
              } else {
                output_json = json{
                    {"path", output_path->string()},
                    {"format", "png"},
                    {"mode", preview_mode_name},
                    {"width", preview_width},
                    {"height", height},
                };
              }
              break;
            case OutputFormat::EXR:
              try {
                if (!is_3d && analysis_format == reshade::api::format::r16g16b16a16_float) {
                  if ((mapped_data.row_pitch % sizeof(std::uint16_t)) != 0u) {
                    output_error = std::format(
                        "Mapped RGBA16F readback row pitch ({}) is not aligned to 16-bit values.",
                        mapped_data.row_pitch);
                    break;
                  }

                  const auto row_stride_values = static_cast<size_t>(mapped_data.row_pitch) / sizeof(std::uint16_t);
                  const auto total_values = row_stride_values * static_cast<size_t>(height);
                  const auto* half_pixels = static_cast<const std::uint16_t*>(mapped_data.data);
                  renodx::utils::exr::WriteRgba16f(
                      output_path.value(),
                      width,
                      height,
                      std::span<const std::uint16_t>(half_pixels, total_values),
                      row_stride_values);
                } else {
                  renodx::utils::exr::WriteRgba16f(
                      output_path.value(),
                      preview_width,
                      height,
                      std::span<const float>(exr_pixels.data(), exr_pixels.size()));
                }
                output_json = json{
                    {"path", output_path->string()},
                    {"format", "exr"},
                    {"layout", "scanline"},
                    {"compression", "none"},
                    {"pixelType", "rgba16f"},
                    {"width", preview_width},
                    {"height", height},
                };
              } catch (const std::exception& exception) {
                output_error = std::format(
                    "Failed to write RGBA16F EXR dump to '{}': {}",
                    output_path->string(),
                    exception.what());
              }
              break;
          }
        }
      }

      if (!output_error.has_value() && output_directory.has_value()) {
        std::error_code directory_error = {};
        std::filesystem::create_directories(output_directory.value(), directory_error);
        if (directory_error) {
          output_error = std::format(
              "Failed to create output directory '{}': {}",
              output_directory->string(),
              directory_error.message());
        } else if (!TryBuildRawPngPixels(intermediate_format, mapped_data, width, height, raw_png_pixels)) {
          output_error = std::format(
              "Hash-named dumping currently supports only RGBA8/BGRA8-style resource formats. The current resource format is {}.",
              resource_format_name);
        } else {
          std::string chosen_prefix = name_prefix.value_or("");
          if (chosen_prefix.empty() && resource_view_summary_value.has_value()) {
            if (resource_view_summary_value->resource_reflection.has_value()
                && !resource_view_summary_value->resource_reflection->empty()) {
              chosen_prefix = resource_view_summary_value->resource_reflection.value();
            } else if (resource_view_summary_value->resource_view_reflection.has_value()
                       && !resource_view_summary_value->resource_view_reflection->empty()) {
              chosen_prefix = resource_view_summary_value->resource_view_reflection.value();
            }
          }
          if (chosen_prefix.empty()) {
            chosen_prefix = "resource";
          }

          const auto file_name = MakeSuggestedFileName(
              chosen_prefix,
              resource_format_name,
              width,
              height,
              preferred_crc32,
              "png");
          const auto hash_dump_path = (output_directory.value() / file_name).lexically_normal();
          if (!WritePngPreview(hash_dump_path, width, height, raw_png_pixels)) {
            output_error = std::format(
                "Failed to write hash-named PNG dump to '{}'.",
                hash_dump_path.string());
          } else {
            hash_dump_json = json{
                {"path", hash_dump_path.string()},
                {"fileName", file_name},
                {"format", "png"},
                {"width", width},
                {"height", height},
                {"crc32", FormatCrc32(preferred_crc32)},
                {"crc32Value", preferred_crc32},
                {"crc32Source", preferred_crc32_source},
                {"liveCrc32", FormatCrc32(live_raw_crc32)},
                {"liveCrc32Value", live_raw_crc32},
            };
          }
        }
      }

      readback.Release();

      if (output_error.has_value()) {
        tool_result = ToolResult{
            .text = output_error.value(),
            .structured_content = json{{"error", output_error.value()}},
            .is_error = true,
        };
        return tool_result;
      }

      std::string suggested_prefix = name_prefix.value_or("");
      if (suggested_prefix.empty() && resource_view_summary_value.has_value()) {
        if (resource_view_summary_value->resource_reflection.has_value()
            && !resource_view_summary_value->resource_reflection->empty()) {
          suggested_prefix = resource_view_summary_value->resource_reflection.value();
        } else if (resource_view_summary_value->resource_view_reflection.has_value()
                   && !resource_view_summary_value->resource_view_reflection->empty()) {
          suggested_prefix = resource_view_summary_value->resource_view_reflection.value();
        }
      }
      if (suggested_prefix.empty()) {
        suggested_prefix = "resource";
      }

      auto resource_view_json = resource_view_summary_value.has_value()
                                    ? json(resource_view_summary_value.value())
                                    : json(nullptr);
      json analysis_target_json = {
          {"preferClone", prefer_clone},
          {"usedClone", target.used_clone},
          {"requestedResourceViewHandle", FormatHandle(context, resource_view.handle)},
          {"resolvedResourceViewHandle", FormatHandle(context, target.resolved_view.handle)},
          {"resolvedResourceHandle", FormatHandle(context, target.resolved_resource.handle)},
          {"resolvedViewType", std::format("{}", static_cast<std::uint32_t>(target.resolved_view_desc.type))},
          {"resolvedViewFormat", FormatFormat(context, analysis_format)},
          {"resolvedResourceType", std::format("{}", static_cast<std::uint32_t>(target.resolved_resource_desc.type))},
          {"resolvedResourceFormat", target.resolved_resource_desc.type == reshade::api::resource_type::buffer
                                         ? json(nullptr)
                                         : json(FormatFormat(context, target.resolved_resource_desc.texture.format))},
      };
      const auto compute_percentile = [](std::vector<float>* samples, double fraction) -> json {
        if (samples->empty()) return json(nullptr);
        const auto target_index = std::min<size_t>(
            samples->size() - 1u,
            static_cast<size_t>(std::floor(fraction * static_cast<double>(samples->size() - 1u))));
        std::nth_element(samples->begin(), samples->begin() + static_cast<std::ptrdiff_t>(target_index), samples->end());
        return json(static_cast<double>((*samples)[target_index]));
      };
      const json luminance_p99 = compute_percentile(&luminance_samples, 0.99);
      const json luminance_p999 = compute_percentile(&luminance_samples, 0.999);
      const json max_rgb_p99 = compute_percentile(&max_rgb_samples, 0.99);
      const json max_rgb_p999 = compute_percentile(&max_rgb_samples, 0.999);

      json result = {
          {"deviceIndex", device_index},
          {"resourceViewHandle", resource_view.handle == 0u ? json(nullptr) : json(FormatHandle(context, resource_view.handle))},
          {"resourceHandle", FormatHandle(context, target.resolved_resource.handle)},
          {"resourceView", resource_view_json},
          {"analysisTarget", analysis_target_json},
          {
              "resourceIdentity",
              {
                  {"liveCrc32", FormatCrc32(live_raw_crc32)},
                  {"liveCrc32Value", live_raw_crc32},
                  {"preferredCrc32", FormatCrc32(preferred_crc32)},
                  {"preferredCrc32Value", preferred_crc32},
                  {"preferredCrc32Source", preferred_crc32_source},
                  {"initialUpload", initial_upload.has_value() ? UploadSignatureToJson(initial_upload.value()) : json(nullptr)},
                  {"latestUpload", latest_upload.has_value() ? UploadSignatureToJson(latest_upload.value()) : json(nullptr)},
                  {"suggestedFileName", MakeSuggestedFileName(suggested_prefix, resource_format_name, width, height, preferred_crc32, "png")},
              },
          },
          {
              "analysis",
              {
                  {"snapshotExact", false},
                  {"note", "This reads the current contents of the live resource view, not a frozen historical copy from the snapshot cache."},
                  {"subresourceIndex", subresource},
                  {"width", width},
                  {"height", height},
                  {"depth", depth},
                  {"analysisFormat", FormatFormat(context, analysis_format)},
                  {"intermediateFormat", FormatFormat(context, intermediate_format)},
                  {"channelCount", channel_count},
                  {"pixelCount", pixel_count},
                  {"finitePixels", RatioStats(finite_pixel_count, pixel_count)},
                  {"nanPixels", RatioStats(nan_pixel_count, pixel_count)},
                  {"infPixels", RatioStats(inf_pixel_count, pixel_count)},
                  {"pixelsAllRgbZero", RatioStats(pixels_all_rgb_zero, pixel_count)},
                  {"pixelsAnyNegative", RatioStats(pixels_any_negative, pixel_count)},
                  {"pixelsAnyChannelAtOrAbove1", RatioStats(pixels_any_at_or_above_one, pixel_count)},
                  {"pixelsAnyChannelAbove1", RatioStats(pixels_any_above_one, pixel_count)},
                  {"pixelsAnyChannelAbove2", RatioStats(pixels_any_above_two, pixel_count)},
                  {"pixelsAnyChannelAbove4", RatioStats(pixels_any_above_four, pixel_count)},
                  {
                      "channels",
                      {
                          {"r", ScalarStats(channel_min[0], channel_max[0], channel_sum[0], finite_pixel_count)},
                          {"g", ScalarStats(channel_min[1], channel_max[1], channel_sum[1], finite_pixel_count)},
                          {"b", ScalarStats(channel_min[2], channel_max[2], channel_sum[2], finite_pixel_count)},
                          {"a", ScalarStats(channel_min[3], channel_max[3], channel_sum[3], finite_pixel_count)},
                      },
                  },
                  {"luminance", ScalarStats(luminance_min, luminance_max, luminance_sum, finite_pixel_count)},
                  {
                      "percentiles",
                      {
                          {"luminanceP99", luminance_p99},
                          {"luminanceP999", luminance_p999},
                          {"maxRgbP99", max_rgb_p99},
                          {"maxRgbP999", max_rgb_p999},
                          {"sampleCount", luminance_samples.size()},
                      },
                  },
                  {
                      "gamut",
                      {
                          {"assumedColorSpace", pq_encoded ? "hdr10_pq_bt2020" : "bt709_linear"},
                          {"bt709", RatioStats(gamut_bt709_pixels, finite_pixel_count)},
                          {"dciP3Only", RatioStats(gamut_dci_p3_pixels, finite_pixel_count)},
                          {"bt2020Only", RatioStats(gamut_bt2020_pixels, finite_pixel_count)},
                          {"outOfBt2020", RatioStats(gamut_out_of_bt2020_pixels, finite_pixel_count)},
                      },
                  },
              },
          },
      };
      if (!output_json.is_null()) {
        if (output_format == OutputFormat::PNG) {
          result["preview"] = output_json;
        } else {
          result["dump"] = output_json;
        }
      }
      if (!hash_dump_json.is_null()) {
        result["hashDump"] = hash_dump_json;
      }

      std::string text;
      const auto analyzed_view_handle = FormatHandle(context, target.resolved_view.handle);
      const auto* const analyzed_view_label = target.used_clone ? "clone resource view" : "resource view";
      if (output_path.has_value()) {
        if (output_format == OutputFormat::PNG) {
          text = std::format(
              "Analyzed {}x{} {} {} {} on device #{}. Saved a {} preview to '{}'. {} pixel(s) are above 1.0 in at least one RGB channel.",
              width,
              height,
              FormatFormat(context, analysis_format),
              analyzed_view_label,
              analyzed_view_handle,
              device_index,
              preview_mode_name,
              output_path->string(),
              pixels_any_above_one);
        } else {
          text = std::format(
              "Analyzed {}x{} {} {} {} on device #{}. Saved an RGBA16F EXR dump to '{}'. {} pixel(s) are above 1.0 in at least one RGB channel.",
              width,
              height,
              FormatFormat(context, analysis_format),
              analyzed_view_label,
              analyzed_view_handle,
              device_index,
              output_path->string(),
              pixels_any_above_one);
        }
        if (!hash_dump_json.is_null()) {
          text += std::format(" Saved a hash-named PNG dump to '{}'.", hash_dump_json["path"].get<std::string>());
        }
      } else {
        if (!hash_dump_json.is_null()) {
          text = std::format(
              "Analyzed {}x{} {} {} {} on device #{}. Saved a hash-named PNG dump to '{}'. {} pixel(s) are above 1.0 in at least one RGB channel.",
              width,
              height,
              FormatFormat(context, analysis_format),
              analyzed_view_label,
              analyzed_view_handle,
              device_index,
              hash_dump_json["path"].get<std::string>(),
              pixels_any_above_one);
        } else {
          text = std::format(
              "Analyzed {}x{} {} {} {} on device #{}. {} pixel(s) are above 1.0 in at least one RGB channel.",
              width,
              height,
              FormatFormat(context, analysis_format),
              analyzed_view_label,
              analyzed_view_handle,
              device_index,
              pixels_any_above_one);
        }
      }
      tool_result = ToolResult{
          .text = text,
          .structured_content = result,
      };
    }
  }

  readback.Release();
  return tool_result;
}

[[nodiscard]] inline ToolResult PerformBatchPassScan(
    const ProcessingContext& context,
    std::uint32_t device_index,
    reshade::api::device* device,
    reshade::api::command_queue* queue,
    const std::vector<ScanTargetSpec>& targets,
    bool prefer_clone) {
  json entries = json::array();
  std::size_t ok_count = 0u;
  std::size_t error_count = 0u;
  std::optional<std::size_t> last_hdr_entry_index = std::nullopt;
  std::optional<std::size_t> first_clipped_after_hdr_index = std::nullopt;
  std::optional<std::size_t> last_negative_entry_index = std::nullopt;
  std::optional<std::size_t> first_nan_or_inf_entry_index = std::nullopt;

  for (std::size_t index = 0u; index < targets.size(); ++index) {
    const auto& spec = targets[index];
    const auto single_result = PerformResourceAnalysis(
        context,
        device_index,
        device,
        queue,
        spec.resource_view,
        spec.resource,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        PreviewMode::CLAMPED,
        "clamped",
        prefer_clone);

    json entry = {
        {"scanIndex", index},
        {"label", spec.label},
        {"drawIndex", spec.draw_index.has_value() ? json(spec.draw_index.value()) : json(nullptr)},
        {"shaderHashes", spec.shader_hashes},
        {"resourceViewHandle", spec.resource_view.handle == 0u ? json(nullptr) : json(FormatHandle(context, spec.resource_view.handle))},
        {"resourceHandle", spec.resource.handle == 0u ? json(nullptr) : json(FormatHandle(context, spec.resource.handle))},
        {"ok", !single_result.is_error},
    };

    if (single_result.is_error) {
      ++error_count;
      entry["error"] = single_result.text;
    } else {
      ++ok_count;
      const auto& sc = single_result.structured_content;
      if (sc.contains("resourceHandle")) {
        entry["resourceHandle"] = sc["resourceHandle"];
      }
      if (sc.contains("analysis")) {
        const auto& analysis = sc["analysis"];
        entry["width"] = analysis["width"];
        entry["height"] = analysis["height"];
        entry["format"] = analysis["analysisFormat"];
        entry["channels"] = analysis["channels"];
        entry["luminance"] = analysis["luminance"];
        entry["percentiles"] = analysis["percentiles"];
        entry["gamut"] = analysis["gamut"];
        entry["pixelsAnyAbove1"] = analysis["pixelsAnyChannelAbove1"];
        entry["pixelsAnyNegative"] = analysis["pixelsAnyNegative"];
        entry["nanPixels"] = analysis["nanPixels"];
        entry["infPixels"] = analysis["infPixels"];

        const auto above_one_count = analysis["pixelsAnyChannelAbove1"]["count"].get<std::uint64_t>();
        const auto negative_count = analysis["pixelsAnyNegative"]["count"].get<std::uint64_t>();
        const auto nan_count = analysis["nanPixels"]["count"].get<std::uint64_t>();
        const auto inf_count = analysis["infPixels"]["count"].get<std::uint64_t>();
        if (above_one_count > 0u) {
          last_hdr_entry_index = index;
          first_clipped_after_hdr_index = std::nullopt;
        } else if (last_hdr_entry_index.has_value() && !first_clipped_after_hdr_index.has_value()) {
          first_clipped_after_hdr_index = index;
        }
        if (negative_count > 0u) {
          last_negative_entry_index = index;
        }
        if ((nan_count > 0u || inf_count > 0u) && !first_nan_or_inf_entry_index.has_value()) {
          first_nan_or_inf_entry_index = index;
        }
      }
    }

    entries.push_back(std::move(entry));
  }

  const auto make_summary_ref = [&](const std::optional<std::size_t>& entry_index) -> json {
    if (!entry_index.has_value()) return json(nullptr);
    const auto& spec = targets[entry_index.value()];
    return json{
        {"scanIndex", entry_index.value()},
        {"label", spec.label},
        {"drawIndex", spec.draw_index.has_value() ? json(spec.draw_index.value()) : json(nullptr)},
        {"shaderHashes", spec.shader_hashes},
    };
  };

  json result = {
      {"deviceIndex", device_index},
      {"scannedCount", targets.size()},
      {"okCount", ok_count},
      {"errorCount", error_count},
      {"hdrDropOff", {
                         {"lastPassWithValuesAbove1", make_summary_ref(last_hdr_entry_index)},
                         {"firstClippedPassAfterHdr", make_summary_ref(first_clipped_after_hdr_index)},
                         {"lastPassWithNegativeValues", make_summary_ref(last_negative_entry_index)},
                         {"firstPassWithNanOrInf", make_summary_ref(first_nan_or_inf_entry_index)},
                     }},
      {"results", entries},
  };

  auto text = std::format(
      "Scanned {} resource(s) on device #{} ({} succeeded, {} failed).",
      targets.size(),
      device_index,
      ok_count,
      error_count);
  return ToolResult{
      .text = text,
      .structured_content = result,
  };
}

[[nodiscard]] inline std::pair<double, double> ComputeHueDegAndSat(double r, double g, double b) {
  const double c_max = std::max({r, g, b});
  const double c_min = std::min({r, g, b});
  const double delta = c_max - c_min;
  double hue = 0.0;
  if (delta > 1e-6) {
    if (c_max == r) {
      hue = 60.0 * std::fmod(((g - b) / delta), 6.0);
    } else if (c_max == g) {
      hue = 60.0 * (((b - r) / delta) + 2.0);
    } else {
      hue = 60.0 * (((r - g) / delta) + 4.0);
    }
    if (hue < 0.0) {
      hue += 360.0;
    }
  }
  const double sat = (c_max > 1e-6) ? (delta / c_max) : 0.0;
  return {hue, sat};
}

[[nodiscard]] inline double SignedHueDeltaDeg(double measured_deg, double reference_deg) {
  double diff = std::fmod((measured_deg - reference_deg) + 540.0, 360.0) - 180.0;
  if (diff < -180.0) diff += 360.0;
  return diff;
}

[[nodiscard]] inline ToolResult PerformLutAnalysis(
    const ProcessingContext& context,
    std::uint32_t device_index,
    reshade::api::device* device,
    reshade::api::command_queue* queue,
    reshade::api::resource_view resource_view,
    reshade::api::resource raw_resource,
    const LutAnalysisSpec& spec,
    bool prefer_clone) {
  AnalysisTarget target = {};
  try {
    target = ResolveAnalysisTarget(resource_view, raw_resource, device, prefer_clone);
  } catch (const std::exception& exception) {
    auto error = std::string(exception.what());
    if (error.starts_with("resourceViewHandle ")) {
      error = std::format("resourceViewHandle {} is not currently tracked.", FormatHandle(context, resource_view.handle));
    } else if (error.starts_with("resourceHandle ")) {
      error = std::format("resourceHandle {} is not currently tracked.", FormatHandle(context, raw_resource.handle));
    }
    return ToolResult{
        .text = error,
        .structured_content = json{{"error", error}},
        .is_error = true,
    };
  }

  const bool is_3d = (target.resolved_resource_desc.type == reshade::api::resource_type::texture_3d);
  if (target.resolved_resource_desc.type != reshade::api::resource_type::texture_2d
      && target.resolved_resource_desc.type != reshade::api::resource_type::surface
      && !is_3d) {
    return ToolResult{
        .text = "devkit_analyze_lut only supports 3D textures (texture_3d) and 2D strip LUT textures.",
        .structured_content = json{{"error", "devkit_analyze_lut only supports 3D textures (texture_3d) and 2D strip LUT textures."}},
        .is_error = true,
    };
  }
  if (target.resolved_resource_desc.texture.samples != 1u) {
    return ToolResult{
        .text = "Multisampled textures are not supported for LUT analysis.",
        .structured_content = json{{"error", "Multisampled textures are not supported for LUT analysis."}},
        .is_error = true,
    };
  }

  const auto first_level = target.resolved_view_desc.texture.first_level;
  const auto first_layer = is_3d ? 0u : target.resolved_view_desc.texture.first_layer;
  const auto levels = std::max<uint32_t>(target.resolved_resource_desc.texture.levels, 1u);
  const auto subresource = is_3d ? first_level : (first_level + (first_layer * levels));
  const auto width = std::max<uint32_t>(target.resolved_resource_desc.texture.width >> first_level, 1u);
  const auto height = std::max<uint32_t>(target.resolved_resource_desc.texture.height >> first_level, 1u);
  const auto depth = is_3d
                         ? std::max<uint32_t>(static_cast<uint32_t>(target.resolved_resource_desc.texture.depth_or_layers) >> first_level, 1u)
                         : 1u;
  const auto intermediate_format = reshade::api::format_to_default_typed(target.resolved_resource_desc.texture.format, 0);
  const auto analysis_format = target.resolved_view_desc.format != reshade::api::format::unknown
                                   ? target.resolved_view_desc.format
                                   : intermediate_format;

  std::string resolved_layout = spec.layout;
  uint32_t nx = 0u;
  uint32_t ny = 0u;
  uint32_t nz = 0u;

  if (resolved_layout == "auto") {
    if (is_3d) {
      resolved_layout = "texture_3d";
    } else if (height >= 2u && height <= 128u && width == height * height) {
      resolved_layout = "strip_2d_horizontal";
    } else if (width >= 2u && width <= 128u && height == width * width) {
      resolved_layout = "strip_2d_vertical";
    } else if (spec.lut_size.has_value()) {
      const auto n = spec.lut_size.value();
      if (width == n * n && height == n) {
        resolved_layout = "strip_2d_horizontal";
      } else if (height == n * n && width == n) {
        resolved_layout = "strip_2d_vertical";
      }
    }
  }

  if (resolved_layout == "texture_3d") {
    if (!is_3d) {
      return ToolResult{
          .text = "layout='texture_3d' requires a texture_3d resource.",
          .structured_content = json{{"error", "layout='texture_3d' requires a texture_3d resource."}},
          .is_error = true,
      };
    }
    nx = width;
    ny = height;
    nz = depth;
  } else if (resolved_layout == "strip_2d_horizontal") {
    const auto n = spec.lut_size.value_or(height);
    if (n < 2u || width != n * n || height != n) {
      const auto error = std::format(
          "Resource dimensions {}x{} do not match a horizontal 2D LUT strip of size {} ({}x{}).",
          width,
          height,
          n,
          n * n,
          n);
      return ToolResult{
          .text = error,
          .structured_content = json{{"error", error}},
          .is_error = true,
      };
    }
    nx = n;
    ny = n;
    nz = n;
  } else if (resolved_layout == "strip_2d_vertical") {
    const auto n = spec.lut_size.value_or(width);
    if (n < 2u || height != n * n || width != n) {
      const auto error = std::format(
          "Resource dimensions {}x{} do not match a vertical 2D LUT strip of size {} ({}x{}).",
          width,
          height,
          n,
          n,
          n * n);
      return ToolResult{
          .text = error,
          .structured_content = json{{"error", error}},
          .is_error = true,
      };
    }
    nx = n;
    ny = n;
    nz = n;
  } else {
    const auto error = std::format(
        "Could not auto-detect LUT layout for {}x{}x{} texture. Specify layout ('texture_3d', 'strip_2d_horizontal', 'strip_2d_vertical') and lutSize.",
        width,
        height,
        depth);
    return ToolResult{
        .text = error,
        .structured_content = json{{"error", error}},
        .is_error = true,
    };
  }

  const uint64_t voxel_count = static_cast<uint64_t>(nx) * static_cast<uint64_t>(ny) * static_cast<uint64_t>(nz);
  if (voxel_count == 0u || voxel_count > (128ull * 128ull * 128ull)) {
    const auto error = std::format("LUT volume {}x{}x{} exceeds the 128^3 analysis limit.", nx, ny, nz);
    return ToolResult{
        .text = error,
        .structured_content = json{{"error", error}},
        .is_error = true,
    };
  }

  if (HasFewerThanThreeChannels(analysis_format)) {
    const auto error = std::format(
        "Format {} has fewer than 3 channels, so this is not a color LUT (likely an exposure grid or histogram). Use devkit_analyze_resource instead.",
        FormatFormat(context, analysis_format));
    return ToolResult{
        .text = error,
        .structured_content = json{{"error", error}},
        .is_error = true,
    };
  }

  MappedTextureReadback readback;
  std::string readback_error;
  if (!readback.Acquire(
          device,
          queue,
          target.resolved_resource,
          subresource,
          is_3d,
          width,
          height,
          depth,
          intermediate_format,
          &readback_error)) {
    return ToolResult{
        .text = readback_error,
        .structured_content = json{{"error", readback_error}},
        .is_error = true,
    };
  }
  const auto& mapped_data = readback.data;

  std::vector<std::array<float, 3>> cube(static_cast<size_t>(voxel_count), {0.f, 0.f, 0.f});
  const auto slice_pitch = (mapped_data.slice_pitch != 0u)
                               ? static_cast<size_t>(mapped_data.slice_pitch)
                               : (static_cast<size_t>(mapped_data.row_pitch) * static_cast<size_t>(height));
  const auto pixel_stride = static_cast<size_t>(reshade::api::format_row_pitch(intermediate_format, 1));
  bool supported_format = true;

  for (uint32_t b = 0u; b < nz && supported_format; ++b) {
    for (uint32_t g = 0u; g < ny && supported_format; ++g) {
      for (uint32_t r = 0u; r < nx; ++r) {
        uint32_t src_x = 0u;
        uint32_t src_y = 0u;
        uint32_t src_z = 0u;
        if (resolved_layout == "texture_3d") {
          src_x = r;
          src_y = g;
          src_z = b;
        } else if (resolved_layout == "strip_2d_horizontal") {
          src_x = (b * nx) + r;
          src_y = g;
          src_z = 0u;
        } else {
          src_x = r;
          src_y = (b * ny) + g;
          src_z = 0u;
        }

        const auto* slice = static_cast<const std::uint8_t*>(mapped_data.data) + (slice_pitch * src_z);
        const auto* row = slice + (static_cast<size_t>(mapped_data.row_pitch) * src_y);
        const auto* pixel = row + (pixel_stride * src_x);
        float rgba[4] = {0.f, 0.f, 0.f, 1.f};
        uint32_t decoded_channels = 4u;
        if (!DecodePixelToLinear(analysis_format, pixel, rgba, decoded_channels)) {
          supported_format = false;
          break;
        }

        const auto cube_index = ((static_cast<size_t>(b) * static_cast<size_t>(ny)) + static_cast<size_t>(g)) * static_cast<size_t>(nx)
                                + static_cast<size_t>(r);
        cube[cube_index] = {rgba[0], rgba[1], rgba[2]};
      }
    }
  }

  readback.Release();

  if (!supported_format) {
    const auto error = std::format("LUT readback is not implemented for format {}.", FormatFormat(context, analysis_format));
    return ToolResult{
        .text = error,
        .structured_content = json{{"error", error}},
        .is_error = true,
    };
  }

  const auto sample_voxel = [&](uint32_t r, uint32_t g, uint32_t b) -> const std::array<float, 3>& {
    return cube[((static_cast<size_t>(b) * static_cast<size_t>(ny)) + static_cast<size_t>(g)) * static_cast<size_t>(nx) + static_cast<size_t>(r)];
  };

  double ch_min[3] = {
      std::numeric_limits<double>::infinity(),
      std::numeric_limits<double>::infinity(),
      std::numeric_limits<double>::infinity(),
  };
  double ch_max[3] = {
      -std::numeric_limits<double>::infinity(),
      -std::numeric_limits<double>::infinity(),
      -std::numeric_limits<double>::infinity(),
  };
  double ch_sum[3] = {0.0, 0.0, 0.0};
  uint64_t finite_count = 0u;
  uint64_t nan_count = 0u;
  uint64_t inf_count = 0u;
  uint64_t above_one_count = 0u;
  uint64_t below_zero_count = 0u;
  double identity_max_delta = 0.0;
  double identity_sum_delta = 0.0;

  for (uint32_t b = 0u; b < nz; ++b) {
    const double ib = (nz > 1u) ? (static_cast<double>(b) / static_cast<double>(nz - 1u)) : 0.0;
    for (uint32_t g = 0u; g < ny; ++g) {
      const double ig = (ny > 1u) ? (static_cast<double>(g) / static_cast<double>(ny - 1u)) : 0.0;
      for (uint32_t r = 0u; r < nx; ++r) {
        const double ir = (nx > 1u) ? (static_cast<double>(r) / static_cast<double>(nx - 1u)) : 0.0;
        const auto& rgb = sample_voxel(r, g, b);
        const bool has_nan = std::isnan(rgb[0]) || std::isnan(rgb[1]) || std::isnan(rgb[2]);
        const bool has_inf = std::isinf(rgb[0]) || std::isinf(rgb[1]) || std::isinf(rgb[2]);
        if (has_nan) ++nan_count;
        if (has_inf) ++inf_count;
        if (has_nan || has_inf) continue;

        ++finite_count;
        if (rgb[0] > 1.0001f || rgb[1] > 1.0001f || rgb[2] > 1.0001f) ++above_one_count;
        if (rgb[0] < -0.0001f || rgb[1] < -0.0001f || rgb[2] < -0.0001f) ++below_zero_count;

        for (int c = 0; c < 3; ++c) {
          ch_min[c] = std::min(ch_min[c], static_cast<double>(rgb[c]));
          ch_max[c] = std::max(ch_max[c], static_cast<double>(rgb[c]));
          ch_sum[c] += static_cast<double>(rgb[c]);
        }

        const double dr = std::abs(static_cast<double>(rgb[0]) - ir);
        const double dg = std::abs(static_cast<double>(rgb[1]) - ig);
        const double db = std::abs(static_cast<double>(rgb[2]) - ib);
        const double max_d = std::max({dr, dg, db});
        identity_max_delta = std::max(identity_max_delta, max_d);
        identity_sum_delta += (dr + dg + db) / 3.0;
      }
    }
  }

  const double global_min = finite_count > 0u ? std::min({ch_min[0], ch_min[1], ch_min[2]}) : 0.0;
  const double global_max = finite_count > 0u ? std::max({ch_max[0], ch_max[1], ch_max[2]}) : 0.0;
  const double identity_mean_delta = finite_count > 0u ? (identity_sum_delta / static_cast<double>(finite_count)) : 0.0;

  // Neutral axis (R = G = B)
  const uint32_t diag_steps = std::min({nx, ny, nz});
  struct NeutralSample {
    double input = 0.0;
    double r = 0.0;
    double g = 0.0;
    double b = 0.0;
    double lum = 0.0;
  };
  std::vector<NeutralSample> neutral_samples;
  neutral_samples.reserve(diag_steps);
  json neutral_axis_json = json::array();
  bool neutral_monotonic = true;
  double max_neutral_chroma_error = 0.0;
  double err_identity = 0.0;
  double err_lin_to_srgb = 0.0;
  double err_srgb_to_lin = 0.0;

  for (uint32_t i = 0u; i < diag_steps; ++i) {
    const double t = (diag_steps > 1u) ? (static_cast<double>(i) / static_cast<double>(diag_steps - 1u)) : 0.0;
    const auto rx = static_cast<uint32_t>(std::llround(t * static_cast<double>(nx - 1u)));
    const auto gy = static_cast<uint32_t>(std::llround(t * static_cast<double>(ny - 1u)));
    const auto bz = static_cast<uint32_t>(std::llround(t * static_cast<double>(nz - 1u)));
    const auto& rgb = sample_voxel(rx, gy, bz);
    const double lum = (0.2126 * rgb[0]) + (0.7152 * rgb[1]) + (0.0722 * rgb[2]);
    const double chroma_err = static_cast<double>(std::max({rgb[0], rgb[1], rgb[2]}) - std::min({rgb[0], rgb[1], rgb[2]}));

    if (!neutral_samples.empty() && lum < neutral_samples.back().lum - 1e-4) {
      neutral_monotonic = false;
    }
    max_neutral_chroma_error = std::max(max_neutral_chroma_error, chroma_err);

    neutral_samples.push_back(NeutralSample{
        .input = t,
        .r = rgb[0],
        .g = rgb[1],
        .b = rgb[2],
        .lum = lum,
    });

    err_identity += std::abs(lum - t);
    err_lin_to_srgb += std::abs(lum - std::pow(std::max(t, 0.0), 1.0 / 2.2));
    err_srgb_to_lin += std::abs(lum - std::pow(std::max(t, 0.0), 2.2));

    json eff_gamma = nullptr;
    if (t > 0.02 && t < 0.98 && lum > 1e-4) {
      eff_gamma = std::log(lum) / std::log(t);
    }

    neutral_axis_json.push_back(json{
        {"index", i},
        {"input", t},
        {"r", static_cast<double>(rgb[0])},
        {"g", static_cast<double>(rgb[1])},
        {"b", static_cast<double>(rgb[2])},
        {"luminance", lum},
        {"chromaError", chroma_err},
        {"effectiveGamma", eff_gamma},
    });
  }

  const auto interp_neutral_lum = [&](double x) -> double {
    if (neutral_samples.empty()) return 0.0;
    if (x <= 0.0) return neutral_samples.front().lum;
    if (x >= 1.0) return neutral_samples.back().lum;
    const double pos = x * static_cast<double>(neutral_samples.size() - 1u);
    const auto i0 = static_cast<size_t>(std::floor(pos));
    const auto i1 = std::min(i0 + 1u, neutral_samples.size() - 1u);
    const double frac = pos - static_cast<double>(i0);
    return neutral_samples[i0].lum + (frac * (neutral_samples[i1].lum - neutral_samples[i0].lum));
  };

  // Sampled at LUT coordinates, not scene values: the input shaper (sRGB, log, PQ) is unknown here.
  const double lum_010 = interp_neutral_lum(0.10);
  const double lum_018 = interp_neutral_lum(0.18);
  const double lum_025 = interp_neutral_lum(0.25);
  const double lum_050 = interp_neutral_lum(0.50);
  const double lum_075 = interp_neutral_lum(0.75);
  const double lum_100 = interp_neutral_lum(1.00);
  const double midtone_slope = (lum_050 - lum_025) / 0.25;
  const double shoulder_slope = (lum_100 - lum_075) / 0.25;

  std::string baked_tonemap_assessment = "linear_or_mild_grade";
  if (global_max > 1.05) {
    baked_tonemap_assessment = "hdr_extended_output";
  } else if (identity_max_delta < 0.005) {
    baked_tonemap_assessment = "identity_or_noop";
  } else if (midtone_slope > 0.1 && (shoulder_slope / midtone_slope) < 0.45 && lum_100 <= 1.02) {
    baked_tonemap_assessment = "likely_baked_tonemap";
  } else if (midtone_slope > 1.15 && shoulder_slope < 0.85) {
    baked_tonemap_assessment = "contrast_s_curve";
  }

  const double norm_steps = std::max<double>(static_cast<double>(diag_steps), 1.0);
  const double mae_identity = err_identity / norm_steps;
  const double mae_lin_to_srgb = err_lin_to_srgb / norm_steps;
  const double mae_srgb_to_lin = err_srgb_to_lin / norm_steps;

  // First neutral-axis coordinate where luminance reaches 98% of white, interpolated between samples.
  const auto white_reached_sample = std::ranges::find_if(neutral_samples, [lum_100](const NeutralSample& sample) { return sample.lum >= 0.98 * lum_100; });
  double white_reached_coord = 1.0;
  if (white_reached_sample != neutral_samples.end()) {
    white_reached_coord = white_reached_sample->input;
    if (white_reached_sample != neutral_samples.begin()) {
      const auto& previous = *(white_reached_sample - 1);
      const double span = white_reached_sample->lum - previous.lum;
      if (span > 0.0) {
        white_reached_coord = previous.input + ((white_reached_sample->input - previous.input) * (((0.98 * lum_100) - previous.lum) / span));
      }
    }
  }

  std::string estimated_input_space = "encoded_sdr_srgb_or_gamma22";
  if (!neutral_monotonic && (global_min < -0.05 || max_neutral_chroma_error > 0.3)) {
    // A color LUT's neutral axis rises with its input and stays near gray; a signed or strongly tinted non-monotonic
    // volume is some other 3D data.
    estimated_input_space = "not_a_color_lut";
  } else if (global_max > 1.05) {
    estimated_input_space = "hdr_linear_or_extended";
  } else if (mae_lin_to_srgb < mae_identity * 0.7 && mae_lin_to_srgb < 0.12) {
    estimated_input_space = "linear_to_encoded_gamma";
  } else if (mae_srgb_to_lin < mae_identity * 0.7 && mae_srgb_to_lin < 0.12) {
    estimated_input_space = "encoded_to_linear";
  } else if ((lum_018 > 0.34 && lum_050 > 0.65 && lum_100 <= 1.05)
             // Log/PQ shaper baked with a tonemap: crushed toe, steep mids, flat shoulder (e.g. Unreal's 32^3 log LUT)
             || (lum_010 < 0.02 && midtone_slope > 0.1 && shoulder_slope < (midtone_slope * 0.2) && lum_100 <= 1.05)) {
    // A PQ axis reaches SDR white around coord 0.5-0.65 (it spans 0..10000 nits); a log axis (e.g. Unreal) around 0.75-0.9.
    // Coords in between stay undecided instead of flipping on a hard threshold.
    estimated_input_space = (white_reached_coord < 0.65 ? "pq_shaper_to_sdr" : (white_reached_coord > 0.75 ? "log_shaper_to_sdr" : "pq_or_log_shaper_to_sdr"));
  } else if (identity_max_delta < 0.005) {
    estimated_input_space = "identity_space_preserving";
  }

  // Gamma and identity comparisons assume a linear or gamma-encoded input axis; on a shaped axis they mean nothing.
  const bool shaped_input_axis = estimated_input_space.ends_with("_shaper_to_sdr");
  if (shaped_input_axis) {
    for (auto& entry : neutral_axis_json) {
      entry["effectiveGamma"] = nullptr;
    }
  }

  const auto make_corner_json = [&](const char* name, uint32_t r, uint32_t g, uint32_t b, std::optional<double> ref_hue_deg) {
    const auto& rgb = sample_voxel(r, g, b);
    json entry = {
        {"name", name},
        {"r", static_cast<double>(rgb[0])},
        {"g", static_cast<double>(rgb[1])},
        {"b", static_cast<double>(rgb[2])},
        {"luminance", (0.2126 * rgb[0]) + (0.7152 * rgb[1]) + (0.0722 * rgb[2])},
    };
    if (ref_hue_deg.has_value()) {
      const auto [hue_deg, sat] = ComputeHueDegAndSat(rgb[0], rgb[1], rgb[2]);
      entry["hueDeg"] = hue_deg;
      entry["hueShiftDeg"] = SignedHueDeltaDeg(hue_deg, ref_hue_deg.value());
      entry["saturation"] = sat;
    }
    return entry;
  };

  json corners_json = {
      {"black", make_corner_json("black", 0u, 0u, 0u, std::nullopt)},
      {"white", make_corner_json("white", nx - 1u, ny - 1u, nz - 1u, std::nullopt)},
      {"red", make_corner_json("red", nx - 1u, 0u, 0u, 0.0)},
      {"yellow", make_corner_json("yellow", nx - 1u, ny - 1u, 0u, 60.0)},
      {"green", make_corner_json("green", 0u, ny - 1u, 0u, 120.0)},
      {"cyan", make_corner_json("cyan", 0u, ny - 1u, nz - 1u, 180.0)},
      {"blue", make_corner_json("blue", 0u, 0u, nz - 1u, 240.0)},
      {"magenta", make_corner_json("magenta", nx - 1u, 0u, nz - 1u, 300.0)},
  };

  const auto& black_rgb = sample_voxel(0u, 0u, 0u);
  const auto& white_rgb = sample_voxel(nx - 1u, ny - 1u, nz - 1u);
  const double black_lift = static_cast<double>(std::max({black_rgb[0], black_rgb[1], black_rgb[2]}));
  const double white_lum = (0.2126 * white_rgb[0]) + (0.7152 * white_rgb[1]) + (0.0722 * white_rgb[2]);
  const bool is_cubic = (nx == ny && ny == nz);

  json exports_json = json::object();
  if (spec.export_cube_path.has_value()) {
    if (!is_cubic) {
      return ToolResult{
          .text = std::format("Cannot export non-cubic LUT ({}x{}x{}) to .cube format.", nx, ny, nz),
          .structured_content = json{{"error", std::format("Cannot export non-cubic LUT ({}x{}x{}) to .cube format.", nx, ny, nz)}},
          .is_error = true,
      };
    }
    std::error_code dir_error;
    if (!spec.export_cube_path->parent_path().empty()) {
      std::filesystem::create_directories(spec.export_cube_path->parent_path(), dir_error);
    }
    std::ofstream cube_out(spec.export_cube_path.value(), std::ios::out | std::ios::trunc);
    if (!cube_out.is_open()) {
      const auto error = std::format("Failed to open '{}' for .cube export.", spec.export_cube_path->string());
      return ToolResult{
          .text = error,
          .structured_content = json{{"error", error}},
          .is_error = true,
      };
    }
    cube_out << "TITLE \"RenoDX DevKit LUT " << FormatHandle(context, target.resolved_resource.handle) << "\"\n";
    cube_out << "LUT_3D_SIZE " << nx << "\n";
    cube_out << "DOMAIN_MIN 0.0 0.0 0.0\n";
    cube_out << "DOMAIN_MAX 1.0 1.0 1.0\n";
    cube_out << std::fixed << std::setprecision(6);
    for (const auto& rgb : cube) {
      const float r_val = std::isfinite(rgb[0]) ? rgb[0] : 0.f;
      const float g_val = std::isfinite(rgb[1]) ? rgb[1] : 0.f;
      const float b_val = std::isfinite(rgb[2]) ? rgb[2] : 0.f;
      cube_out << r_val << ' ' << g_val << ' ' << b_val << '\n';
    }
    cube_out.flush();
    exports_json["cube"] = {
        {"path", spec.export_cube_path->string()},
        {"lutSize", nx},
        {"entryCount", cube.size()},
    };
  }

  if (spec.export_preview_path.has_value()) {
    std::error_code dir_error;
    if (!spec.export_preview_path->parent_path().empty()) {
      std::filesystem::create_directories(spec.export_preview_path->parent_path(), dir_error);
    }
    const auto preview_fmt = GetOutputFormat(spec.export_preview_path.value());
    const uint32_t strip_w = nx * nz;
    const uint32_t strip_h = ny;
    if (preview_fmt == OutputFormat::EXR) {
      std::vector<float> exr_strip(static_cast<size_t>(strip_w) * static_cast<size_t>(strip_h) * 4u, 1.f);
      for (uint32_t b = 0u; b < nz; ++b) {
        for (uint32_t g = 0u; g < ny; ++g) {
          for (uint32_t r = 0u; r < nx; ++r) {
            const auto& rgb = sample_voxel(r, g, b);
            const auto sx = (b * nx) + r;
            const auto offset = ((static_cast<size_t>(g) * static_cast<size_t>(strip_w)) + static_cast<size_t>(sx)) * 4u;
            exr_strip[offset + 0u] = rgb[0];
            exr_strip[offset + 1u] = rgb[1];
            exr_strip[offset + 2u] = rgb[2];
            exr_strip[offset + 3u] = 1.f;
          }
        }
      }
      renodx::utils::exr::WriteRgba16f(
          spec.export_preview_path.value(),
          strip_w,
          strip_h,
          std::span<const float>(exr_strip.data(), exr_strip.size()));
      exports_json["preview"] = {{"path", spec.export_preview_path->string()}, {"format", "exr"}, {"width", strip_w}, {"height", strip_h}};
    } else {
      std::vector<std::uint8_t> png_strip(static_cast<size_t>(strip_w) * static_cast<size_t>(strip_h) * 4u, 255u);
      for (uint32_t b = 0u; b < nz; ++b) {
        for (uint32_t g = 0u; g < ny; ++g) {
          for (uint32_t r = 0u; r < nx; ++r) {
            const auto& rgb = sample_voxel(r, g, b);
            const float rgba[4] = {rgb[0], rgb[1], rgb[2], 1.f};
            const auto sx = (b * nx) + r;
            const auto offset = ((static_cast<size_t>(g) * static_cast<size_t>(strip_w)) + static_cast<size_t>(sx)) * 4u;
            EncodePreviewPixel(rgba, PreviewMode::CLAMPED, png_strip.data() + offset);
          }
        }
      }
      if (!WritePngPreview(spec.export_preview_path.value(), strip_w, strip_h, png_strip)) {
        const auto error = std::format("Failed to write LUT preview PNG to '{}'.", spec.export_preview_path->string());
        return ToolResult{
            .text = error,
            .structured_content = json{{"error", error}},
            .is_error = true,
        };
      }
      exports_json["preview"] = {{"path", spec.export_preview_path->string()}, {"format", "png"}, {"width", strip_w}, {"height", strip_h}};
    }
  }

  json result = {
      {"deviceIndex", device_index},
      {"resourceViewHandle", resource_view.handle == 0u ? json(nullptr) : json(FormatHandle(context, resource_view.handle))},
      {"resourceHandle", FormatHandle(context, target.resolved_resource.handle)},
      {"layout", resolved_layout},
      {"format", FormatFormat(context, analysis_format)},
      {"dimensions", {{"nx", nx}, {"ny", ny}, {"nz", nz}, {"isCubic", is_cubic}, {"voxelCount", voxel_count}}},
      {"range", {
                    {"min", global_min},
                    {"max", global_max},
                    {"clipsToSdr", global_max <= 1.001 && global_min >= -0.001},
                    {"hasHdrValues", global_max > 1.001},
                    {"hasNegativeValues", global_min < -0.001},
                    {"voxelsAboveOne", RatioStats(above_one_count, finite_count)},
                    {"voxelsBelowZero", RatioStats(below_zero_count, finite_count)},
                    {"nanCount", nan_count},
                    {"infCount", inf_count},
                    {"channels", {
                                     {"r", ScalarStats(ch_min[0], ch_max[0], ch_sum[0], finite_count)},
                                     {"g", ScalarStats(ch_min[1], ch_max[1], ch_sum[1], finite_count)},
                                     {"b", ScalarStats(ch_min[2], ch_max[2], ch_sum[2], finite_count)},
                                 }},
                }},
      {"identity", {
                       {"maxDelta", identity_max_delta},
                       {"meanDelta", identity_mean_delta},
                       {"isNearIdentity", shaped_input_axis ? json(nullptr) : json(identity_max_delta < 0.005)},
                   }},
      {"neutralAxisSummary", {
                                 {"blackLift", black_lift},
                                 {"whiteLuminance", white_lum},
                                 {"luminanceAtCoord018", lum_018},
                                 {"luminanceAtCoord050", lum_050},
                                 {"midtoneSlope", midtone_slope},
                                 {"shoulderSlope", shoulder_slope},
                                 {"monotonic", neutral_monotonic},
                                 {"bakedTonemapAssessment", baked_tonemap_assessment},
                                 {"estimatedInputSpace", estimated_input_space},
                                 {"whiteReachedAtCoord", white_reached_coord},
                                 {"curveErrors", {
                                                     {"maeIdentity", mae_identity},
                                                     {"maeLinearToGamma22", mae_lin_to_srgb},
                                                     {"maeGamma22ToLinear", mae_srgb_to_lin},
                                                 }},
                             }},
      {"neutralAxis", neutral_axis_json},
      {"corners", corners_json},
  };
  if (!exports_json.empty()) {
    result["exports"] = exports_json;
  }

  if (spec.export_json_path.has_value()) {
    std::error_code dir_error;
    if (!spec.export_json_path->parent_path().empty()) {
      std::filesystem::create_directories(spec.export_json_path->parent_path(), dir_error);
    }
    std::ofstream json_out(spec.export_json_path.value(), std::ios::out | std::ios::trunc);
    if (!json_out.is_open()) {
      const auto error = std::format("Failed to open '{}' for JSON export.", spec.export_json_path->string());
      return ToolResult{
          .text = error,
          .structured_content = json{{"error", error}},
          .is_error = true,
      };
    }
    json_out << result.dump(2) << '\n';
    json_out.flush();
    result["exports"]["json"] = {{"path", spec.export_json_path->string()}};
  }

  auto text = std::format(
      "Analyzed {}x{}x{} {} LUT ({}) on device #{}: range [{:.4f}, {:.4f}], blackLift={:.4f}, whiteLum={:.4f}, assessment={}, inputSpace={}.",
      nx,
      ny,
      nz,
      FormatFormat(context, analysis_format),
      resolved_layout,
      device_index,
      global_min,
      global_max,
      black_lift,
      white_lum,
      baked_tonemap_assessment,
      estimated_input_space);
  return ToolResult{
      .text = text,
      .structured_content = result,
  };
}

}  // namespace internal

[[nodiscard]] inline ToolResult HandleAnalyzeResourceTool(const json& arguments, const ToolContext& context) {
  if (!context.resolve_device_index || !context.enqueue_request) {
    throw std::runtime_error("AnalyzeResourceToolContext is not initialized.");
  }

  const auto resource_view_handle = resource_handle::GetRequired(arguments, "resourceViewHandle");
  const auto device_index = context.resolve_device_index(arguments);
  const auto output_path_value = mcp_arguments::GetOptional<std::string>(arguments, "outputPath").value_or("");
  const auto preview_mode_value = mcp_arguments::GetOptional<std::string>(arguments, "previewMode").value_or("clamped");
  const auto preview_mode = internal::ParsePreviewMode(preview_mode_value);
  const auto prefer_clone = mcp_arguments::GetOptional<bool>(arguments, "preferClone").value_or(false);

  std::optional<std::filesystem::path> output_path = std::nullopt;
  if (!output_path_value.empty()) {
    auto requested_path = std::filesystem::path(output_path_value);
    if (!requested_path.is_absolute()) {
      throw std::runtime_error("outputPath must be an absolute path.");
    }
    if (!requested_path.has_extension()) {
      requested_path.replace_extension(L".png");
    } else if (requested_path.extension() != L".png" && requested_path.extension() != L".exr") {
      throw std::runtime_error("outputPath must end in '.png' or '.exr'.");
    }
    output_path = requested_path.lexically_normal();
  }

  auto request = std::make_shared<PendingRequest>();
  request->resource_view = reshade::api::resource_view{resource_view_handle};
  request->output_path = std::move(output_path);
  request->output_directory = std::nullopt;
  request->name_prefix = std::nullopt;
  request->preview_mode = preview_mode;
  request->preview_mode_name = std::string(internal::GetPreviewModeName(preview_mode));
  request->prefer_clone = prefer_clone;

  if (!context.enqueue_request(device_index, request)) {
    const auto error = std::format("Failed to queue the resource analysis request for device #{}.", device_index);
    return ToolResult{
        .text = error,
        .structured_content = json{{"error", error}},
        .is_error = true,
    };
  }

  {
    std::unique_lock request_lock(request->mutex);
    if (!request->cv.wait_for(request_lock, std::chrono::seconds(15), [&request] { return request->completed || request->started; })) {
      request->canceled = true;
      request_lock.unlock();
      if (context.cancel_request) {
        context.cancel_request(device_index, request);
      }
      return ToolResult{
          .text = "Timed out waiting for the next present to process the resource analysis request.",
          .structured_content = json{{"error", "Timed out waiting for the next present to process the resource analysis request."}},
          .is_error = true,
      };
    }

    if (!request->completed) {
      request->cv.wait(request_lock, [&request] { return request->completed; });
    }
  }

  if (!request->result.has_value()) {
    return ToolResult{
        .text = "The resource analysis request completed without a result.",
        .structured_content = json{{"error", "The resource analysis request completed without a result."}},
        .is_error = true,
    };
  }

  return request->result.value();
}

[[nodiscard]] inline ToolResult HandleDumpResourceWithHashTool(const json& arguments, const ToolContext& context) {
  if (!context.resolve_device_index || !context.enqueue_request) {
    throw std::runtime_error("AnalyzeResourceToolContext is not initialized.");
  }

  const auto resource_view_handle = resource_handle::GetRequired(arguments, "resourceViewHandle");
  const auto device_index = context.resolve_device_index(arguments);
  const auto output_directory_value = mcp_arguments::GetRequired<std::string>(arguments, "outputDirectory");
  const auto prefer_clone = mcp_arguments::GetOptional<bool>(arguments, "preferClone").value_or(false);
  const auto name_prefix = mcp_arguments::GetOptional<std::string>(arguments, "namePrefix");

  auto output_directory = std::filesystem::path(output_directory_value);
  if (!output_directory.is_absolute()) {
    throw std::runtime_error("outputDirectory must be an absolute path.");
  }
  output_directory = output_directory.lexically_normal();

  auto request = std::make_shared<PendingRequest>();
  request->resource_view = reshade::api::resource_view{resource_view_handle};
  request->output_path = std::nullopt;
  request->output_directory = output_directory;
  request->name_prefix = name_prefix.has_value() && !name_prefix->empty()
                             ? std::optional<std::string>(*name_prefix)
                             : std::nullopt;
  request->preview_mode = PreviewMode::CLAMPED;
  request->preview_mode_name = std::string(internal::GetPreviewModeName(PreviewMode::CLAMPED));
  request->prefer_clone = prefer_clone;

  if (!context.enqueue_request(device_index, request)) {
    const auto error = std::format("Failed to queue the resource dump request for device #{}.", device_index);
    return ToolResult{
        .text = error,
        .structured_content = json{{"error", error}},
        .is_error = true,
    };
  }

  {
    std::unique_lock request_lock(request->mutex);
    if (!request->cv.wait_for(request_lock, std::chrono::seconds(15), [&request] { return request->completed || request->started; })) {
      request->canceled = true;
      request_lock.unlock();
      if (context.cancel_request) {
        context.cancel_request(device_index, request);
      }
      return ToolResult{
          .text = "Timed out waiting for the next present to process the resource dump request.",
          .structured_content = json{{"error", "Timed out waiting for the next present to process the resource dump request."}},
          .is_error = true,
      };
    }

    if (!request->completed) {
      request->cv.wait(request_lock, [&request] { return request->completed; });
    }
  }

  if (!request->result.has_value()) {
    return ToolResult{
        .text = "The resource dump request completed without a result.",
        .structured_content = json{{"error", "The resource dump request completed without a result."}},
        .is_error = true,
    };
  }

  return request->result.value();
}

[[nodiscard]] inline ToolResult HandleScanPassResourcesTool(const json& arguments, const ToolContext& context) {
  if (!context.resolve_device_index || !context.enqueue_request) {
    throw std::runtime_error("AnalyzeResourceToolContext is not initialized.");
  }

  const auto device_index = context.resolve_device_index(arguments);
  const auto max_resources = mcp_arguments::ValidateRange(
      "maxResources",
      mcp_arguments::GetOptional<std::uint32_t>(arguments, "maxResources").value_or(16u),
      1u,
      32u);
  const auto prefer_clone = mcp_arguments::GetOptional<bool>(arguments, "preferClone").value_or(false);
  const auto include_output_chain = mcp_arguments::GetOptional<bool>(arguments, "includeOutputChain").value_or(false);

  std::vector<ScanTargetSpec> targets;
  std::vector<std::pair<std::uint64_t, std::uint64_t>> seen_keys;
  const auto try_add_target = [&](ScanTargetSpec spec) {
    if (targets.size() >= max_resources) return;
    if (spec.resource_view.handle == 0u && spec.resource.handle == 0u) return;
    const auto key = std::make_pair(spec.resource_view.handle, spec.resource.handle);
    if (std::find(seen_keys.begin(), seen_keys.end(), key) != seen_keys.end()) return;
    seen_keys.push_back(key);
    targets.push_back(std::move(spec));
  };

  if (arguments.contains("resourceViewHandles") && arguments["resourceViewHandles"].is_array()) {
    for (const auto& item : arguments["resourceViewHandles"]) {
      const auto handle = resource_handle::Parse("resourceViewHandles[]", item);
      try_add_target(ScanTargetSpec{
          .resource_view = reshade::api::resource_view{handle},
          .label = item.is_string() ? item.get<std::string>() : std::format("0x{:016X}", handle),
      });
    }
  }

  if (arguments.contains("resourceHandles") && arguments["resourceHandles"].is_array()) {
    for (const auto& item : arguments["resourceHandles"]) {
      const auto handle = resource_handle::Parse("resourceHandles[]", item);
      try_add_target(ScanTargetSpec{
          .resource = reshade::api::resource{handle},
          .label = item.is_string() ? item.get<std::string>() : std::format("0x{:016X}", handle),
      });
    }
  }

  std::vector<std::uint32_t> draw_indices;
  if (arguments.contains("drawIndices") && arguments["drawIndices"].is_array()) {
    for (const auto& item : arguments["drawIndices"]) {
      if (!item.is_number_integer() || item.get<std::int64_t>() < 0) {
        throw std::runtime_error("drawIndices entries must be non-negative integers.");
      }
      draw_indices.push_back(static_cast<std::uint32_t>(item.get<std::uint64_t>()));
    }
  }

  if ((!draw_indices.empty() || include_output_chain) && context.resolve_scan_targets) {
    for (auto& spec : context.resolve_scan_targets(device_index, draw_indices, include_output_chain)) {
      try_add_target(std::move(spec));
    }
  }

  if (targets.empty()) {
    throw std::runtime_error(
        "No scan targets were resolved. Provide resourceViewHandles, resourceHandles, drawIndices, or set includeOutputChain=true with an active snapshot.");
  }

  auto request = std::make_shared<PendingRequest>();
  request->batch_targets = std::move(targets);
  request->prefer_clone = prefer_clone;

  if (!context.enqueue_request(device_index, request)) {
    const auto error = std::format("Failed to queue the batch resource scan request for device #{}.", device_index);
    return ToolResult{
        .text = error,
        .structured_content = json{{"error", error}},
        .is_error = true,
    };
  }

  {
    std::unique_lock request_lock(request->mutex);
    if (!request->cv.wait_for(request_lock, std::chrono::seconds(20), [&request] { return request->completed || request->started; })) {
      request->canceled = true;
      request_lock.unlock();
      if (context.cancel_request) {
        context.cancel_request(device_index, request);
      }
      return ToolResult{
          .text = "Timed out waiting for the next present to process the batch resource scan request.",
          .structured_content = json{{"error", "Timed out waiting for the next present to process the batch resource scan request."}},
          .is_error = true,
      };
    }

    if (!request->completed) {
      request->cv.wait(request_lock, [&request] { return request->completed; });
    }
  }

  if (!request->result.has_value()) {
    return ToolResult{
        .text = "The batch resource scan request completed without a result.",
        .structured_content = json{{"error", "The batch resource scan request completed without a result."}},
        .is_error = true,
    };
  }

  return request->result.value();
}

[[nodiscard]] inline ToolResult HandleAnalyzeLutTool(const json& arguments, const ToolContext& context) {
  if (!context.resolve_device_index || !context.enqueue_request) {
    throw std::runtime_error("AnalyzeResourceToolContext is not initialized.");
  }

  const auto device_index = context.resolve_device_index(arguments);
  const auto prefer_clone = mcp_arguments::GetOptional<bool>(arguments, "preferClone").value_or(false);
  const auto layout = mcp_arguments::GetOptional<std::string>(arguments, "layout").value_or("auto");
  const auto lut_size = mcp_arguments::GetOptional<std::uint32_t>(arguments, "lutSize");

  uint64_t view_handle = 0u;
  uint64_t res_handle = 0u;
  if (arguments.contains("resourceViewHandle") && !arguments["resourceViewHandle"].is_null()) {
    view_handle = resource_handle::GetRequired(arguments, "resourceViewHandle");
  }
  if (arguments.contains("resourceHandle") && !arguments["resourceHandle"].is_null()) {
    res_handle = resource_handle::GetRequired(arguments, "resourceHandle");
  }
  if (view_handle == 0u && res_handle == 0u) {
    throw std::runtime_error("Either resourceViewHandle or resourceHandle is required.");
  }

  const auto parse_abs_path = [&arguments](std::string_view key, std::string_view expected_ext) -> std::optional<std::filesystem::path> {
    const auto raw = mcp_arguments::GetOptional<std::string>(arguments, key).value_or("");
    if (raw.empty()) return std::nullopt;
    auto path = std::filesystem::path(raw);
    if (!path.is_absolute()) {
      throw std::runtime_error(std::format("{} must be an absolute path.", key));
    }
    if (!expected_ext.empty() && !path.has_extension()) {
      path.replace_extension(expected_ext);
    }
    return path.lexically_normal();
  };

  LutAnalysisSpec spec = {
      .layout = layout,
      .lut_size = lut_size,
      .export_cube_path = parse_abs_path("exportCubePath", ".cube"),
      .export_json_path = parse_abs_path("exportJsonPath", ".json"),
      .export_preview_path = parse_abs_path("exportPreviewPath", ".png"),
  };
  if (spec.export_preview_path.has_value()) {
    const auto ext = spec.export_preview_path->extension();
    if (ext != L".png" && ext != L".exr") {
      throw std::runtime_error("exportPreviewPath must end in '.png' or '.exr'.");
    }
  }

  auto request = std::make_shared<PendingRequest>();
  request->resource_view = reshade::api::resource_view{view_handle};
  request->resource = reshade::api::resource{res_handle};
  request->lut_analysis = std::move(spec);
  request->prefer_clone = prefer_clone;

  if (!context.enqueue_request(device_index, request)) {
    const auto error = std::format("Failed to queue the LUT analysis request for device #{}.", device_index);
    return ToolResult{
        .text = error,
        .structured_content = json{{"error", error}},
        .is_error = true,
    };
  }

  {
    std::unique_lock request_lock(request->mutex);
    if (!request->cv.wait_for(request_lock, std::chrono::seconds(15), [&request] { return request->completed || request->started; })) {
      request->canceled = true;
      request_lock.unlock();
      if (context.cancel_request) {
        context.cancel_request(device_index, request);
      }
      return ToolResult{
          .text = "Timed out waiting for the next present to process the LUT analysis request.",
          .structured_content = json{{"error", "Timed out waiting for the next present to process the LUT analysis request."}},
          .is_error = true,
      };
    }

    if (!request->completed) {
      request->cv.wait(request_lock, [&request] { return request->completed; });
    }
  }

  if (!request->result.has_value()) {
    return ToolResult{
        .text = "The LUT analysis request completed without a result.",
        .structured_content = json{{"error", "The LUT analysis request completed without a result."}},
        .is_error = true,
    };
  }

  return request->result.value();
}

[[nodiscard]] inline ToolResult HandleCompareResourceAbTool(const json& arguments, const ToolContext& context) {
  if (!context.resolve_device_index || !context.enqueue_request) {
    throw std::runtime_error("AnalyzeResourceToolContext is not initialized.");
  }

  const auto device_index = context.resolve_device_index(arguments);
  const auto prefer_clone = mcp_arguments::GetOptional<bool>(arguments, "preferClone").value_or(false);
  uint64_t view_handle = 0u;
  uint64_t res_handle = 0u;
  if (arguments.contains("resourceViewHandle") && !arguments["resourceViewHandle"].is_null()) {
    view_handle = resource_handle::GetRequired(arguments, "resourceViewHandle");
  }
  if (arguments.contains("resourceHandle") && !arguments["resourceHandle"].is_null()) {
    res_handle = resource_handle::GetRequired(arguments, "resourceHandle");
  }
  if (view_handle == 0u && res_handle == 0u) {
    throw std::runtime_error("Either resourceViewHandle or resourceHandle is required.");
  }

  auto request = std::make_shared<PendingRequest>();
  request->resource_view = reshade::api::resource_view{view_handle};
  request->resource = reshade::api::resource{res_handle};
  request->prefer_clone = prefer_clone;
  request->ab_compare = AbCompareSpec{.active = true, .phase = 0u};

  if (!context.enqueue_request(device_index, request)) {
    const auto error = std::format("Failed to queue A/B resource comparison for device #{}.", device_index);
    return ToolResult{
        .text = error,
        .structured_content = json{{"error", error}},
        .is_error = true,
    };
  }

  {
    std::unique_lock request_lock(request->mutex);
    if (!request->cv.wait_for(request_lock, std::chrono::seconds(20), [&request] { return request->completed || request->started; })) {
      request->canceled = true;
      request_lock.unlock();
      if (context.cancel_request) {
        context.cancel_request(device_index, request);
      }
      return ToolResult{
          .text = "Timed out waiting for present to run A/B resource comparison.",
          .structured_content = json{{"error", "Timed out waiting for present to run A/B resource comparison."}},
          .is_error = true,
      };
    }
    if (!request->completed) {
      request->cv.wait(request_lock, [&request] { return request->completed; });
    }
  }

  if (!request->result.has_value()) {
    return ToolResult{
        .text = "The A/B resource comparison completed without a result.",
        .structured_content = json{{"error", "The A/B resource comparison completed without a result."}},
        .is_error = true,
    };
  }
  return request->result.value();
}

inline void ProcessPendingRequests(
    const std::deque<std::shared_ptr<PendingRequest>>& requests,
    std::uint32_t device_index,
    reshade::api::device* device,
    reshade::api::command_queue* queue,
    const ProcessingContext& context) {
  for (const auto& request : requests) {
    {
      std::scoped_lock request_lock(request->mutex);
      if (request->completed || request->canceled) {
        request->completed = true;
        request->cv.notify_all();
        continue;
      }
      request->started = true;
    }
    request->cv.notify_all();

    if (request->ab_compare.has_value()) {
      auto& ab = request->ab_compare.value();
      try {
        if (ab.phase == 0u) {
          ab.with_live_result = internal::PerformResourceAnalysis(
              context,
              device_index,
              device,
              queue,
              request->resource_view,
              request->resource,
              std::nullopt,
              std::nullopt,
              std::nullopt,
              PreviewMode::CLAMPED,
              "clamped",
              request->prefer_clone);

          if (ab.with_live_result->is_error) {
            std::scoped_lock request_lock(request->mutex);
            request->result = ab.with_live_result;
            request->completed = true;
            request->cv.notify_all();
            continue;
          }

          ab.suspended_live_shaders = context.suspend_live_shaders ? context.suspend_live_shaders(device) : false;
          if (!ab.suspended_live_shaders || !context.requeue_request) {
            json result = {
                {"deviceIndex", device_index},
                {"liveShadersWereActive", false},
                {"withLiveShaders", ab.with_live_result->structured_content},
                {"vanilla", nullptr},
                {"delta", nullptr},
            };
            std::scoped_lock request_lock(request->mutex);
            request->result = ToolResult{
                .text = "No active live shaders were loaded to toggle for A/B comparison; returned current resource analysis only.",
                .structured_content = result,
            };
            request->completed = true;
            request->cv.notify_all();
            continue;
          }

          ab.phase = 1u;
          context.requeue_request(device_index, request);
          continue;
        }

        // Phase 1 (next frame, rendered vanilla):
        const auto vanilla_result = internal::PerformResourceAnalysis(
            context,
            device_index,
            device,
            queue,
            request->resource_view,
            request->resource,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            PreviewMode::CLAMPED,
            "clamped",
            request->prefer_clone);

        if (ab.suspended_live_shaders && context.restore_live_shaders) {
          context.restore_live_shaders(device);
        }

        if (vanilla_result.is_error) {
          std::scoped_lock request_lock(request->mutex);
          request->result = vanilla_result;
          request->completed = true;
          request->cv.notify_all();
          continue;
        }

        const auto& a = ab.with_live_result->structured_content["analysis"];
        const auto& b = vanilla_result.structured_content["analysis"];
        const auto num_delta = [](const json& lhs, const json& rhs) -> json {
          if (lhs.is_number() && rhs.is_number()) {
            return json(lhs.get<double>() - rhs.get<double>());
          }
          return json(nullptr);
        };
        const auto int_delta = [](const json& lhs, const json& rhs) -> json {
          if (lhs.is_number_integer() && rhs.is_number_integer()) {
            return json(lhs.get<std::int64_t>() - rhs.get<std::int64_t>());
          }
          return json(nullptr);
        };

        json delta = {
            {"luminanceMaxDelta", num_delta(a["luminance"]["max"], b["luminance"]["max"])},
            {"luminanceMeanDelta", num_delta(a["luminance"]["mean"], b["luminance"]["mean"])},
            {"luminanceP99Delta", num_delta(a["percentiles"]["luminanceP99"], b["percentiles"]["luminanceP99"])},
            {"luminanceP999Delta", num_delta(a["percentiles"]["luminanceP999"], b["percentiles"]["luminanceP999"])},
            {"maxRgbP99Delta", num_delta(a["percentiles"]["maxRgbP99"], b["percentiles"]["maxRgbP99"])},
            {"maxRgbP999Delta", num_delta(a["percentiles"]["maxRgbP999"], b["percentiles"]["maxRgbP999"])},
            {"pixelsAboveOneDelta", int_delta(a["pixelsAnyChannelAbove1"]["count"], b["pixelsAnyChannelAbove1"]["count"])},
            {"pixelsNegativeDelta", int_delta(a["pixelsAnyNegative"]["count"], b["pixelsAnyNegative"]["count"])},
            {"nanPixelsDelta", int_delta(a["nanPixels"]["count"], b["nanPixels"]["count"])},
            {"infPixelsDelta", int_delta(a["infPixels"]["count"], b["infPixels"]["count"])},
            {"gamutBt709RatioDelta", num_delta(a["gamut"]["bt709"]["ratio"], b["gamut"]["bt709"]["ratio"])},
            {"gamutDciP3OnlyRatioDelta", num_delta(a["gamut"]["dciP3Only"]["ratio"], b["gamut"]["dciP3Only"]["ratio"])},
            {"gamutBt2020OnlyRatioDelta", num_delta(a["gamut"]["bt2020Only"]["ratio"], b["gamut"]["bt2020Only"]["ratio"])},
        };

        json result = {
            {"deviceIndex", device_index},
            {"liveShadersWereActive", true},
            {"delta", delta},
            {"withLiveShaders", a},
            {"vanilla", b},
        };

        std::scoped_lock request_lock(request->mutex);
        request->result = ToolResult{
            .text = std::format(
                "Completed live-shader A/B comparison on device #{}: maxLum delta={}, pixelsAbove1 delta={}, pixelsNegative delta={}, nan delta={}.",
                device_index,
                delta["luminanceMaxDelta"].dump(),
                delta["pixelsAboveOneDelta"].dump(),
                delta["pixelsNegativeDelta"].dump(),
                delta["nanPixelsDelta"].dump()),
            .structured_content = result,
        };
        request->completed = true;
      } catch (const std::exception& exception) {
        if (ab.suspended_live_shaders && context.restore_live_shaders) {
          context.restore_live_shaders(device);
        }
        std::scoped_lock request_lock(request->mutex);
        request->result = ToolResult{
            .text = exception.what(),
            .structured_content = json{{"error", exception.what()}},
            .is_error = true,
        };
        request->completed = true;
      }
      request->cv.notify_all();
      continue;
    }

    ToolResult result = {
        .text = "Unknown resource analysis failure.",
        .structured_content = json{{"error", "Unknown resource analysis failure."}},
        .is_error = true,
    };
    try {
      if (request->batch_targets.has_value()) {
        result = internal::PerformBatchPassScan(
            context,
            device_index,
            device,
            queue,
            request->batch_targets.value(),
            request->prefer_clone);
      } else if (request->lut_analysis.has_value()) {
        result = internal::PerformLutAnalysis(
            context,
            device_index,
            device,
            queue,
            request->resource_view,
            request->resource,
            request->lut_analysis.value(),
            request->prefer_clone);
      } else {
        result = internal::PerformResourceAnalysis(
            context,
            device_index,
            device,
            queue,
            request->resource_view,
            request->resource,
            request->output_path,
            request->output_directory,
            request->name_prefix,
            request->preview_mode,
            request->preview_mode_name,
            request->prefer_clone);
      }
    } catch (const std::exception& exception) {
      result = ToolResult{
          .text = exception.what(),
          .structured_content = json{{"error", exception.what()}},
          .is_error = true,
      };
    }

    {
      std::scoped_lock request_lock(request->mutex);
      request->result = std::move(result);
      request->completed = true;
    }
    request->cv.notify_all();
  }
}

}  // namespace renodx::addons::devkit::mcp::resource_analysis
