#include "./shared.h"

// Parameters of the game's Immortals-style curve, shared by the "RenoDX
// (Vanilla+)" tone mapper and by the anchors PsychoV-22 is matched to.
static const float BLACKFLAG_IMMORTALS_SLOPE = 1.5f;
static const float BLACKFLAG_IMMORTALS_TOE_THRESHOLD = 0.05f;
static const float BLACKFLAG_IMMORTALS_SHOULDER_START = 0.5f;
static const float BLACKFLAG_IMMORTALS_TOE_SLOPE = 1.f;
static const float BLACKFLAG_IMMORTALS_BLACK_OFFSET = 0.f;

// Output anchor, fixed at SDR mid gray.
static const float BLACKFLAG_OUTPUT_ANCHOR = 0.18f;

// Input anchor, solved from the linear section of the curve so that it lands on
// BLACKFLAG_OUTPUT_ANCHOR. Note the two live in different scales: the scene
// values reaching the tone mapper are 100x this.
static const float BLACKFLAG_SCENE_ANCHOR =
    BLACKFLAG_IMMORTALS_TOE_THRESHOLD
    + ((BLACKFLAG_OUTPUT_ANCHOR - BLACKFLAG_IMMORTALS_TOE_THRESHOLD) / BLACKFLAG_IMMORTALS_SLOPE);

// Log-log slope of the Immortals curve at the anchor. In the linear section
// y = (x - toe) * slope + toe, so d(log y)/d(log x) = slope * x_anchor / y_anchor.
// PsychoV-22's cone response is the same quantity, which is what makes the two
// tone mappers comparable at their default settings.
static const float BLACKFLAG_IMMORTALS_ANCHOR_SLOPE =
    BLACKFLAG_IMMORTALS_SLOPE * BLACKFLAG_SCENE_ANCHOR / BLACKFLAG_OUTPUT_ANCHOR;

float ToneMapAnchorSlope() {
  return RENODX_TONE_MAP_TYPE == BLACKFLAG_TONE_MAP_TYPE_PSYCHOV22
             ? max(RENODX_TONE_MAP_CONE_RESPONSE, 1e-3f)
             : BLACKFLAG_IMMORTALS_ANCHOR_SLOPE;
}

// Scene value that the tone curve maps to peak white.
//
// The purity controls run in the grading LUT, ahead of the tone mapper, so they
// only ever see scene-referred values. Inverting the curve's anchor slope gives
// them a reference that tracks the actual display headroom instead of the fixed
// constant this used to carry:
//   scene_peak / scene_mid = (display_peak / display_mid) ^ (1 / anchor_slope)
float ScenePeakReference(float mid_gray) {
  float display_peak_over_mid_gray = max(CUSTOM_PEAK_RATIO / BLACKFLAG_OUTPUT_ANCHOR, 1.f);
  return mid_gray * pow(display_peak_over_mid_gray, 1.f / ToneMapAnchorSlope());
}

float ContrastAndFlare(float x, float contrast, float contrast_highlights, float contrast_shadows, float flare, float mid_gray = 0.18f) {
  if (contrast == 1.f && flare == 0.f && contrast_highlights == 1.f && contrast_shadows == 1.f) return x;

  const float x_normalized = x / mid_gray;
  const float split_contrast = renodx::math::Select(x < mid_gray, contrast_shadows, contrast_highlights);
  float flare_ratio = renodx::math::DivideSafe(x_normalized + flare, x_normalized, 1.f);
  float exponent = contrast * split_contrast * flare_ratio;
  return pow(x_normalized, exponent) * mid_gray;
}

float ApplyLuminanceGradingChannel(float channel, float gamma, float exposure, float highlights, float shadows, float contrast, float contrast_highlights, float contrast_shadows, float flare, float mid_gray = 0.18f) {
  float channel_adjusted = channel * exposure;
  if (gamma != 1.f) {
    channel_adjusted = renodx::math::Select(channel_adjusted < 1.f, pow(channel_adjusted, gamma), channel_adjusted);
  }
  channel_adjusted = renodx::color::grade::Highlights(channel_adjusted, highlights, mid_gray);
  channel_adjusted = renodx::color::grade::Shadows(channel_adjusted, shadows, mid_gray);
  channel_adjusted = ContrastAndFlare(channel_adjusted, contrast, contrast_highlights, contrast_shadows, flare, mid_gray);
  return channel_adjusted;
}

float3 ApplyLuminanceGradingAP1(float3 color, float gamma, float exposure, float highlights, float shadows, float contrast, float contrast_highlights, float contrast_shadows, float flare, float mid_gray = 0.18f) {
  float y = max(0.f, renodx::color::yf::from::AP1(color));
  float y_adjusted = ApplyLuminanceGradingChannel(y, gamma, exposure, highlights, shadows, contrast, contrast_highlights, contrast_shadows, flare, mid_gray);
  float3 color_adjusted = color * renodx::math::DivideSafe(y_adjusted, y, 1.f);

  return color_adjusted;
}

float3 ApplyPurityGradingLMS(float3 color_lms, float purity_scale, float purity_highlights, float dechroma, float3 mid_gray_lms, float scene_peak_reference) {
  if (purity_scale == 1.f && purity_highlights == 0.f && dechroma == 0.f) return color_lms;

  float lum_target = max(0.f, renodx::color::yf::from::LMS(color_lms));
  float percent_max = saturate(renodx::math::DivideSafe(lum_target, scene_peak_reference, 0.f));

  if (dechroma != 0.f) {
    purity_scale *= lerp(1.f, 0.f, saturate(pow(percent_max, 1.f - dechroma)));
  }

  if (purity_highlights != 0.f) {
    float blowout_change = pow(1.f - percent_max, 100.f * abs(purity_highlights));
    if (purity_highlights < 0.f) {
      blowout_change = 2.f - blowout_change;
    }
    purity_scale *= blowout_change;
  }

  if (purity_scale != 1.f) {
    color_lms = renodx::tonemap::psychov::psycho22_ApplyAdaptiveMBPurity(color_lms, mid_gray_lms, purity_scale);
  }

  return color_lms;
}

float3 ApplyCoolnessAP1(float3 color_ap1, float strength) {
  return lerp(
      color_ap1,
      renodx::color::ap1::from::BT709(renodx::color::bt709::from::BT709D93(renodx::color::bt709::from::AP1(color_ap1))),
      strength);
}

float3 ApplyUserGradingAP1(float3 color_ap1, float mid_gray = 0.18f) {
  color_ap1 = ApplyLuminanceGradingAP1(color_ap1,
                                       1.f,
                                       RENODX_TONE_MAP_EXPOSURE,
                                       RENODX_TONE_MAP_HIGHLIGHTS,
                                       RENODX_TONE_MAP_SHADOWS,
                                       RENODX_TONE_MAP_CONTRAST,
                                       1.f,
                                       1.f,
                                       0.10f * pow(RENODX_TONE_MAP_FLARE, 10.f),
                                       mid_gray);

  float3 color_lms = renodx::color::lms::from::AP1(color_ap1);
  color_lms = ApplyPurityGradingLMS(
      color_lms,
      RENODX_TONE_MAP_SATURATION,
      -1.f * (RENODX_TONE_MAP_HIGHLIGHT_SATURATION - 1.f),
      RENODX_TONE_MAP_DECHROMA,
      renodx::color::lms::from::AP1(mid_gray.xxx),
      ScenePeakReference(mid_gray));

  color_ap1 = renodx::color::ap1::from::LMS(color_lms);
  color_ap1 = ApplyCoolnessAP1(color_ap1, RENODX_COLOR_GRADE_COOLNESS);

  return color_ap1;
}
