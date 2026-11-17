#include "./shared.h"

static const float MID_GRAY = 0.18f;

// Vanilla filmic curve of the uber composite (Uncharted 2 shape). Its white point is 1, so 1.0 in gives 1.0 out.
// The game's scene color is sRGB encoded, and so is the curve's output.
float3 VanillaFilmic(float3 x) {
  const float3 numerator = x * (x * 0.15f + 0.05f) + 0.004f;
  const float3 denominator = x * (x * 0.15f + 0.5f) + 0.06f;
  return (numerator / denominator - 0.0666666701f) * 4.53191471f;
}

// Luminance the vanilla filmic curve produces for a neutral gray, as linear light in and out.
float VanillaGrayCurve(float linear_gray) {
  const float3 encoded_gray = renodx::color::srgb::EncodeSafe(linear_gray.xxx);
  return renodx::color::y::from::BT709(renodx::color::srgb::DecodeSafe(saturate(VanillaFilmic(encoded_gray))));
}

// Filmic curve + post filmic additions + 16^3 LUT grade, the end of every uber composite variant.
// scene_color: scene, bloom and lens dirt merged, clamped at 0, sRGB encoded. Input of the vanilla filmic curve.
// post_filmic_add: terms the game adds after the filmic curve and before the LUT (lens dirt glow, gas mask glint).
// uv: screen position for film grain.
// Returns the swap chain color and, in alpha, the luma the game's edge detection reads.
float4 ToneMapAndGrade(float3 scene_color, float3 post_filmic_add, float2 uv, Texture3D<float4> lut, SamplerState lut_sampler) {
  const float3 filmic_color = VanillaFilmic(scene_color) + post_filmic_add;
  const float3 lut_sample = lut.Sample(lut_sampler, filmic_color.zyx).xyz;
  const float3 graded_vanilla = saturate(filmic_color) + lut_sample.zyx * 2.f - 1.f;

  // Stays vanilla so the game's edge detection behaves the same with HDR
  const float alpha = sqrt(max(0.f, dot(graded_vanilla, float3(0.299f, 0.587f, 0.114f))));

  if (RENODX_TONE_MAP_TYPE == 0.f) {
    // The original RGBA8 write clamps
    return float4(saturate(graded_vanilla), alpha);
  }

  // Everything below works in linear light. Neutral SDR is the vanilla filmic curve without LUT and glow.
  // The delta between graded and neutral then isolates the grade, and PsychoV replaces the vanilla curve.
  const float3 untonemapped = renodx::color::srgb::DecodeSafe(scene_color);
  const float3 neutral_sdr = renodx::color::srgb::DecodeSafe(saturate(VanillaFilmic(scene_color)));
  const float3 graded_sdr = renodx::color::srgb::DecodeSafe(saturate(graded_vanilla));

  // Anchor PsychoV to the vanilla curve level and local slope (x * V'(x) / V(x)) at mid gray
  const float vanilla_mid_gray_out = VanillaGrayCurve(MID_GRAY);
  const float vanilla_slope = (VanillaGrayCurve(MID_GRAY * 1.1f) - VanillaGrayCurve(MID_GRAY * 0.9f)) / (MID_GRAY * 0.2f);
  const float vanilla_cone = max(0.01f, MID_GRAY * vanilla_slope / max(vanilla_mid_gray_out, 1e-5f));

  const float3 untonemapped_graded = renodx::draw::ComputeUntonemappedGraded(untonemapped, graded_sdr, neutral_sdr);

  float3 tonemapped = 0.f.xxx;

  const bool input_finite = all((asuint(untonemapped_graded) & 0x7f800000u) != 0x7f800000u);
  const float3 input_lms = mul(renodx::tonemap::psychov::PSYCHO35_BT709_TO_LMS_MAT, untonemapped_graded);
  const float3 anchor_lms = mul(renodx::tonemap::psychov::PSYCHO35_BT709_TO_LMS_MAT, float3(MID_GRAY, MID_GRAY, MID_GRAY));

  // PsychoV-35 conditioning requires finite input, positive physiological Yf, and positive unweighted LMS cone mean.
  if (input_finite
      && (renodx::color::yf::from::LMS(input_lms) > 0.f)
      && (renodx::tonemap::psychov::psycho35_Mean(input_lms / anchor_lms) > 0.f)) {
    float peak_value = max(1.f, RENODX_PEAK_WHITE_NITS / RENODX_DIFFUSE_WHITE_NITS);
    if (RENODX_GAMMA_CORRECTION == renodx::draw::GAMMA_CORRECTION_GAMMA_2_2) {
      peak_value = renodx::color::correct::Gamma(peak_value, true, 2.2f);
    } else if (RENODX_GAMMA_CORRECTION == renodx::draw::GAMMA_CORRECTION_GAMMA_2_4) {
      peak_value = renodx::color::correct::Gamma(peak_value, true, 2.4f);
    }

    tonemapped = renodx::tonemap::psychov::psychotm_test35(
        untonemapped_graded,
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
        vanilla_cone * max(0.01f, RENODX_PSYCHOV_CONE_RESPONSE),
        float3(MID_GRAY, MID_GRAY, MID_GRAY),
        float3(vanilla_mid_gray_out, vanilla_mid_gray_out, vanilla_mid_gray_out),
        1.f,
        (int)RENODX_PSYCHOV_GAMUT_MODE,
        1.f,
        1.f,
        renodx::tonemap::psychov::PSYCHO35_SOURCE_BOUNDARY_BT709,
        true);  // human_gamut_conditioning: the LUT grade delta can push cones negative, test35 conditions and floors them

    if (any((asuint(tonemapped) & 0x7f800000u) == 0x7f800000u)) {
      tonemapped = 0.f.xxx;
    }

    // psychotm_test35 has no blowout input (that slot is a placeholder), so apply it as the dechroma
    tonemapped = renodx::color::grade::UserColorGrading(tonemapped, 1.f, 1.f, 1.f, 1.f, 1.f, RENODX_TONE_MAP_BLOWOUT, 0.f);
  }

  if (CUSTOM_FILM_GRAIN_STRENGTH > 0.f) {
    tonemapped = renodx::effects::ApplyFilmGrain(tonemapped, uv, CUSTOM_RANDOM, CUSTOM_FILM_GRAIN_STRENGTH * 0.03f);
  }

  return float4(renodx::draw::RenderIntermediatePass(tonemapped), alpha);
}
