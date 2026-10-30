/*
 * Copyright (C) 2024 Carlos Lopez
 * SPDX-License-Identifier: MIT
 */

#define ImTextureID ImU64

#define DEBUG_LEVEL_0

#include <deps/imgui/imgui.h>
#include <include/reshade.hpp>

#include <embed/shaders.h>

#include "../../mods/shader.hpp"
#include "../../mods/swapchain.hpp"
#include "../../utils/settings.hpp"
#include "./shared.h"

namespace {

ShaderInjectData shader_injection;

uint64_t pixel_slot_0_view_handle = 0u;
bool scene_tonemapped = false;

// The desktop mode ends with the scene tonemap: before it the desktop widget is drawn into the in-world monitor texture,
// after it the fullscreen desktop is drawn over the scene.
bool OnTonemapDraw(reshade::api::command_list* cmd_list) {
  scene_tonemapped = true;
  shader_injection.desktop_active = 0.f;
  return true;
}

// The in-game PC desktop (wallpaper, windows and browser) is drawn through the Slate UI shaders. Its wallpaper is a
// wide texture, so once one is bound at slot 0 the rest of the desktop mode is treated as desktop.
bool OnSlateDraw(reshade::api::command_list* cmd_list) {
  if (pixel_slot_0_view_handle == 0u) return true;

  auto* device = cmd_list->get_device();
  const auto resource = device->get_resource_from_view({pixel_slot_0_view_handle});
  if (resource.handle == 0u) return true;

  const auto desc = device->get_resource_desc(resource);
  // Wide static texture only: font atlases and other UI textures are square or smaller, scene buffers are render targets.
  const bool is_render_target = (static_cast<uint32_t>(desc.usage) & static_cast<uint32_t>(reshade::api::resource_usage::render_target)) != 0u;
  if (desc.type == reshade::api::resource_type::texture_2d && !is_render_target
      && desc.texture.width >= 1920u && desc.texture.height >= 1080u
      && (desc.texture.width * 2u) > (desc.texture.height * 3u)) {
    const float desktop_mode = (scene_tonemapped ? 2.f : 1.f);
    static bool logged_in_world_monitor = false;
    static bool logged_fullscreen = false;
    bool* logged = (scene_tonemapped ? &logged_fullscreen : &logged_in_world_monitor);
    if (!*logged) {
      *logged = true;
      reshade::log::message(reshade::log::level::info, (scene_tonemapped ? "WTTG3 desktop detected: fullscreen" : "WTTG3 desktop detected: in-world monitor"));
    }
    shader_injection.desktop_active = desktop_mode;
  }
  return true;
}

void OnPushDescriptors(
    reshade::api::command_list* cmd_list,
    reshade::api::shader_stage stages,
    reshade::api::pipeline_layout layout,
    uint32_t layout_param,
    const reshade::api::descriptor_table_update& update) {
  if (update.type != reshade::api::descriptor_type::texture_shader_resource_view) return;
  if ((static_cast<uint32_t>(stages) & static_cast<uint32_t>(reshade::api::shader_stage::pixel)) == 0u) return;

  for (uint32_t i = 0u; i < update.count; i++) {
    if ((update.binding + i) != 0u) continue;

    pixel_slot_0_view_handle = static_cast<const reshade::api::resource_view*>(update.descriptors)[i].handle;
    return;
  }
}

void OnPresent(
    reshade::api::command_queue* queue,
    reshade::api::swapchain* swapchain,
    const reshade::api::rect* source_rect,
    const reshade::api::rect* dest_rect,
    uint32_t dirty_rect_count,
    const reshade::api::rect* dirty_rects) {
  shader_injection.desktop_active = 0.f;
  scene_tonemapped = false;
}

renodx::mods::shader::CustomShaders custom_shaders = {
    CustomShaderEntryCallback(0xEB865AA4, OnTonemapDraw),
    CustomShaderEntryCallback(0xB09C8D39, OnTonemapDraw),
    CustomShaderEntry(0xBC71921B),
    CustomShaderEntry(0x3B3AC4E0),
    CustomShaderEntryCallback(0x0F400CE2, OnSlateDraw),
    CustomShaderEntryCallback(0x352F2EBC, OnSlateDraw),
    CustomShaderEntryCallback(0x3940EDBD, OnSlateDraw),
    CustomShaderEntryCallback(0x7AF0A12C, OnSlateDraw),
    CustomShaderEntryCallback(0x852E5B6C, OnSlateDraw),
    CustomShaderEntryCallback(0x9FE466FC, OnSlateDraw),
    CustomShaderEntryCallback(0xBA238F31, OnSlateDraw),
    CustomShaderEntryCallback(0xEA333174, OnSlateDraw),
    CustomShaderEntryCallback(0xA2DEBAA3, OnSlateDraw),
    CustomShaderEntryCallback(0xE657CA31, OnSlateDraw),
};

renodx::utils::settings::Settings settings = {
    new renodx::utils::settings::Setting{
        .key = "ToneMapType",
        .binding = &shader_injection.tone_map_type,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .can_reset = true,
        .label = "Tone Mapper",
        .section = "Tone Mapping",
        .tooltip = "Sets the tone mapper type",
        .labels = {"Vanilla", "PsychoV-30"},
        .parse = [](float value) {
          if (value == 1.f) return RENODX_TONE_MAP_TYPE_PSYCHOV30;
          return 0.f; },
    },
    new renodx::utils::settings::Setting{
        .key = "ToneMapPeakNits",
        .binding = &shader_injection.peak_white_nits,
        .default_value = 1500.f,
        .can_reset = false,
        .label = "Peak Brightness",
        .section = "Tone Mapping",
        .tooltip = "Sets the value of peak white in nits",
        .min = 48.f,
        .max = 4000.f,
    },
    new renodx::utils::settings::Setting{
        .key = "ToneMapGameNits",
        .binding = &shader_injection.diffuse_white_nits,
        .default_value = 203.f,
        .label = "Game Brightness",
        .section = "Tone Mapping",
        .tooltip = "Sets the value of 100% white in nits",
        .min = 48.f,
        .max = 500.f,
    },
    new renodx::utils::settings::Setting{
        .key = "ToneMapUINits",
        .binding = &shader_injection.graphics_white_nits,
        .default_value = 203.f,
        .label = "UI Brightness",
        .section = "Tone Mapping",
        .tooltip = "Sets the brightness of UI and HUD elements in nits",
        .min = 1.f,
        .max = 500.f,
    },
    new renodx::utils::settings::Setting{
        .key = "ToneMapDesktopNits",
        .binding = &shader_injection.desktop_white_nits,
        .default_value = 203.f,
        .label = "Desktop Brightness",
        .section = "Tone Mapping",
        .tooltip = "Sets the brightness of the PC desktop and browser in nits",
        .min = 1.f,
        .max = 500.f,
    },
    new renodx::utils::settings::Setting{
        .key = "GammaCorrection",
        .binding = &shader_injection.gamma_correction,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .label = "Gamma Correction",
        .section = "Tone Mapping",
        .tooltip = "Emulates a display EOTF.",
        .labels = {"Off", "2.2", "BT.1886"},
    },
    new renodx::utils::settings::Setting{
        .key = "PsychoVConeResponse",
        .binding = &shader_injection.psychov_cone_response,
        .default_value = 50.f,
        .label = "Cone Response",
        .section = "Tone Mapping",
        .tooltip = "PsychoV cone response. Higher is more contrast, 50 is neutral.",
        .min = 0.f,
        .max = 100.f,
        .is_enabled = []() { return shader_injection.tone_map_type == RENODX_TONE_MAP_TYPE_PSYCHOV30; },
        .parse = [](float value) { return value * 0.02f; },
    },
    new renodx::utils::settings::Setting{
        .key = "PsychoVGamutHull",
        .binding = &shader_injection.psychov_gamut_mode,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .label = "Gamut Hull",
        .section = "Tone Mapping",
        .tooltip = "Gamut PsychoV-30 compresses colors into. BT.709 keeps the game's colors, BT.2020 allows wider colors.",
        .labels = {"BT.709", "BT.2020"},
        .is_enabled = []() { return shader_injection.tone_map_type == RENODX_TONE_MAP_TYPE_PSYCHOV30; },
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeExposure",
        .binding = &shader_injection.tone_map_exposure,
        .default_value = 1.f,
        .label = "Exposure",
        .section = "Color Grading",
        .max = 2.f,
        .format = "%.2f",
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeHighlights",
        .binding = &shader_injection.tone_map_highlights,
        .default_value = 50.f,
        .label = "Highlights",
        .section = "Color Grading",
        .max = 100.f,
        .parse = [](float value) { return value * 0.02f; },
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeShadows",
        .binding = &shader_injection.tone_map_shadows,
        .default_value = 50.f,
        .label = "Shadows",
        .section = "Color Grading",
        .max = 100.f,
        .parse = [](float value) { return value * 0.02f; },
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeContrast",
        .binding = &shader_injection.tone_map_contrast,
        .default_value = 50.f,
        .label = "Contrast",
        .section = "Color Grading",
        .max = 100.f,
        .parse = [](float value) { return value * 0.02f; },
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeSaturation",
        .binding = &shader_injection.tone_map_saturation,
        .default_value = 50.f,
        .label = "Saturation",
        .section = "Color Grading",
        .max = 100.f,
        .parse = [](float value) { return value * 0.02f; },
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeBlowout",
        .binding = &shader_injection.tone_map_blowout,
        .default_value = 0.f,
        .label = "Blowout",
        .section = "Color Grading",
        .tooltip = "Controls highlight desaturation due to overexposure.",
        .max = 100.f,
        .parse = [](float value) { return value * 0.01f; },
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeScene",
        .binding = &shader_injection.color_grade_strength,
        .default_value = 100.f,
        .label = "Scene Grading",
        .section = "Color Grading",
        .tooltip = "Scene grading as applied by the game",
        .max = 100.f,
        .is_enabled = []() { return shader_injection.tone_map_type > 0; },
        .parse = [](float value) { return value * 0.01f; },
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
        },
    },
};

void OnPresetOff() {
  renodx::utils::settings::UpdateSettings({
      {"ToneMapType", 0.f},
      {"ToneMapPeakNits", 203.f},
      {"ToneMapGameNits", 203.f},
      {"ToneMapUINits", 203.f},
      {"ToneMapDesktopNits", 203.f},
      {"GammaCorrection", 0.f},
      {"PsychoVConeResponse", 50.f},
      {"PsychoVGamutHull", 1.f},
      {"ColorGradeExposure", 1.f},
      {"ColorGradeHighlights", 50.f},
      {"ColorGradeShadows", 50.f},
      {"ColorGradeContrast", 50.f},
      {"ColorGradeSaturation", 50.f},
      {"ColorGradeBlowout", 0.f},
      {"ColorGradeScene", 100.f},
  });
}

bool fired_on_init_swapchain = false;

void OnInitSwapchain(reshade::api::swapchain* swapchain, bool resize) {
  if (fired_on_init_swapchain) return;
  fired_on_init_swapchain = true;
  auto peak = renodx::utils::swapchain::GetPeakNits(swapchain);
  if (peak.has_value()) {
    settings[1]->default_value = peak.value();
    settings[1]->can_reset = true;
  }
}

bool initialized = false;

}  // namespace

extern "C" __declspec(dllexport) constexpr const char* NAME = "RenoDX";
extern "C" __declspec(dllexport) constexpr const char* DESCRIPTION = "RenoDX for Welcome to the Game III";

BOOL APIENTRY DllMain(HMODULE h_module, DWORD fdw_reason, LPVOID lpv_reserved) {
  switch (fdw_reason) {
    case DLL_PROCESS_ATTACH:
      if (!reshade::register_addon(h_module)) return FALSE;

      if (!initialized) {
        renodx::mods::shader::force_pipeline_cloning = true;
        renodx::mods::shader::expected_constant_buffer_space = 50;
        renodx::mods::shader::expected_constant_buffer_index = 13;
        renodx::mods::shader::allow_multiple_push_constants = true;

        renodx::mods::swapchain::expected_constant_buffer_index = 13;
        renodx::mods::swapchain::expected_constant_buffer_space = 50;
        renodx::mods::swapchain::use_resource_cloning = true;
        renodx::mods::swapchain::swap_chain_proxy_shaders = {
            {
                reshade::api::device_api::d3d11,
                {
                    .vertex_shader = __swap_chain_proxy_vertex_shader_dx11,
                    .pixel_shader = __swap_chain_proxy_pixel_shader_dx11,
                },
            },
        };

        // HDR10 output needs a borderless window
        renodx::mods::swapchain::force_borderless = true;
        renodx::mods::swapchain::prevent_full_screen = true;
        renodx::mods::swapchain::SetUseHDR10(true);
        renodx::mods::swapchain::use_resize_buffer = false;
        // Tonemap output and swapchain are R10G10B10A2; the upgrade keeps values above 1.0 through the vignette pass and UI blend
        renodx::mods::swapchain::swap_chain_upgrade_targets.push_back({
            .old_format = reshade::api::format::r10g10b10a2_unorm,
            .new_format = reshade::api::format::r16g16b16a16_float,
            .use_resource_view_cloning = true,
            .aspect_ratio = static_cast<float>(renodx::mods::swapchain::SwapChainUpgradeTarget::ANY),
            .usage_include = reshade::api::resource_usage::render_target,
        });
        // Bloom and other R11G11B10 intermediates: float16 removes the 5/6-bit mantissa banding and allows negative channels
        renodx::mods::swapchain::swap_chain_upgrade_targets.push_back({
            .old_format = reshade::api::format::r11g11b10_float,
            .new_format = reshade::api::format::r16g16b16a16_float,
            .use_resource_view_cloning = true,
            .aspect_ratio = renodx::mods::swapchain::SwapChainUpgradeTarget::BACK_BUFFER,
            .usage_include = reshade::api::resource_usage::render_target | reshade::api::resource_usage::unordered_access,
        });

        // Values for settings that are not exposed in the GUI
        shader_injection.swap_chain_clamp_color_space = 1.f;
        shader_injection.swap_chain_encoding = 4.f;
        shader_injection.swap_chain_encoding_color_space = 1.f;
        // Fixed BT.2020 hull at full strength
        shader_injection.psychov_gamut_compression = 1.f;
        shader_injection.psychov_compression = 1.f;
        shader_injection.psychov_cone_response = 1.f;
        // Fixed BT.2020 source gamut (PSYCHO30_SOURCE_BOUNDARY_BT2020)
        shader_injection.psychov_source_boundary = 2.f;

        initialized = true;
      }

      reshade::register_event<reshade::addon_event::init_swapchain>(OnInitSwapchain);  // detect peak nits
      reshade::register_event<reshade::addon_event::push_descriptors>(OnPushDescriptors);
      reshade::register_event<reshade::addon_event::present>(OnPresent);
      break;
    case DLL_PROCESS_DETACH:
      reshade::unregister_event<reshade::addon_event::init_swapchain>(OnInitSwapchain);
      reshade::unregister_event<reshade::addon_event::push_descriptors>(OnPushDescriptors);
      reshade::unregister_event<reshade::addon_event::present>(OnPresent);
      reshade::unregister_addon(h_module);
      break;
  }

  renodx::utils::settings::Use(fdw_reason, &settings, &OnPresetOff);
  renodx::mods::swapchain::Use(fdw_reason, &shader_injection);
  renodx::mods::shader::Use(fdw_reason, custom_shaders, &shader_injection);

  return TRUE;
}
