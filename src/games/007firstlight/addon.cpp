/*
 * Copyright (C) 2026 Musa Haji
 * Copyright (C) 2026 Lazorr
 * SPDX-License-Identifier: MIT
 */

#define ImTextureID ImU64

#define DEBUG_LEVEL_0

#include <deps/imgui/imgui.h>
#include <include/reshade.hpp>

#include <embed/shaders.h>

#include "../../mods/shader.hpp"
#include "../../utils/date.hpp"
#include "../../utils/settings.hpp"
#include "dlss.hpp"
#include "isfast_noise.hpp"
#include "shared.h"

namespace {

ShaderInjectData shader_injection;

renodx::mods::shader::CustomShaders custom_shaders = {__ALL_CUSTOM_SHADERS};

// Jeder Wert != 0 schaltet die komplette RenoDX-Kette ein; der Wert selbst
// waehlt nur die Tonwertkurve.
bool IsToneMapped() { return shader_injection.tone_map_type != 0.f; }

bool IsPsychoV22() { return shader_injection.tone_map_type == 2.f; }

renodx::utils::settings::Settings settings = {
    new renodx::utils::settings::Setting{
        .key = "ToneMapType",
        .binding = &shader_injection.tone_map_type,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .label = "Tonemapper",
        .section = "Tonemapping",
        .tooltip = "Legt den Typ des Tonemappers fest",
        .labels = {"Vanilla", "RenoDX", "PsychoV-22"},
    },
    new renodx::utils::settings::Setting{
        .key = "ToneMapPeakNits",
        .binding = &shader_injection.peak_white_nits,
        .default_value = 1000.f,
        .label = "Spitzenhelligkeit",
        .section = "Tonemapping",
        .tooltip = "Legt den Wert für Spitzenweiß in Nits fest",
        .min = 48.f,
        .max = 4000.f,
        .is_enabled = []() { return IsToneMapped(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ToneMapGameNits",
        .binding = &shader_injection.diffuse_white_nits,
        .default_value = 203.f,
        .label = "Spielhelligkeit",
        .section = "Tonemapping",
        .tooltip = "Legt den Wert für 100 % Weiß in Nits fest",
        .min = 48.f,
        .max = 500.f,
        .is_enabled = []() { return IsToneMapped(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ToneMapUINits",
        .binding = &shader_injection.graphics_white_nits,
        .default_value = 203.f,
        .label = "UI-Helligkeit",
        .section = "Tonemapping",
        .tooltip = "Legt die Helligkeit von UI- und HUD-Elementen in Nits fest",
        .min = 48.f,
        .max = 500.f,
        .is_enabled = []() { return IsToneMapped(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ToneMapScaling",
        .binding = &shader_injection.tone_map_scaling,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Skalierung",
        .section = "Tonemapping",
        .tooltip = "Luminanz skaliert Farben gleichmäßig, Pro Kanal entspricht dem"
                   "\nursprünglichen Verhalten des Tonemappers."
                   "\nBei PsychoV-22 wirkt dies nur noch auf die Gammakorrektur,"
                   "\nnicht auf die Tonwertkurve.",
        .labels = {"Luminanz", "Pro Kanal"},
        .is_enabled = []() { return IsToneMapped(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ToneMapConeResponse",
        .binding = &shader_injection.tone_map_cone_response,
        .default_value = 60.f,
        .label = "Zapfenantwort",
        .section = "Tonemapping",
        .tooltip = "Steuert die Zapfenantwort-Formung von PsychoV-22."
                   "\nSkaliert Kontrast und Farbreinheit der Tonwertkurve gemeinsam."
                   "\nNur mit PsychoV-22 verfügbar.",
        .max = 100.f,
        .is_enabled = []() { return IsPsychoV22(); },
        .parse = [](float value) { return value * 0.02f; },
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeExposure",
        .binding = &shader_injection.tone_map_exposure,
        .default_value = 1.f,
        .label = "Belichtung",
        .section = "Farbkorrektur",
        .max = 2.f,
        .format = "%.2f",
        .is_enabled = []() { return IsToneMapped(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeGamma",
        .binding = &shader_injection.tone_map_gamma,
        .default_value = 1.f,
        .label = "Gamma",
        .section = "Farbkorrektur",
        .min = 0.75f,
        .max = 1.25f,
        .format = "%.2f",
        .is_enabled = []() { return IsToneMapped(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeHighlights",
        .binding = &shader_injection.tone_map_highlights,
        .default_value = 50.f,
        .label = "Lichter",
        .section = "Farbkorrektur",
        .max = 100.f,
        .is_enabled = []() { return IsToneMapped(); },
        .parse = [](float value) { return value * 0.02f; },
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeHighlightContrast",
        .binding = &shader_injection.tone_map_contrast_highlights,
        .default_value = 50.f,
        .label = "Lichterkontrast",
        .section = "Farbkorrektur",
        .max = 100.f,
        .is_enabled = []() { return IsToneMapped(); },
        .parse = [](float value) { return value * 0.02f; },
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeShadows",
        .binding = &shader_injection.tone_map_shadows,
        .default_value = 50.f,
        .label = "Schatten",
        .section = "Farbkorrektur",
        .max = 100.f,
        .is_enabled = []() { return IsToneMapped(); },
        .parse = [](float value) { return value * 0.02f; },
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeShadowContrast",
        .binding = &shader_injection.tone_map_contrast_shadows,
        .default_value = 50.f,
        .label = "Schattenkontrast",
        .section = "Farbkorrektur",
        .max = 100.f,
        .is_enabled = []() { return IsToneMapped(); },
        .parse = [](float value) { return value * 0.02f; },
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeContrast",
        .binding = &shader_injection.tone_map_contrast,
        .default_value = 50.f,
        .label = "Kontrast",
        .section = "Farbkorrektur",
        .max = 100.f,
        .is_enabled = []() { return IsToneMapped(); },
        .parse = [](float value) { return value * 0.02f; },
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeSaturation",
        .binding = &shader_injection.tone_map_saturation,
        .default_value = 50.f,
        .label = "Sättigung",
        .section = "Farbkorrektur",
        .max = 100.f,
        .is_enabled = []() { return IsToneMapped(); },
        .parse = [](float value) { return value * 0.02f; },
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeHighlightSaturation",
        .binding = &shader_injection.tone_map_highlight_saturation,
        .default_value = 50.f,
        .label = "Lichtersättigung",
        .section = "Farbkorrektur",
        .tooltip = "Fügt den Lichtern Farbe hinzu oder entzieht sie.",
        .max = 100.f,
        .is_enabled = []() { return IsToneMapped(); },
        .parse = [](float value) { return value * 0.02f; },
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeDechroma",
        .binding = &shader_injection.tone_map_dechroma,
        .default_value = 0.f,
        .label = "Entsättigung",
        .section = "Farbkorrektur",
        .tooltip = "Steuert die Entsättigung der Lichter durch Überbelichtung.",
        .max = 100.f,
        .is_enabled = []() { return IsToneMapped(); },
        .parse = [](float value) { return value * 0.01f; },
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeFlare",
        .binding = &shader_injection.tone_map_flare,
        .default_value = 0.f,
        .label = "Streulicht",
        .section = "Farbkorrektur",
        .tooltip = "Kompensation von Streu- und Blendlicht",
        .max = 100.f,
        .is_enabled = []() { return IsToneMapped(); },
        .parse = [](float value) { return value * 0.01f; },
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeLUTStrength",
        .binding = &shader_injection.color_grade_lut_strength,
        .default_value = 100.f,
        .label = "LUT-Stärke",
        .section = "Farbkorrektur",
        .tooltip = "Stärke des spieleigenen Farbkorrektur-LUT."
                   "\nPsychoV-22 umgeht den LUT vollständig, damit er dessen"
                   "\nFarbton- und Reinheitsentscheidungen nicht überschreibt"
                   "\nund nichts außerhalb BT.709 abgeschnitten wird.",
        .max = 100.f,
        .is_enabled = []() { return IsToneMapped() && !IsPsychoV22(); },
        .parse = [](float value) { return value * 0.01f; },
    },
    new renodx::utils::settings::Setting{
        .key = "FxBloom",
        .binding = &shader_injection.custom_bloom,
        .default_value = 100.f,
        .label = "Bloom",
        .section = "Effekte",
        .max = 100.f,
        .parse = [](float value) { return value * 0.01f; },
    },
    new renodx::utils::settings::Setting{
        .key = "FxFilmGrainType",
        .binding = &shader_injection.custom_film_grain_type,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 2.f,
        .label = "Filmkorn-Typ",
        .section = "Effekte",
        .labels = {"Vanilla (Fehlerhaft)", "Vanilla (Korrigiert)", "Perzeptuell"},
    },
    new renodx::utils::settings::Setting{
        .key = "FxGrainStrength",
        .binding = &shader_injection.custom_grain_strength,
        .default_value = 50.f,
        .label = "Filmkorn",
        .section = "Effekte",
        .max = 100.f,
        .parse = [](float value) { return value * 0.02f; },
    },
    new renodx::utils::settings::Setting{
        .key = "FxISFASTShadows",
        .binding = &shader_injection.custom_isfast_shadows,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "IS-FAST-Schattenrauschen",
        .section = "Schatten",
        .tooltip = "Nutzt IS-FAST-Rauschen für die Rotation der Schattenabtastung im Deferred-Pass"
                   "\nund reduziert so vertikale Jitter-Artefakte.",
        .labels = {"Aus", "Ein"},
    },
    new renodx::utils::settings::Setting{
        .key = "FxSSRReflectionFix",
        .binding = &shader_injection.custom_ssr_reflection_fix,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .label = "SSR-Pixelfehler-Korrektur",
        .section = "Reflexionen",
        .tooltip = "Aus nutzt die ursprüngliche lineare SSR-Farbabtastung."
                   "\nScharf ist die geprüfte Korrektur per Point Load."
                   "\nGefiltert ist eine zurückhaltende 5x5-Weichzeichnung, die den"
                   "\nPoint-Load-Abdruck der SSR erhält.",
        .labels = {"Aus", "Scharf", "Gefiltert"},
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::BUTTON,
        .label = "Alles zurücksetzen",
        .section = "Optionen",
        .group = "button-line-0",
        .on_change = []() {
          for (auto* setting : settings) {
            if (setting->key.empty()) continue;
            if (!setting->can_reset) continue;
            renodx::utils::settings::UpdateSetting(setting->key, setting->default_value);
          }
        },
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::BUTTON,
        .label = "Purist",
        .section = "Optionen",
        .group = "button-line-0",
        .tooltip = "Bleibt näher an der ursprünglichen künstlerischen Absicht.",
        .on_change = []() {
          renodx::utils::settings::ResetSettings();
          renodx::utils::settings::UpdateSettings({
              {"FxFilmGrainType", 1.f},
              {"ToneMapScaling", 1.f},
          });
        },
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::BUTTON,
        .label = "Discord",
        .section = "Links",
        .group = "button-line-1",
        .tint = 0x5865F2,
        .on_change = []() {
          renodx::utils::platform::LaunchURL("https://discord.gg/", "t9v7wx9NTD");
        },
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::BUTTON,
        .label = "Weitere Mods",
        .section = "Links",
        .group = "button-line-1",
        .tint = 0x2B3137,
        .on_change = []() {
          renodx::utils::platform::LaunchURL("https://github.com/", "clshortfuse/renodx/wiki/Mods");
        },
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::BUTTON,
        .label = "Github",
        .section = "Links",
        .group = "button-line-1",
        .tint = 0x2B3137,
        .on_change = []() {
          renodx::utils::platform::LaunchURL("https://github.com/", "clshortfuse/renodx");
        },
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::BUTTON,
        .label = "Musa's Ko-Fi",
        .section = "Links",
        .group = "button-line-2",
        .tint = 0xFF5A16,
        .on_change = []() {
          renodx::utils::platform::LaunchURL("https://ko-fi.com/", "musaqh");
        },
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::BUTTON,
        .label = "ShortFuse's Ko-Fi",
        .section = "Links",
        .group = "button-line-2",
        .tint = 0xFF5A16,
        .on_change = []() {
          renodx::utils::platform::LaunchURL("https://ko-fi.com/", "shortfuse");
        },
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::TEXT,
        .label = std::string("Build: ") + renodx::utils::date::ISO_DATE_TIME,
        .section = "Über",
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::TEXT,
        .label = std::string("- Erfordert aktiviertes HDR im Spiel\n"),
        .section = "Über",
    },
};

void OnPresetOff() {
  renodx::utils::settings::UpdateSettings({
      {"ToneMapType", 0.f},
      {"ToneMapPeakNits", 1000.f},
      {"ToneMapGameNits", 203.f},
      {"ToneMapUINits", 203.f},
      {"ToneMapScaling", 1.f},
      {"ToneMapConeResponse", 60.f},
      {"ColorGradeExposure", 1.f},
      {"ColorGradeHighlights", 50.f},
      {"ColorGradeHighlightContrast", 50.f},
      {"ColorGradeShadows", 50.f},
      {"ColorGradeShadowContrast", 50.f},
      {"ColorGradeContrast", 50.f},
      {"ColorGradeGamma", 1.f},
      {"ColorGradeSaturation", 50.f},
      {"ColorGradeHighlightSaturation", 50.f},
      {"ColorGradeDechroma", 0.f},
      {"ColorGradeFlare", 0.f},
      {"ColorGradeLUTStrength", 100.f},
      {"FxBloom", 100.f},
      {"FxFilmGrainType", 0.f},
      {"FxGrainStrength", 50.f},
      {"FxISFASTShadows", 0.f},
      {"FxSSRReflectionFix", 0.f},
  });
  firstlight::dlss::OnPresetOff();
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
extern "C" __declspec(dllexport) constexpr const char* DESCRIPTION = "RenoDX für 007 First Light";

BOOL APIENTRY DllMain(HMODULE h_module, DWORD fdw_reason, LPVOID lpv_reserved) {
  switch (fdw_reason) {
    case DLL_PROCESS_ATTACH:
      if (!reshade::register_addon(h_module)) return FALSE;

      if (!initialized) {
        renodx::mods::shader::force_pipeline_cloning = true;
        renodx::mods::shader::allow_multiple_push_constants = true;
        renodx::mods::shader::expected_constant_buffer_space = 50;

        firstlight::isfast::AddShaders(custom_shaders);
        firstlight::dlss::AppendSettings(settings);

        initialized = true;
      }
      reshade::register_event<reshade::addon_event::init_swapchain>(OnInitSwapchain);  // detect peak nits
      reshade::register_event<reshade::addon_event::init_device>(firstlight::isfast::OnInitDevice);
      reshade::register_event<reshade::addon_event::destroy_device>(firstlight::isfast::OnDestroyDevice);

      break;
    case DLL_PROCESS_DETACH:
      reshade::unregister_event<reshade::addon_event::init_swapchain>(OnInitSwapchain);  // detect peak nits
      reshade::unregister_event<reshade::addon_event::init_device>(firstlight::isfast::OnInitDevice);
      reshade::unregister_event<reshade::addon_event::destroy_device>(firstlight::isfast::OnDestroyDevice);

      reshade::unregister_addon(h_module);
      break;
  }

  renodx::utils::settings::Use(fdw_reason, &settings, &OnPresetOff);
  renodx::mods::shader::Use(fdw_reason, custom_shaders, &shader_injection);
  firstlight::dlss::Use(fdw_reason);

  return TRUE;
}
