#ifndef SRC_GAMES_WTTG3_COMMON_HLSLI_
#define SRC_GAMES_WTTG3_COMMON_HLSLI_

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

// Scales an intermediate-encoded color by a linear factor.
float3 ScaleEncoded(float3 encoded_color, float scale) {
  return renodx::draw::EncodeColor(renodx::draw::DecodeColor(encoded_color, RENODX_INTERMEDIATE_ENCODING) * scale, RENODX_INTERMEDIATE_ENCODING);
}

// The desktop is drawn by the Slate UI shaders, either into the off-screen texture of the in-world monitor (mode 1)
// or fullscreen over the tonemapped scene (mode 2). Fullscreen it ends up scaled with the UI brightness. The in-world
// monitor is scaled relative to the default white, its UI brightness scaling is undone by ScaleSceneOverlayToDefaultWhite.
// The scale is applied in the encoding of the intermediate buffer, so alpha blended windows stay consistent.
float3 ApplyDesktopBrightness(float3 encoded_color) {
  const float reference_nits = ((shader_injection.desktop_active == 1.f) ? renodx::color::bt2408::REFERENCE_WHITE : shader_injection.graphics_white_nits);
  if (shader_injection.desktop_active == 0.f || reference_nits <= 0.f) return encoded_color;
  return ScaleEncoded(encoded_color, shader_injection.desktop_white_nits / reference_nits);
}

// The overlay pass blends the unexposed scene color (including the in-world monitor) over the tonemapped scene
// without tonemapping, so it gets scaled with the UI brightness like UI. Keep it at the default white instead.
float3 ScaleSceneOverlayToDefaultWhite(float3 encoded_color) {
  if (RENODX_TONE_MAP_TYPE == 0.f || shader_injection.graphics_white_nits <= 0.f) return encoded_color;
  return ScaleEncoded(encoded_color, renodx::color::bt2408::REFERENCE_WHITE / shader_injection.graphics_white_nits);
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

#endif  // SRC_GAMES_WTTG3_COMMON_HLSLI_
