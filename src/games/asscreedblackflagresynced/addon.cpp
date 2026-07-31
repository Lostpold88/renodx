/*
 * Copyright (C) 2026 Musa Haji
 * SPDX-License-Identifier: MIT
 */

#define ImTextureID ImU64

#define DEBUG_LEVEL_0

#include <atomic>

#include <deps/imgui/imgui.h>
#include <include/reshade.hpp>

#include <embed/shaders.h>

#include "../../mods/shader.hpp"
#include "../../utils/date.hpp"
#include "../../utils/settings.hpp"
#include "shared.h"

namespace {

// Reference diffuse white the purity controls are calibrated against. The
// game's own paper white follows its exposure setting, which is not bound in
// the grading LUT, so this stays fixed rather than tracking the UI slider.
constexpr float kReferenceDiffuseWhiteNits = 203.f;
constexpr float kFallbackPeakNits = 1000.f;

// Cone response is the log-log slope of the tone curve at mid gray. The slider
// is offset so that its midpoint lands on the slope of the Vanilla+ curve,
// which is BLACKFLAG_IMMORTALS_ANCHOR_SLOPE in common.hlsli:
//   1.5 * (0.05 + (0.18 - 0.05) / 1.5) / 0.18
// Each slider step stays worth 0.02, matching every other grading slider here.
constexpr float kConeResponseStep = 0.02f;
constexpr float kConeResponseNeutral = 1.13888889f;
constexpr float kConeResponseOffset = kConeResponseNeutral - (50.f * kConeResponseStep);

ShaderInjectData shader_injection;

std::atomic_bool tone_map_lut_invalidated = false;
std::atomic_bool ui_lut_invalidated = false;
float applied_tone_map_type = 2.f;
float applied_cone_response = kConeResponseNeutral;
float applied_ui_nits = 203.f;

void SetToneMapLutInvalidated(bool invalidated) {
  tone_map_lut_invalidated.store(invalidated, std::memory_order_relaxed);
}

void SetUiLutInvalidated(bool invalidated) {
  ui_lut_invalidated.store(invalidated, std::memory_order_relaxed);
}

bool ToneMapLutValuesDirty() {
  return shader_injection.tone_map_type != applied_tone_map_type
         || shader_injection.tone_map_cone_response != applied_cone_response;
}

bool UiLutValuesDirty() {
  return shader_injection.graphics_white_nits != applied_ui_nits;
}

void RefreshToneMapLutDirtyState() {
  SetToneMapLutInvalidated(ToneMapLutValuesDirty());
}

void RefreshUiLutDirtyState() {
  SetUiLutInvalidated(UiLutValuesDirty());
}

void MarkToneMapLutApplied() {
  applied_tone_map_type = shader_injection.tone_map_type;
  applied_cone_response = shader_injection.tone_map_cone_response;
  RefreshToneMapLutDirtyState();
}

void MarkUiLutApplied() {
  applied_ui_nits = shader_injection.graphics_white_nits;
  RefreshUiLutDirtyState();
}

void InitializeAppliedValues() {
  applied_tone_map_type = shader_injection.tone_map_type;
  applied_cone_response = shader_injection.tone_map_cone_response;
  applied_ui_nits = shader_injection.graphics_white_nits;
}

void OnToneMapLutBuilderDrawn(reshade::api::command_list* /*cmd_list*/) {
  MarkToneMapLutApplied();
}

void OnUiLutBuilderDrawn(reshade::api::command_list* /*cmd_list*/) {
  MarkUiLutApplied();
}

void OnToneMapLutControlledSettingChanged(float /*previous*/, float /*current*/) {
  RefreshToneMapLutDirtyState();
}

void OnUiNitsSettingChanged(float /*previous*/, float /*current*/) {
  RefreshUiLutDirtyState();
}

void OnPresetChangedInvalidateIfChanged() {
  RefreshToneMapLutDirtyState();
  RefreshUiLutDirtyState();
}

bool IsToneMapped() {
  return shader_injection.tone_map_type != 0.f;
}

bool IsPsychoV22() {
  return shader_injection.tone_map_type == 2.f;
}

renodx::mods::shader::CustomShaders custom_shaders = {
    {0x3BABF259, {
                     .crc32 = 0x3BABF259,
                     .code = __0x3BABF259,
                     .on_drawn = &OnToneMapLutBuilderDrawn,
                 }},  // ACES ToneMap LutBuilder
    {0x19D9391A, {
                     .crc32 = 0x19D9391A,
                     .code = __0x19D9391A,
                     .on_drawn = &OnToneMapLutBuilderDrawn,
                 }},  // SDR AC ToneMap LutBuilder
    {0x964A2F2C, {
                     .crc32 = 0x964A2F2C,
                     .code = __0x964A2F2C,
                     .on_drawn = &OnToneMapLutBuilderDrawn,
                 }},  // HDR AC ToneMap LutBuilder
    {0xBF0CF1AA, {
                     .crc32 = 0xBF0CF1AA,
                     .code = __0xBF0CF1AA,
                     .on_drawn = &OnUiLutBuilderDrawn,
                 }},  // UI - sRGB to HDR
    __ALL_CUSTOM_SHADERS};

renodx::utils::settings::Settings settings = {
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::TEXT,
        .label = std::string("Requires HDR enabled in the game's display options.\n"
                             "Overall brightness and peak brightness stay on the in-game\n"
                             "exposure and peak sliders."),
        .section = "Tone Mapping",
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::TEXT,
        .label = "Toggle the in-game HDR setting or restart the game to apply the changed tone mapper.",
        .section = "Tone Mapping",
        .tint = 0xFF0000,
        .is_visible = []() { return tone_map_lut_invalidated.load(std::memory_order_relaxed); },
        .is_sticky = false,
    },
    new renodx::utils::settings::Setting{
        .key = "ToneMapType",
        .binding = &shader_injection.tone_map_type,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 2.f,
        .label = "Tone Mapper",
        .section = "Tone Mapping",
        .tooltip = "Vanilla+ keeps the game's own curve and only fixes its gamut handling."
                   "\nPsychoV-22 replaces the curve outright with a cone response model:"
                   "\nopener shadows, an earlier and softer highlight roll-off."
                   "\n\nNeeds an in-game HDR toggle or a restart to take effect.",
        .labels = {"Vanilla", "RenoDX (Vanilla+)", "PsychoV-22"},
        .on_change_value = &OnToneMapLutControlledSettingChanged,
    },
    new renodx::utils::settings::Setting{
        .key = "ToneMapConeResponse",
        .binding = &shader_injection.tone_map_cone_response,
        .default_value = 50.f,
        .label = "Cone Response",
        .section = "Tone Mapping",
        .tooltip = "Contrast and purity of the PsychoV-22 curve, driven together."
                   "\nWith every other control neutral this is exactly the log-log slope of the"
                   "\ncurve at mid gray. 50 is the slope of the Vanilla+ curve, so the two tone"
                   "\nmappers line up at mid gray by default."
                   "\n\nNeeds an in-game HDR toggle or a restart to take effect.",
        .max = 100.f,
        .is_enabled = []() { return IsPsychoV22(); },
        .parse = [](float value) { return (value * kConeResponseStep) + kConeResponseOffset; },
        .on_change_value = &OnToneMapLutControlledSettingChanged,
    },
    new renodx::utils::settings::Setting{
        .key = "Exposure",
        .binding = &shader_injection.tone_map_exposure,
        .default_value = 1.f,
        .label = "Exposure",
        .section = "Color Grading",
        .tooltip = "Linear scene exposure, applied ahead of the tone mapper.",
        .max = 2.f,
        .format = "%.2f",
        .is_enabled = []() { return IsToneMapped(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeShadows",
        .binding = &shader_injection.tone_map_shadows,
        .default_value = 50.f,
        .label = "Shadows",
        .section = "Color Grading",
        .tooltip = "Lifts or crushes the darker half of the image.",
        .max = 100.f,
        .is_enabled = []() { return IsToneMapped(); },
        .parse = [](float value) { return value * 0.02f; },
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeHighlights",
        .binding = &shader_injection.tone_map_highlights,
        .default_value = 50.f,
        .label = "Highlights",
        .section = "Color Grading",
        .tooltip = "Lifts or lowers the brighter half of the image.",
        .max = 100.f,
        .is_enabled = []() { return IsToneMapped(); },
        .parse = [](float value) { return value * 0.02f; },
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeContrast",
        .binding = &shader_injection.tone_map_contrast,
        .default_value = 50.f,
        .label = "Contrast",
        .section = "Color Grading",
        .tooltip = "Steepens or flattens the image around mid gray.",
        .max = 100.f,
        .is_enabled = []() { return IsToneMapped(); },
        .parse = [](float value) { return value * 0.02f; },
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeFlare",
        .binding = &shader_injection.tone_map_flare,
        .default_value = 0.f,
        .label = "Flare",
        .section = "Color Grading",
        .tooltip = "Flare/glare compensation. Raises the black floor the way a lens would.",
        .max = 100.f,
        .is_enabled = []() { return IsToneMapped(); },
        .parse = [](float value) { return value * 0.01f; },
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeSaturation",
        .binding = &shader_injection.tone_map_saturation,
        .default_value = 50.f,
        .label = "Saturation",
        .section = "Color Grading",
        .tooltip = "Overall colour purity.",
        .max = 100.f,
        .is_enabled = []() { return IsToneMapped(); },
        .parse = [](float value) { return value * 0.02f; },
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeHighlightSaturation",
        .binding = &shader_injection.tone_map_highlight_saturation,
        .default_value = 50.f,
        .label = "Highlight Saturation",
        .section = "Color Grading",
        .tooltip = "Colour purity of the brightest parts of the image, independent of the overall"
                   "\nsaturation above.",
        .max = 100.f,
        .is_enabled = []() { return IsToneMapped(); },
        .parse = [](float value) { return value * 0.02f; },
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeDechroma",
        .binding = &shader_injection.tone_map_dechroma,
        .default_value = 0.f,
        .label = "Dechroma",
        .section = "Color Grading",
        .tooltip = "Bleaches colour towards white as it approaches peak brightness, the way film"
                   "\nand the eye both do under overexposure.",
        .max = 100.f,
        .is_enabled = []() { return IsToneMapped(); },
        .parse = [](float value) { return value * 0.01f; },
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeCoolness",
        .binding = &shader_injection.tone_map_coolness,
        .default_value = 0.f,
        .label = "Coolness",
        .section = "Color Grading",
        .tooltip = "Shifts the white point towards D93. Takes the warm cast off the Caribbean"
                   "\ndaylight without touching saturation.",
        .max = 100.f,
        .is_enabled = []() { return IsToneMapped(); },
        .parse = [](float value) { return value * 0.01f; },
    },
    new renodx::utils::settings::Setting{
        .key = "ColorFilterStrength",
        .binding = &shader_injection.custom_color_filter_strength,
        .default_value = 100.f,
        .label = "Color Filter Strength",
        .section = "Color Grading",
        .tooltip = "How much of the game's own per-scene colour filter is kept. 0 removes it.",
        .max = 100.f,
        .is_enabled = []() { return IsToneMapped(); },
        .parse = [](float value) { return value * 0.01f; },
    },
    new renodx::utils::settings::Setting{
        .key = "FxBloom",
        .binding = &shader_injection.custom_bloom,
        .default_value = 100.f,
        .label = "Bloom",
        .section = "Effects",
        .tooltip = "Amount of bloom. 100 is the game's own amount.",
        .max = 100.f,
        .parse = [](float value) { return value * 0.01f; },
    },
    new renodx::utils::settings::Setting{
        .key = "FxBloomScaling",
        .binding = &shader_injection.custom_bloom_scaling,
        .default_value = 0.f,
        .label = "Bloom Scaling",
        .section = "Effects",
        .tooltip = "Fades bloom out over dark scene content instead of letting it sit on top as a"
                   "\nflat haze. Cleans up night scenes.",
        .max = 100.f,
        .parse = [](float value) { return value * 0.01f; },
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::TEXT,
        .label = "Restart the game to apply the changed UI brightness.",
        .section = "UI",
        .tint = 0xFF0000,
        .is_visible = []() { return ui_lut_invalidated.load(std::memory_order_relaxed); },
        .is_sticky = false,
    },
    new renodx::utils::settings::Setting{
        .key = "ToneMapUINits",
        .binding = &shader_injection.graphics_white_nits,
        .default_value = 203.f,
        .label = "UI Brightness",
        .section = "UI",
        .tooltip = "Brightness of UI and HUD elements in nits."
                   "\n\nNeeds a game restart to take effect.",
        .min = 48.f,
        .max = 500.f,
        .is_enabled = []() { return IsToneMapped(); },
        .on_change_value = &OnUiNitsSettingChanged,
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::BUTTON,
        .label = "Reset All",
        .section = "Options",
        .on_change = []() {
          for (auto* setting : settings) {
            if (setting->key.empty()) continue;
            if (!setting->can_reset) continue;
            renodx::utils::settings::UpdateSetting(setting->key, setting->default_value);
          }
          RefreshToneMapLutDirtyState();
          RefreshUiLutDirtyState();
        },
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::BUTTON,
        .label = "RenoDX Discord",
        .section = "Links",
        .group = "button-line-2",
        .tint = 0x5865F2,
        .on_change = []() {
          renodx::utils::platform::LaunchURL("https://discord.gg/", "t9v7wx9NTD");
        },
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::BUTTON,
        .label = "HDR Den Discord",
        .section = "Links",
        .group = "button-line-2",
        .tint = 0x5865F2,
        .on_change = []() {
          renodx::utils::platform::LaunchURL("https://discord.gg/", "a7HECzaPG7");
        },
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::BUTTON,
        .label = "More Mods",
        .section = "Links",
        .group = "button-line-2",
        .tint = 0x2B3137,
        .on_change = []() {
          renodx::utils::platform::LaunchURL("https://github.com/clshortfuse/renodx/wiki/Mods");
        },
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::BUTTON,
        .label = "Github",
        .section = "Links",
        .group = "button-line-2",
        .tint = 0x2B3137,
        .on_change = []() {
          renodx::utils::platform::LaunchURL("https://github.com/clshortfuse/renodx");
        },
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::BUTTON,
        .label = "Musa's Ko-Fi",
        .section = "Links",
        .group = "button-line-3",
        .tint = 0xFF5A16,
        .on_change = []() { renodx::utils::platform::LaunchURL("https://ko-fi.com/musaqh"); },
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::BUTTON,
        .label = "ShortFuse's Ko-Fi",
        .section = "Links",
        .group = "button-line-3",
        .tint = 0xFF5A16,
        .on_change = []() { renodx::utils::platform::LaunchURL("https://ko-fi.com/shortfuse"); },
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::TEXT,
        .label = std::string("Build: ") + renodx::utils::date::ISO_DATE_TIME,
        .section = "About",
    },
};

void OnPresetOff() {
  renodx::utils::settings::UpdateSettings({
      {"ToneMapType", 0.f},
      {"ToneMapConeResponse", 50.f},
      {"ToneMapUINits", 203.f},
      {"Exposure", 1.f},
      {"ColorGradeHighlights", 50.f},
      {"ColorGradeShadows", 50.f},
      {"ColorGradeContrast", 50.f},
      {"ColorGradeSaturation", 50.f},
      {"ColorGradeHighlightSaturation", 50.f},
      {"ColorGradeCoolness", 0.f},
      {"ColorGradeDechroma", 0.f},
      {"ColorGradeFlare", 0.f},
      {"ColorFilterStrength", 100.f},
      {"FxBloom", 100.f},
      {"FxBloomScaling", 0.f},
  });

  RefreshToneMapLutDirtyState();
  RefreshUiLutDirtyState();
}

bool fired_on_init_swapchain = false;

// The purity controls run in the scene-referred grading LUT, where the game's
// peak setting is not bound. The display's own peak is the closest reference
// available to the addon, so it is what they anchor against.
void OnInitSwapchain(reshade::api::swapchain* swapchain, bool /*resize*/) {
  if (fired_on_init_swapchain) return;
  fired_on_init_swapchain = true;
  auto peak = renodx::utils::swapchain::GetPeakNits(swapchain);
  shader_injection.custom_peak_ratio =
      peak.value_or(kFallbackPeakNits) / kReferenceDiffuseWhiteNits;
}

bool initialized = false;

}  // namespace

extern "C" __declspec(dllexport) constexpr const char* NAME = "RenoDX";
extern "C" __declspec(dllexport) constexpr const char* DESCRIPTION = "RenoDX for Assassin's Creed Black Flag Resynced";

BOOL APIENTRY DllMain(HMODULE h_module, DWORD fdw_reason, LPVOID lpv_reserved) {
  switch (fdw_reason) {
    case DLL_PROCESS_ATTACH:
      if (!reshade::register_addon(h_module)) return FALSE;

      if (!initialized) {
        renodx::mods::shader::expected_constant_buffer_space = 50;
        renodx::mods::shader::expected_constant_buffer_index = 13;
        renodx::mods::shader::force_pipeline_cloning = true;
        renodx::utils::settings::on_preset_changed_callbacks.emplace_back(&OnPresetChangedInvalidateIfChanged);
        shader_injection.custom_peak_ratio = kFallbackPeakNits / kReferenceDiffuseWhiteNits;
        initialized = true;
      }

      reshade::register_event<reshade::addon_event::init_swapchain>(OnInitSwapchain);  // detect peak nits
      break;
    case DLL_PROCESS_DETACH:
      reshade::unregister_event<reshade::addon_event::init_swapchain>(OnInitSwapchain);  // detect peak nits
      reshade::unregister_addon(h_module);
      break;
  }

  renodx::utils::settings::Use(fdw_reason, &settings, &OnPresetOff);
  if (fdw_reason == DLL_PROCESS_ATTACH) {
    InitializeAppliedValues();
    RefreshToneMapLutDirtyState();
    RefreshUiLutDirtyState();
  }
  renodx::mods::shader::Use(fdw_reason, custom_shaders, &shader_injection);

  return TRUE;
}
