#include "./shared.h"

float3 ApplyExposureContrastFlareHighlightsShadowsByLuminance(float3 untonemapped, float y, renodx::color::grade::Config config, float mid_gray = 0.18f) {
  if (config.exposure == 1.f && config.shadows == 1.f && config.highlights == 1.f && config.contrast == 1.f && config.flare == 0.f) {
    return untonemapped;
  }
  float3 color = untonemapped;

  color *= config.exposure;

  const float y_normalized = y / mid_gray;
  const float highlight_mask = 1.f / mid_gray;
  const float shadow_mask = mid_gray;

  // contrast & flare
  float flare = renodx::math::DivideSafe(y_normalized + config.flare, y_normalized, 1.f);
  float exponent = config.contrast * flare;
  const float y_contrasted = pow(y_normalized, exponent);

  // highlights
  float y_highlighted = pow(y_contrasted, config.highlights);
  y_highlighted = lerp(y_contrasted, y_highlighted, saturate(y_contrasted / highlight_mask));

  // shadows
  float y_shadowed = pow(y_highlighted, -1.f * (config.shadows - 2.f));
  y_shadowed = lerp(y_shadowed, y_highlighted, saturate(y_highlighted / shadow_mask));

  const float y_final = y_shadowed * mid_gray;

  color *= (y > 0 ? (y_final / y) : 0);

  return color;
}

float3 ApplySaturationBlowoutHueCorrectionHighlightSaturation(float3 tonemapped, float3 untonemapped, float y, renodx::color::grade::Config config) {
  float3 color = tonemapped;
  if (config.saturation != 1.f || config.dechroma != 0.f || config.hue_correction_strength != 0.f || config.blowout != 0.f) {
    float3 perceptual_new = renodx::color::oklab::from::BT709(color);

    if (config.hue_correction_strength != 0.f) {
      float3 perceptual_old = renodx::color::oklab::from::BT709(untonemapped);

      // Save chrominance to apply black
      float chrominance_pre_adjust = distance(perceptual_new.yz, 0);

      perceptual_new.yz = lerp(perceptual_new.yz, perceptual_old.yz, config.hue_correction_strength);

      float chrominance_post_adjust = distance(perceptual_new.yz, 0);

      // Apply back previous chrominance
      perceptual_new.yz *= renodx::math::DivideSafe(chrominance_pre_adjust, chrominance_post_adjust, 1.f);
    }

    if (config.dechroma != 0.f) {
      perceptual_new.yz *= lerp(1.f, 0.f, saturate(pow(y / (10000.f / 100.f), (1.f - config.dechroma))));
    }

    if (config.blowout != 0.f) {
      float percent_max = saturate(y * 100.f / 10000.f);
      // positive = 1 to 0, negative = 1 to 2
      float blowout_strength = 100.f;
      float blowout_change = pow(1.f - percent_max, blowout_strength * abs(config.blowout));
      if (config.blowout < 0) {
        blowout_change = (2.f - blowout_change);
      }

      perceptual_new.yz *= blowout_change;
    }

    perceptual_new.yz *= config.saturation;

    color = renodx::color::bt709::from::OkLab(perceptual_new);

    color = renodx::color::bt709::clamp::AP1(color);
  }
  return color;
}

float3 ApplyExposureContrastFlareHighlightsShadowsByLuminanceBT2020(float3 ungraded) {
  renodx::color::grade::Config cg_config = renodx::color::grade::config::Create();
  cg_config.exposure = RENODX_TONE_MAP_EXPOSURE;
  cg_config.highlights = RENODX_TONE_MAP_HIGHLIGHTS;
  cg_config.shadows = RENODX_TONE_MAP_SHADOWS;
  cg_config.contrast = RENODX_TONE_MAP_CONTRAST;
  cg_config.flare = 0.10f * pow(RENODX_TONE_MAP_FLARE, 10.f);

  return ApplyExposureContrastFlareHighlightsShadowsByLuminance(ungraded, renodx::color::y::from::BT2020(ungraded), cg_config, 0.18f);
}

float3 ApplySaturationBlowoutHueCorrectionHighlightSaturation(float3 ungraded, float3 untonemapped) {
  renodx::color::grade::Config cg_config = renodx::color::grade::config::Create();
  cg_config.saturation = RENODX_TONE_MAP_SATURATION;
  cg_config.dechroma = RENODX_TONE_MAP_BLOWOUT;
  cg_config.hue_correction_strength = 0.f;
  cg_config.blowout = -1.f * (RENODX_TONE_MAP_HIGHLIGHT_SATURATION - 1.f);

  return ApplySaturationBlowoutHueCorrectionHighlightSaturation(ungraded, ungraded, renodx::color::y::from::BT709(untonemapped), cg_config);
}

float3 ApplyGammaCorrection(float3 incorrect_color) {
  return renodx::color::correct::GammaSafe(incorrect_color);
}

// Wendet den ausgewaehlten Tonemapper samt Farbkorrektur an.
// Ein- und Ausgabe sind BT.2020-linear, normiert auf 1.0 = Diffusweiss.
// peak_ratio ist die Spitzenhelligkeit relativ zum Diffusweiss.
float3 ApplyToneMapBT2020(float3 ungraded, float3 untonemapped, float peak_ratio) {
  // PsychoV-22 gradet selbst im LMS-Bereich: Belichtung, Lichter, Schatten,
  // Kontrast und Saettigung gehen direkt in die Kurve, damit nicht zweimal
  // gegradet wird. Streulicht, Ueberstrahlung und Lichtersaettigung gehoeren
  // zum Neutwo-Pfad und bleiben hier ungenutzt.
  if (RENODX_TONE_MAP_TYPE == RENODX_TONE_MAP_TYPE_PSYCHOV22) {
    return renodx::color::bt2020::from::BT709(
        renodx::tonemap::psychov::psychotm_test22(
            renodx::color::bt709::from::BT2020(ungraded),
            peak_ratio,
            RENODX_TONE_MAP_EXPOSURE,
            RENODX_TONE_MAP_HIGHLIGHTS,
            RENODX_TONE_MAP_SHADOWS,
            RENODX_TONE_MAP_CONTRAST,
            RENODX_TONE_MAP_SATURATION,  // Reinheit (purity)
            1.f,                         // Ausbleichen (reserviert)
            100.f,                       // Clip-Punkt (reserviert)
            1.f,                         // Farbtonwiederherstellung (reserviert)
            1.f,                         // Adaptionskontrast (veraltet)
            0,                           // Weisskurvenmodus (veraltet)
            RENODX_TONE_MAP_CONE_RESPONSE,
            0.18f.xxx,  // Adaptionszustand (Anker-Eingang)
            0.18f.xxx,  // Hintergrundzustand (Anker-Ausgang)
            1.f,        // Gamut-Kompression
            1,          // BT.2020-Huelle
            1.f,        // Adaptive Normalisierung (veraltet)
            0.f));      // 0 = automatische Kompression
  }

  float3 color = ungraded;
  if (RENODX_TONE_MAP_TYPE != RENODX_TONE_MAP_TYPE_VANILLA) {
    color = ApplyExposureContrastFlareHighlightsShadowsByLuminanceBT2020(color);
    color = renodx::tonemap::neutwo::PerChannel(color, peak_ratio);
  }
  return renodx::color::bt2020::from::BT709(
      ApplySaturationBlowoutHueCorrectionHighlightSaturation(
          renodx::color::bt709::from::BT2020(color), untonemapped));
}