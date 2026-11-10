#ifndef SRC_GAMES_RVTHEREYET_COMMON_HLSLI_
#define SRC_GAMES_RVTHEREYET_COMMON_HLSLI_

#include "./shared.h"

static const float MID_GRAY = 0.18f;

// Vanilla log encoding of the scene color that indexes the grading LUT
static const float LUT_LOG_OFFSET = 0.002667719265f;
static const float LUT_LOG_SCALE = 0.0714285746f;
static const float LUT_LOG_BIAS = 0.610726953f;
// The shader scales the LUT result before writing it
static const float LUT_OUTPUT_SCALE = 1.05f;

// Vanilla LUT result as linear light. The game stores the clipped, sRGB encoded LUT result.
float3 VanillaLutLinear(float3 lut_sample) {
  return renodx::color::srgb::DecodeSafe(saturate(lut_sample * LUT_OUTPUT_SCALE));
}

// Luminance the vanilla LUT produces for a neutral gray scene value. This is the game's own tonemap curve.
float VanillaGrayCurve(Texture3D<float4> lut, SamplerState lut_sampler, float2 lut_params, float gray) {
  float coord = saturate(log2(gray + LUT_LOG_OFFSET) * LUT_LOG_SCALE + LUT_LOG_BIAS);
  coord = coord * lut_params.x + lut_params.y;
  return renodx::color::y::from::BT709(VanillaLutLinear(lut.SampleLevel(lut_sampler, float3(coord, coord, coord), 0).xyz));
}

// untonemapped: linear scene color after exposure and bloom, input of the vanilla grading LUT.
// graded_sdr_encoded: vanilla LUT result as written by the game (tonemap + grade, sRGB encoded, scaled).
// lut / lut_sampler / lut_params (cb0[62].zw: scale, bias): the vanilla LUT.
float3 ToneMapLut(float3 untonemapped, float3 graded_sdr_encoded, Texture3D<float4> lut, SamplerState lut_sampler, float2 lut_params) {
  const float3 graded_sdr = renodx::color::srgb::DecodeSafe(saturate(graded_sdr_encoded));

  // Neutral SDR is the vanilla tonemap without the color grade: the LUT gray curve at the pixel luminance.
  // The delta between graded and neutral then isolates the grade, and the HDR tonemapper replaces the vanilla curve.
  const float untonemapped_y = renodx::color::y::from::BT709(untonemapped);
  const float3 neutral_sdr = untonemapped
                             * renodx::math::DivideSafe(VanillaGrayCurve(lut, lut_sampler, lut_params, untonemapped_y), untonemapped_y, 1.f);

  // Anchor PsychoV to the vanilla curve level and local slope (x * V'(x) / V(x)) at mid gray
  const float vanilla_mid_gray_out = VanillaGrayCurve(lut, lut_sampler, lut_params, MID_GRAY);
  const float slope = (VanillaGrayCurve(lut, lut_sampler, lut_params, MID_GRAY * 1.1f)
                       - VanillaGrayCurve(lut, lut_sampler, lut_params, MID_GRAY * 0.9f))
                      / (MID_GRAY * 0.2f);
  const float vanilla_cone = max(0.01f, MID_GRAY * slope / max(vanilla_mid_gray_out, 1e-5f));

  float peak_value = max(1.f, RENODX_PEAK_WHITE_NITS / RENODX_DIFFUSE_WHITE_NITS);
  if (RENODX_GAMMA_CORRECTION == renodx::draw::GAMMA_CORRECTION_GAMMA_2_2) {
    peak_value = renodx::color::correct::Gamma(peak_value, true, 2.2f);
  } else if (RENODX_GAMMA_CORRECTION == renodx::draw::GAMMA_CORRECTION_GAMMA_2_4) {
    peak_value = renodx::color::correct::Gamma(peak_value, true, 2.4f);
  }

  float3 graded_untonemapped = renodx::draw::ComputeUntonemappedGraded(untonemapped, graded_sdr, neutral_sdr) * RENODX_TONE_MAP_EXPOSURE;
  float3 input_lms = mul(renodx::tonemap::psychov::PSYCHO35_BT709_TO_LMS_MAT, graded_untonemapped);
  float input_yf = renodx::color::yf::from::LMS(input_lms);
  if (!(input_yf > 0.f)) {
    return renodx::draw::RenderIntermediatePass(0.f);
  }
  if (any(input_lms < 0.f)) {
    float3 neutral_lms = (input_yf / renodx::tonemap::psychov::PSYCHO35_D65_WHITE_YF) * renodx::tonemap::psychov::PSYCHO35_D65_WHITE_LMS;
    float3 delta_lms = input_lms - neutral_lms;
    float t = 1.f;
    if (delta_lms.x < 0.f) t = min(t, -neutral_lms.x / delta_lms.x);
    if (delta_lms.y < 0.f) t = min(t, -neutral_lms.y / delta_lms.y);
    if (delta_lms.z < 0.f) t = min(t, -neutral_lms.z / delta_lms.z);
    input_lms = max(0.f, neutral_lms + saturate(t) * delta_lms);
    graded_untonemapped = mul(renodx::tonemap::psychov::PSYCHO35_LMS_TO_BT709_MAT, input_lms);
  }
  if (RENODX_TONE_MAP_HIGHLIGHTS != 1.f || RENODX_TONE_MAP_SHADOWS != 1.f) {
    float anchor_yf = MID_GRAY * renodx::tonemap::psychov::PSYCHO35_D65_WHITE_YF;
    float target_y = input_yf;
    if (RENODX_TONE_MAP_HIGHLIGHTS != 1.f) {
      float x = saturate((target_y - anchor_yf) / anchor_yf);
      float w = x * x * x * (x * (x * 6.f - 15.f) + 10.f);
      float ratio = max(target_y / anchor_yf, 0.f);
      float shaped_y = anchor_yf * pow(ratio, RENODX_TONE_MAP_HIGHLIGHTS) + (target_y - anchor_yf);
      target_y = max(0.f, lerp(target_y, shaped_y, w));
    }
    if (RENODX_TONE_MAP_SHADOWS != 1.f) {
      float x = saturate((anchor_yf - target_y) / anchor_yf);
      float w = x * x * x * (x * (x * 6.f - 15.f) + 10.f);
      float ratio = max(target_y / anchor_yf, 0.f);
      float shaped_y = anchor_yf * pow(ratio, 2.f - RENODX_TONE_MAP_SHADOWS) + (target_y - anchor_yf);
      target_y = max(0.f, lerp(target_y, shaped_y, w));
    }
    graded_untonemapped *= target_y / input_yf;
  }

  float3 tonemapped = renodx::tonemap::psychov::psychotm_test35(
      graded_untonemapped,
      peak_value,
      1.f,
      1.f,
      1.f,
      RENODX_TONE_MAP_CONTRAST,
      RENODX_TONE_MAP_SATURATION,
      0.f,
      100.f,
      1.f,
      1.f,
      1,
      vanilla_cone * (max(0.01f, RENODX_PSYCHOV_CONE_RESPONSE)),
      float3(MID_GRAY, MID_GRAY, MID_GRAY),
      float3(vanilla_mid_gray_out, vanilla_mid_gray_out, vanilla_mid_gray_out),
      saturate(RENODX_PSYCHOV_GAMUT_COMPRESSION),
      (int)RENODX_PSYCHOV_GAMUT_MODE,
      1.f,
      clamp(RENODX_PSYCHOV_COMPRESSION, 0.f, 2.f),
      (int)RENODX_PSYCHOV_SOURCE_BOUNDARY);

  // psychotm_test35 has no blowout input (that slot is a placeholder), so apply it as the dechroma
  tonemapped = renodx::color::grade::UserColorGrading(tonemapped, 1.f, 1.f, 1.f, 1.f, 1.f, RENODX_TONE_MAP_BLOWOUT, 0.f);
  return renodx::draw::RenderIntermediatePass(tonemapped);
}

#endif  // SRC_GAMES_RVTHEREYET_COMMON_HLSLI_
