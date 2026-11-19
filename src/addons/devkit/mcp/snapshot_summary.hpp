/*
 * Copyright (C) 2026 Carlos Lopez
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstddef>
#include <cstdint>

#include "../../../utils/mcp/types.hpp"

namespace renodx::addons::devkit::mcp::snapshot_summary {

using json = renodx::utils::mcp::json;

struct SnapshotSummary {
  std::uint32_t device_index = 0u;
  bool snapshot_queued = false;
  bool snapshot_active = false;
  std::size_t captured_draws = 0u;
  std::size_t tracked_shaders = 0u;
  std::size_t resource_usage_entries = 0u;
  std::size_t shader_usage_entries = 0u;
  std::size_t snapshot_rows = 0u;
  bool snapshot_rows_valid = false;
  std::uint32_t snapshot_capture_count = 0u;
  std::int64_t snapshot_captured_at_ms = 0;
};

// NOLINTNEXTLINE(readability-identifier-naming)
inline void to_json(json& j, const SnapshotSummary& value) {
  j = {
      {"deviceIndex", value.device_index},
      {"snapshotQueued", value.snapshot_queued},
      {"snapshotActive", value.snapshot_active},
      {"capturedDraws", value.captured_draws},
      {"trackedShaders", value.tracked_shaders},
      {"resourceUsageEntries", value.resource_usage_entries},
      {"shaderUsageEntries", value.shader_usage_entries},
      {"snapshotRows", value.snapshot_rows},
      {"snapshotRowsValid", value.snapshot_rows_valid},
      // snapshotRows* describe the overlay's row cache; these describe the capture itself.
      {"snapshotCaptureCount", value.snapshot_capture_count},
      {"snapshotCapturedAtMs", value.snapshot_captured_at_ms == 0 ? json(nullptr) : json(value.snapshot_captured_at_ms)},
  };
}

}  // namespace renodx::addons::devkit::mcp::snapshot_summary
