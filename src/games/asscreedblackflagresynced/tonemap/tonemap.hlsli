#include "../common.hlsli"

struct ImmortalsToneMapConfig {
  float slope;
  float toe_threshold;
  float toe_slope;
  float black_offset;
  float peak_luminance;
  float shoulder_start;
  float shoulder_scale;
  float shoulder_overage;
  bool has_toe;
};

ImmortalsToneMapConfig CreateImmortalsToneMapConfig(
    float slope,
    float toe_threshold,
    float shoulder_start,
    float toe_slope,
    float black_offset,
    float peak_nits) {
  ImmortalsToneMapConfig config;
  config.slope = slope;
  config.toe_threshold = toe_threshold;
  config.toe_slope = toe_slope;
  config.black_offset = black_offset;
  config.peak_luminance = peak_nits * 0.00999999977648258209228515625f;
  config.has_toe = config.toe_threshold > 9.9999997473787516355514526367188e-06f;

  float toe_to_peak_range = mad(peak_nits, 0.00999999977648258209228515625f, -config.toe_threshold);
  float shoulder_start_output = mad(toe_to_peak_range, shoulder_start, config.toe_threshold);
  config.shoulder_start = ((toe_to_peak_range * shoulder_start) / config.slope) + config.toe_threshold;
  config.shoulder_scale = (config.peak_luminance * config.slope) / mad(peak_nits, 0.00999999977648258209228515625f, -shoulder_start_output);
  config.shoulder_overage = mad(-peak_nits, 0.00999999977648258209228515625f, shoulder_start_output);
  return config;
}

#define IMMORTALS_TONEMAP_GENERATOR(T)                                                                                                                                                                       \
  T ApplyImmortalsToneMap(T untonemapped_ap1, ImmortalsToneMapConfig config, out T precompression_ap1) {                                                                                                     \
    T input_scaled = abs(untonemapped_ap1 * 0.00999999977648258209228515625f);                                                                                                                               \
    T toe_ratio = input_scaled / config.toe_threshold;                                                                                                                                                       \
    T toe_ratio_sat = saturate(toe_ratio);                                                                                                                                                                   \
    T toe_ratio_sat_sq = toe_ratio_sat * toe_ratio_sat;                                                                                                                                                      \
    T toe_smooth = mad(toe_ratio_sat, -2.f, 3.f);                                                                                                                                                            \
    T in_shoulder = renodx::math::Select(input_scaled > config.shoulder_start, (T)1.f, (T)0.f);                                                                                                              \
    T toe_curve = renodx::math::Select(config.has_toe, mad(exp2(log2(abs(toe_ratio)) * config.toe_slope), config.toe_threshold, config.black_offset), config.black_offset);                                  \
    T toe_weight = mad(-toe_smooth, toe_ratio_sat_sq, 1.f);                                                                                                                                                  \
    T linear_curve = mad(input_scaled - config.toe_threshold, config.slope, config.toe_threshold);                                                                                                           \
    T linear_weight = mad(toe_smooth, toe_ratio_sat_sq, -1.f) + 1.f;                                                                                                                                         \
    T precompression_curve = (toe_weight * toe_curve) + (linear_weight * linear_curve);                                                                                                                      \
    T shoulder_curve = config.peak_luminance + (exp2(((config.shoulder_scale * (input_scaled - config.shoulder_start)) / config.peak_luminance) * (-1.44269502162933349609375f)) * config.shoulder_overage); \
    precompression_ap1 = precompression_curve * 100.f;                                                                                                                                                       \
    return lerp(precompression_curve, shoulder_curve, in_shoulder) * 100.f;                                                                                                                                  \
  }                                                                                                                                                                                                          \
  T ApplyImmortalsToneMap(T untonemapped_ap1, ImmortalsToneMapConfig config) {                                                                                                                               \
    T precompression_ap1;                                                                                                                                                                                    \
    return ApplyImmortalsToneMap(untonemapped_ap1, config, precompression_ap1);                                                                                                                              \
  }

IMMORTALS_TONEMAP_GENERATOR(float)
IMMORTALS_TONEMAP_GENERATOR(float3)
#undef IMMORTALS_TONEMAP_GENERATOR

float3 BuildToneMapLUTOutput(float3 untonemapped_ap1, float exposure, float display_peak_nits, bool hdr_enabled) {
  // The game uses twice the SDR exposure by default when HDR is enabled.
  float diffuse_white_nits = (exposure / 64.f) * 203.f;
  float target_peak_ratio = display_peak_nits / diffuse_white_nits;
  float3 tonemapped_bt709;

  if (!hdr_enabled) {
    target_peak_ratio = 1.f;
  }

  if (RENODX_TONE_MAP_TYPE == BLACKFLAG_TONE_MAP_TYPE_PSYCHOV22) {
    // PsychoV-22 replaces the Immortals curve outright rather than post-processing
    // its output. Anchor input and output stay the ones the Immortals curve used,
    // so mid gray lands in the same place and the grading LUT, colour filter and
    // bloom keep their working point.
    //
    // Every grading control is passed neutral: exposure, contrast, saturation and
    // the rest already ran in ApplyUserGradingAP1 ahead of this LUT, so feeding
    // them again would grade twice.
    //
    // The BT.2020 hull is used in HDR. Colours outside BT.709 show up as negative
    // BT.709 components and every stage up to the BT.2020 conversion below is
    // sign-preserving.
    tonemapped_bt709 = renodx::tonemap::psychov::psychotm_test22(
        renodx::color::bt709::from::AP1(untonemapped_ap1 * 0.01f),
        target_peak_ratio,
        1.f,    // exposure (already applied)
        1.f,    // highlights (already applied)
        1.f,    // shadows (already applied)
        1.f,    // contrast (already applied)
        1.f,    // purity (already applied)
        1.f,    // bleaching (reserved)
        100.f,  // clip point (reserved)
        1.f,    // hue restore (reserved)
        1.f,    // adaptation contrast (deprecated)
        0,      // white curve mode (deprecated)
        RENODX_TONE_MAP_CONE_RESPONSE,
        BLACKFLAG_SCENE_ANCHOR.xxx,   // anchor in: scene value that maps to mid gray
        BLACKFLAG_OUTPUT_ANCHOR.xxx,  // anchor out: SDR mid gray
        1.f,                          // gamut compression
        hdr_enabled ? 1 : 0,          // BT.2020 / BT.709 hull
        1.f,                          // adaptive normalization (deprecated)
        0.f);                         // 0 = automatic compression
  } else {
    if (RENODX_GAME_GAMMA_CORRECTION != 0.f) {
      target_peak_ratio = renodx::color::correct::GammaSafe(target_peak_ratio, true);
    }

    ImmortalsToneMapConfig config = CreateImmortalsToneMapConfig(
        BLACKFLAG_IMMORTALS_SLOPE,
        BLACKFLAG_IMMORTALS_TOE_THRESHOLD,
        BLACKFLAG_IMMORTALS_SHOULDER_START,
        BLACKFLAG_IMMORTALS_TOE_SLOPE,
        BLACKFLAG_IMMORTALS_BLACK_OFFSET,
        target_peak_ratio * 100.f);
    float3 tonemapped_ap1 = ApplyImmortalsToneMap(untonemapped_ap1, config) / 100.f;
    tonemapped_bt709 = renodx::color::bt709::from::AP1(tonemapped_ap1);

    // The curve has no isolated inflection between its convex toe and concave
    // shoulder. Fix the output anchor at SDR midgray and solve its input anchor
    // from the linear section.
    float3 input_adaptive_anchor_lms =
        renodx::color::lms::from::AP1((100.f * BLACKFLAG_SCENE_ANCHOR).xxx);
    float3 tonemapped_lms = renodx::color::lms::from::BT709(tonemapped_bt709);
    float3 tonemapped_relative_weighted = renodx::tonemap::psychov::psycho22_ToAdaptiveRelativeWeightedLMS(
        tonemapped_lms,
        input_adaptive_anchor_lms);
    tonemapped_relative_weighted = renodx::tonemap::psychov::psycho22_GamutCompressAdaptiveRelativeWeightedLMSBound(
        tonemapped_relative_weighted,
        input_adaptive_anchor_lms,
        hdr_enabled ? renodx::color::macleod_boynton::BT2020_TO_LMS_WEIGHTED_MAT
                    : renodx::color::macleod_boynton::BT709_TO_LMS_WEIGHTED_MAT,
        1.f);
    tonemapped_bt709 = renodx::color::bt709::from::LMS(
        renodx::color::macleod_boynton::UnweighLMS(
            renodx::tonemap::psychov::psycho22_FromAdaptiveRelativeWeightedLMS(
                tonemapped_relative_weighted,
                input_adaptive_anchor_lms)));

    if (RENODX_GAME_GAMMA_CORRECTION != 0.f) {
      tonemapped_bt709 = renodx::color::correct::GammaSafe(tonemapped_bt709);
    }
  }

  if (hdr_enabled) {
    return renodx::color::pq::EncodeSafe(renodx::color::bt2020::from::BT709(tonemapped_bt709), diffuse_white_nits);
  }
  return renodx::color::gamma::EncodeSafe(tonemapped_bt709);
}
