#include "./shared.h"

namespace tlou1 {

static const float MID_GRAY = 0.18f;

// Vanilla LUT input shaper: extends the LUT domain above SDR white
float3 ApplyLUTShaper(float3 encoded_color, float input_scale) {
  const float3 scaled_color = max(encoded_color * input_scale, 0.f);
  return saturate((max(pow(scaled_color, 0.75f) - 1.f, 0.f) + saturate(scaled_color)) / input_scale * 0.5f);
}

// Rational tone curve the game applies instead of the first LUT when that LUT is disabled (cb0[11..13]).
struct FallbackCurve {
  float denominator_x;  // cb0[11].x
  float denominator_y;  // cb0[11].y
  float offset;         // cb0[12].x
  float numerator_x;    // cb0[12].y
  float numerator_y;    // cb0[12].z
  bool enabled;
};

// Luminance the vanilla LUTs produce for a neutral gray scene value. In HDR mode 3 the LUT is the game's tone curve.
// lut_params: x = input scale (cb0[43].x), y/z = LUT scale/bias (cb0[27].yz), w = second LUT strength (cb0[42].w).
float VanillaGrayCurve(Texture3D<float4> lut1, Texture3D<float4> lut2, SamplerState lut_sampler, float4 lut_params, float lut1_enabled,
                       FallbackCurve fallback_curve, bool use_luts, float gray) {
  if (!use_luts) return gray;

  if (fallback_curve.enabled) {
    gray = (fallback_curve.numerator_x * gray + fallback_curve.numerator_y)
               / ((fallback_curve.denominator_x + gray) * gray + fallback_curve.denominator_y)
           + fallback_curve.offset;
  }

  float3 color = renodx::color::srgb::EncodeSafe(float3(gray, gray, gray));
  if (lut1_enabled > 0.5f) {
    const float3 uvw = ApplyLUTShaper(color, lut_params.x) * lut_params.y + lut_params.z;
    color = lut1.SampleLevel(lut_sampler, uvw, 0).xyz;
  }
  if (lut_params.w > 0.f) {
    const float3 uvw = saturate(color) * lut_params.y + lut_params.z;
    color = lerp(color, lut2.SampleLevel(lut_sampler, uvw, 0).xyz, lut_params.w);
  }
  return renodx::color::y::from::BT709(renodx::color::srgb::DecodeSafe(color));
}

// untonemapped: linear scene after exposure, input of the vanilla LUTs.
// graded_sdr_encoded: vanilla LUT result as written by the game (tone curve + grade, sRGB encoded).
// Returns linear BT.709 relative to Game Brightness (1.0 = diffuse white). The BT.2020 hull yields negative channels,
// which the upgraded RGBA16F buffers carry to the output.
float3 ToneMapScene(float3 untonemapped, float3 graded_sdr_encoded, Texture3D<float4> lut1, Texture3D<float4> lut2,
                    SamplerState lut_sampler, float4 lut_params, float lut1_enabled, FallbackCurve fallback_curve, bool use_luts) {
  // Neutral SDR is the vanilla tone curve without the color grade: the LUT gray curve at the pixel luminance.
  // The delta between graded and neutral then isolates the grade, and PsychoV replaces the vanilla curve.
  const float untonemapped_y = renodx::color::y::from::BT709(untonemapped);
  const float3 neutral_sdr = untonemapped
                             * renodx::math::DivideSafe(
                                 VanillaGrayCurve(lut1, lut2, lut_sampler, lut_params, lut1_enabled, fallback_curve, use_luts, untonemapped_y),
                                 untonemapped_y, 1.f);
  // ComputeUntonemappedGraded applies RENODX_COLOR_GRADE_STRENGTH itself
  const float3 graded_sdr = renodx::color::srgb::DecodeSafe(graded_sdr_encoded);

  // Anchor PsychoV to the vanilla curve level and local slope (x * V'(x) / V(x)) at mid gray
  const float vanilla_mid_gray_out = VanillaGrayCurve(lut1, lut2, lut_sampler, lut_params, lut1_enabled, fallback_curve, use_luts, MID_GRAY);
  const float slope = (VanillaGrayCurve(lut1, lut2, lut_sampler, lut_params, lut1_enabled, fallback_curve, use_luts, MID_GRAY * 1.1f)
                       - VanillaGrayCurve(lut1, lut2, lut_sampler, lut_params, lut1_enabled, fallback_curve, use_luts, MID_GRAY * 0.9f))
                      / (MID_GRAY * 0.2f);
  const float vanilla_cone = max(0.01f, MID_GRAY * slope / max(vanilla_mid_gray_out, 1e-5f));

  float peak_value = max(1.f, RENODX_PEAK_WHITE_NITS / RENODX_DIFFUSE_WHITE_NITS);
  if (RENODX_SDR_EOTF_EMULATION == 1.f) {
    peak_value = renodx::color::correct::Gamma(peak_value, true, 2.2f);
  } else if (RENODX_SDR_EOTF_EMULATION == 2.f) {
    peak_value = renodx::color::correct::Gamma(peak_value, true, 2.4f);
  }

  return renodx::tonemap::psychov::psychotm_test30(
      renodx::draw::ComputeUntonemappedGraded(untonemapped, graded_sdr, neutral_sdr),
      peak_value,
      1.f,                                                                       // exposure
      1.f,                                                                       // highlights
      1.f,                                                                       // shadows
      1.f,                                                                       // contrast
      RENODX_TONE_MAP_SATURATION,                                                // saturation
      0.f,                                                                       // placeholder
      100.f,                                                                     // placeholder
      1.f,                                                                       // placeholder
      1.f,                                                                       // placeholder
      1,                                                                         // placeholder
      vanilla_cone * max(0.01f, RENODX_PSYCHOV_CONE_RESPONSE),                   // cone response
      float3(MID_GRAY, MID_GRAY, MID_GRAY),                                      // input anchor
      float3(vanilla_mid_gray_out, vanilla_mid_gray_out, vanilla_mid_gray_out),  // output anchor
      1.f,                                                                       // gamut compression
      (int)RENODX_PSYCHOV_GAMUT_MODE,                                            // gamut hull: 0 = BT.709, 1 = BT.2020
      1.f,                                                                       // placeholder
      1.f);                                                                      // compression
}

namespace post_post_processing {

// Final post-post-processing adjustment. Input and output are extended-range gamma-encoded BT.709.
// Rescales the scene from Game Brightness to UI Brightness, because the HUD is drawn into this buffer afterwards.
float3 ApplyFinalOutput(float3 color) {
  if (RENODX_TONE_MAP_TYPE == 0.f) return color;

  if (RENODX_SDR_EOTF_EMULATION == 1.f) {
    color = renodx::color::gamma::DecodeSafe(color, 2.2f);
  } else if (RENODX_SDR_EOTF_EMULATION == 2.f) {
    color = renodx::color::gamma::DecodeSafe(color, 2.4f);
  } else {
    color = renodx::color::srgb::DecodeSafe(color);
  }

  color *= RENODX_DIFFUSE_WHITE_NITS / RENODX_GRAPHICS_WHITE_NITS;

  if (RENODX_SDR_EOTF_EMULATION == 1.f) {
    return renodx::color::gamma::EncodeSafe(color, 2.2f);
  }
  if (RENODX_SDR_EOTF_EMULATION == 2.f) {
    return renodx::color::gamma::EncodeSafe(color, 2.4f);
  }
  return renodx::color::srgb::EncodeSafe(color);
}

}  // namespace post_post_processing
}  // namespace tlou1
