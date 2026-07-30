/*
 * Copyright (C) 2024 Musa Haji
 * Copyright (C) 2024 Carlos Lopez
 * SPDX-License-Identifier: MIT
 */

#define ImTextureID ImU64

#define DEBUG_LEVEL_0
#define DEBUG_SLIDERS_OFF

#include <deps/imgui/imgui.h>
#include <include/reshade.hpp>

#include <embed/shaders.h>

#include "../../mods/shader.hpp"
#include "../../utils/date.hpp"
#include "../../utils/settings.hpp"
#include "./shared.h"

namespace {

renodx::mods::shader::CustomShaders custom_shaders = {
    CustomShaderEntry(0xF4EFA04D),  // Tonemapping + PQ mit TAA
    CustomShaderEntry(0x279D11F6),  // Tonemapping + PQ mit DLSS/FSR

    CustomShaderEntry(0x8A2543B3),  // Kalibrierungsmenue
};

ShaderInjectData shader_injection;

constexpr float TONE_MAP_TYPE_NEUTWO = 1.f;
constexpr float TONE_MAP_TYPE_PSYCHOV22 = 2.f;

// Sichtbarkeit der Regler haengt vom gewaehlten Tonemapper ab.
bool IsToneMapped() { return shader_injection.tone_map_type != 0.f; }

bool IsNeutwo() { return shader_injection.tone_map_type == TONE_MAP_TYPE_NEUTWO; }

bool IsPsychoV22() { return shader_injection.tone_map_type == TONE_MAP_TYPE_PSYCHOV22; }

// Neutwo hat kein eigenes Farbtonmodell und braucht die SDR-Emulation.
// PsychoV-22 bringt mit MB-Farbtonwiederherstellung und M-Zapfen-Bias ein
// eigenes mit und startet deshalb neutral bei 0.
// Gibt die Einstellung zurueck, damit der Aufrufer den Wert uebernehmen kann.
renodx::utils::settings::Setting* SyncHueShiftDefault(float tone_map_type) {
  auto* hue_shift = renodx::utils::settings::FindSetting("ToneMapHueShift");
  if (hue_shift != nullptr) {
    hue_shift->default_value = (tone_map_type == TONE_MAP_TYPE_PSYCHOV22) ? 0.f : 20.f;
  }
  return hue_shift;
}

renodx::utils::settings::Settings settings = {
    new renodx::utils::settings::Setting{
        .key = "ToneMapType",
        .binding = &shader_injection.tone_map_type,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .label = "Tonemapper",
        .section = "Tonemapping",
        .tooltip = "Legt den Typ des Tonemappers fest",
        .labels = {"Vanilla (Keiner)", "Neutwo", "PsychoV-22"},
        .on_change_value = [](float previous, float current) {
          // Laeuft bereits unter global_mutex, deshalb direkt setzen:
          // UpdateSetting() wuerde dieselbe Sperre erneut anfordern.
          auto* hue_shift = SyncHueShiftDefault(current);
          if (hue_shift != nullptr) {
            hue_shift->Set(hue_shift->default_value)->Write();
          }
        },
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
        .key = "ToneMapOverrideBrightness",
        .binding = &shader_injection.tone_map_override_brightness,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Helligkeitsregler überschreiben",
        .section = "Tonemapping",
        .tooltip = "Überschreibt den spielinternen Helligkeitsregler",
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
        .is_enabled = []() { return shader_injection.tone_map_override_brightness != 0; },
    },
    new renodx::utils::settings::Setting{
        .key = "ToneMapGammaCorrection",
        .binding = &shader_injection.gamma_correction,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .label = "SDR-EOTF-Emulation",
        .section = "Tonemapping",
        .tooltip = "Überschreibt den spielinternen Kontrastregler und emuliert eine 2.2-EOTF",
        .labels = {"Vanilla (Kontrastregler)", "2.2 (Pro Kanal)"},
    },
    new renodx::utils::settings::Setting{
        .key = "ToneMapHueShift",
        .binding = &shader_injection.tone_map_hue_shift,
        .default_value = 20.f,
        .label = "Farbtonverschiebung",
        .section = "Tonemapping",
        .tooltip = "Emuliert die Farbtonverschiebung des SDR-Tonemappings."
                   "\nStandard bei Neutwo: 20."
                   "\nStandard bei PsychoV-22: 0, da dieser ein eigenes"
                   "\nFarbtonmodell mitbringt.",
        .max = 100.f,
        .is_enabled = []() { return IsToneMapped(); },
        .parse = [](float value) { return value * 0.01f; },
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
        .tooltip = "Bei PsychoV-22 steuert dies die Farbreinheit im LMS-Bereich.",
        .max = 100.f,
        .is_enabled = []() { return IsToneMapped(); },
        .parse = [](float value) { return value * 0.02f; },
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeConeResponse",
        .binding = &shader_injection.tone_map_cone_response,
        .default_value = 50.f,
        .label = "Zapfenantwort",
        .section = "Farbkorrektur",
        .tooltip = "Steuert die Zapfenantwort-Formung von PsychoV-22."
                   "\nSkaliert Kontrast und Farbreinheit gemeinsam."
                   "\nNur mit PsychoV-22 verfügbar.",
        .max = 100.f,
        .is_enabled = []() { return IsPsychoV22(); },
        .parse = [](float value) { return value * 0.02f; },
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeHighlightSaturation",
        .binding = &shader_injection.tone_map_highlight_saturation,
        .default_value = 50.f,
        .label = "Lichtersättigung",
        .section = "Farbkorrektur",
        .tooltip = "Fügt den Lichtern Farbe hinzu oder entzieht sie."
                   "\nNur mit Neutwo verfügbar.",
        .max = 100.f,
        .is_enabled = []() { return IsNeutwo(); },
        .parse = [](float value) { return value * 0.02f; },
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeBlowout",
        .binding = &shader_injection.tone_map_blowout,
        .default_value = 0.f,
        .label = "Überstrahlung",
        .section = "Farbkorrektur",
        .tooltip = "Steuert die Entsättigung der Lichter durch Überbelichtung."
                   "\nNur mit Neutwo verfügbar.",
        .max = 100.f,
        .is_enabled = []() { return IsNeutwo(); },
        .parse = [](float value) { return value * 0.01f; },
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeFlare",
        .binding = &shader_injection.tone_map_flare,
        .default_value = 0.f,
        .label = "Streulicht",
        .section = "Farbkorrektur",
        .tooltip = "Kompensation von Streu- und Blendlicht."
                   "\nNur mit Neutwo verfügbar.",
        .max = 100.f,
        .is_enabled = []() { return IsNeutwo(); },
        .parse = [](float value) { return value * 0.01f; },
    },
    new renodx::utils::settings::Setting{
        .key = "HDR10Encoding",
        .binding = &shader_injection.custom_hdr10_encoding,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .label = "HDR10-Kodierung",
        .section = "Erweitert",
        .tooltip = "Legt das Format der HDR10-Kodierung fest",
        .labels = {"Vanilla (PQ-Näherung)", "PQ"},
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::BUTTON,
        .label = "Alles zurücksetzen",
        .section = "Optionen",
        .group = "button-line-1",
        .on_change = []() {
          // Die Schleife setzt den Tonemapper mit zurueck, deshalb muss der
          // Standard der Farbtonverschiebung vorher darauf zeigen.
          auto* tone_map_type = renodx::utils::settings::FindSetting("ToneMapType");
          if (tone_map_type != nullptr) {
            SyncHueShiftDefault(tone_map_type->default_value);
          }
          for (auto* setting : settings) {
            if (setting->key.empty()) continue;
            if (!setting->can_reset) continue;
            renodx::utils::settings::UpdateSetting(setting->key, setting->default_value);
          }
        },
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::BUTTON,
        .label = "Discord",
        .section = "Links",
        .group = "button-line-2",
        .tint = 0x5865F2,
        .on_change = []() {
          renodx::utils::platform::LaunchURL("https://discord.gg/", "5WZXDpmbpP");
        },
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::BUTTON,
        .label = "Weitere Mods",
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
        .value_type = renodx::utils::settings::SettingValueType::TEXT,
        .label = std::string("Build: ") + renodx::utils::date::ISO_DATE_TIME,
        .section = "Über",
    },
};

void OnPresetOff() {
  renodx::utils::settings::UpdateSetting("ToneMapType", 0);
  renodx::utils::settings::UpdateSetting("ToneMapPeakNits", 1000.f);
  renodx::utils::settings::UpdateSetting("ToneMapGameNits", 203.f);
  renodx::utils::settings::UpdateSetting("ToneMapGammaCorrection", 0.f);
  renodx::utils::settings::UpdateSetting("ToneMapHueShift", 0.f);
  renodx::utils::settings::UpdateSetting("ColorGradeExposure", 1.f);
  renodx::utils::settings::UpdateSetting("ColorGradeHighlights", 50.f);
  renodx::utils::settings::UpdateSetting("ColorGradeShadows", 50.f);
  renodx::utils::settings::UpdateSetting("ColorGradeContrast", 50.f);
  renodx::utils::settings::UpdateSetting("ColorGradeSaturation", 50.f);
  renodx::utils::settings::UpdateSetting("ColorGradeConeResponse", 50.f);
  renodx::utils::settings::UpdateSetting("ColorGradeHighlightSaturation", 50.f);
  renodx::utils::settings::UpdateSetting("ColorGradeBlowout", 0.f);
  renodx::utils::settings::UpdateSetting("ColorGradeFlare", 0.f);
  renodx::utils::settings::UpdateSetting("HDR10Encoding", 0.f);
  renodx::utils::settings::UpdateSetting("ToneMapOverrideBrightness", 0.f);
}

bool fired_on_init_swapchain = false;

void OnInitSwapchain(reshade::api::swapchain* swapchain, bool resize) {
  if (fired_on_init_swapchain) return;
  fired_on_init_swapchain = true;
  // Nur den Standard an den geladenen Tonemapper angleichen. Ein gespeicherter
  // eigener Wert bleibt erhalten.
  SyncHueShiftDefault(shader_injection.tone_map_type);
  auto peak = renodx::utils::swapchain::GetPeakNits(swapchain);
  if (peak.has_value()) {
    settings[1]->default_value = peak.value();
    settings[1]->can_reset = true;
  }
}

}  // namespace

extern "C" __declspec(dllexport) constexpr const char* NAME = "RenoDX";
extern "C" __declspec(dllexport) constexpr const char* DESCRIPTION = "RenoDX für God of War (2018)";

BOOL APIENTRY DllMain(HMODULE h_module, DWORD fdw_reason, LPVOID lpv_reserved) {
  switch (fdw_reason) {
    case DLL_PROCESS_ATTACH:
      if (!reshade::register_addon(h_module)) return FALSE;

      renodx::mods::shader::force_pipeline_cloning = true;
      renodx::mods::shader::expected_constant_buffer_index = 11;

      reshade::register_event<reshade::addon_event::init_swapchain>(OnInitSwapchain);
      break;
    case DLL_PROCESS_DETACH:
      reshade::unregister_event<reshade::addon_event::init_swapchain>(OnInitSwapchain);
      reshade::unregister_addon(h_module);
      break;
  }

  renodx::utils::settings::Use(fdw_reason, &settings, &OnPresetOff);
  renodx::mods::shader::Use(fdw_reason, custom_shaders, &shader_injection);

  return TRUE;
}