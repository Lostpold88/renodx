#ifndef SRC_GAMES_DAVETHEDIVER_COMMON_HLSLI_
#define SRC_GAMES_DAVETHEDIVER_COMMON_HLSLI_

#include "./shared.h"

static const float MID_GRAY = 0.18f;

// URP uber 2D grading LUT: 32 slices of 32x32 laid out horizontally (1024x32), indexed by saturated linear color.
// lut_params: (1 / lut width, 1 / lut height, lut size - 1), i.e. cb0[136].xyz. The result is linear light.
float3 SampleLut(Texture2D<float4> lut, SamplerState lut_sampler, float3 lut_params, float3 color) {
  const float3 scaled = color * lut_params.z;
  const float slice = floor(scaled.z);
  const float2 uv = float2(scaled.x * lut_params.x + 0.5f * lut_params.x + slice * lut_params.y,
                           scaled.y * lut_params.y + 0.5f * lut_params.y);
  const float3 lower = lut.SampleLevel(lut_sampler, uv, 0).xyz;
  const float3 upper = lut.SampleLevel(lut_sampler, uv + float2(lut_params.y, 0.f), 0).xyz;
  return lerp(lower, upper, scaled.z - slice);
}

float VanillaGrayCurve(Texture2D<float4> lut, SamplerState lut_sampler, float3 lut_params, float gray) {
  return renodx::color::y::from::BT709(SampleLut(lut, lut_sampler, lut_params, saturate(gray).xxx));
}

// untonemapped: linear scene color after exposure and bloom, input of the vanilla grading LUT (before saturate).
// graded_sdr: vanilla LUT result (tonemap + grade), linear.
float3 ToneMapLut(float3 untonemapped, float3 graded_sdr, Texture2D<float4> lut, SamplerState lut_sampler, float3 lut_params) {
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

  float3 tonemapped = renodx::tonemap::psychov::psychotm_test30(
      renodx::draw::ComputeUntonemappedGraded(untonemapped, graded_sdr, neutral_sdr),
      peak_value,
      RENODX_TONE_MAP_EXPOSURE,
      RENODX_TONE_MAP_HIGHLIGHTS,
      RENODX_TONE_MAP_SHADOWS,
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

  // psychotm_test30 has no blowout input (that slot is a placeholder), so apply it as the dechroma
  tonemapped = renodx::color::grade::UserColorGrading(tonemapped, 1.f, 1.f, 1.f, 1.f, 1.f, RENODX_TONE_MAP_BLOWOUT, 0.f);
  return renodx::draw::RenderIntermediatePass(tonemapped);
}

#endif  // SRC_GAMES_DAVETHEDIVER_COMMON_HLSLI_
