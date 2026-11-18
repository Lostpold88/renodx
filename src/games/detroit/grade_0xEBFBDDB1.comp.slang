#version 450
// Scene grade + soft shoulder + sRGB OETF, native input is linear HDR (RGBA16F) and output is the gamma-encoded UI canvas.
// CUSTOM: Soft shoulder and OETF are replaced by PsychoV-35 when the tone mapper is not Vanilla.
#include "./shared.h"

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

struct sCB {
  float4 Lift;
  float4 Gamma;
  float4 Gain;
  float4 Shadow;
  float4 Midtone;
  float4 Highlight;
  float4 Offset;
  float LowRange;
  float HighRange;
  float Contrast;
  float Pivot;
  float Saturation;
  float Hue;
  int ColorSpace;
  float Dummy0;
};

struct POSTPROCESSING_VIEWPORT_COMPOSITING_CONSTANT_BUFFER {
  int4 _viScaleNeo4k;
  float4 _vfInvViewportSize;
  int4 _viViewportOffsetSize;
  int4 _viMireSrgb;
  float4 _vfGrainSize;
  float4 _vfGrainIrregularityU;
  float4 _vfGrainIrregularityV;
  float4 _vfGrainIntensity;
  float4 _vfGrainBlack;
};

layout(set = 0, binding = 53, std140) uniform dyn_CONSTANT_BUFFER_BLOCK_ColorGrading_CB_INDEX {
  sCB ColorGrading;
}
_372;

layout(set = 0, binding = 52, std140) uniform dyn_CONSTANT_BUFFER_BLOCK_0 {
  POSTPROCESSING_VIEWPORT_COMPOSITING_CONSTANT_BUFFER g_cbComposite;
}
_541;

layout(set = 0, binding = 4) uniform sampler2D noise_texture;
layout(set = 0, binding = 1) uniform sampler2D grading_lut_r;
layout(set = 0, binding = 2) uniform sampler2D grading_lut_g;
layout(set = 0, binding = 3) uniform sampler2D grading_lut_b;
layout(set = 0, binding = 0) uniform sampler2D scene_image;
layout(set = 0, binding = 16, rgba16f) uniform writeonly image2D OutputTexture;

static const float3 BT709_LUMA = float3(0.2125999927520751953125, 0.715200006961822509765625, 0.072200000286102294921875);
static const float kBarShadesA[4] = {0.847058832645416259765625, 0.89803922176361083984375, 0.949019610881805419921875, 1.0};
static const float kBarShadesC[4] = {0.0, 0.0470588244497776031494140625, 0.098039217293262481689453125, 0.14901961386203765869140625};

float Shoulder(float x) {
  if (x <= 0.5) return x;
  if (x >= 1.89999997615814208984375) return (0.044551424682140350341796875 * x) + 0.84530413150787353515625;
  return 0.949999988079071044921875 - (0.449999988079071044921875 * exp((0.5 - x) * 2.22222232818603515625));
}

bool InRect(int2 texel, int2 origin, int2 size) {
  return all(texel >= origin) && all(texel <= (origin + size));
}

void main() {
  const int2 texel = int2(gl_GlobalInvocationID.xy);
  const int4 viewport = _541.g_cbComposite._viViewportOffsetSize;
  if (texel.x >= viewport.z || texel.y >= viewport.w) return;

  const int4 mire = _541.g_cbComposite._viMireSrgb;
  float3 color = texelFetch(scene_image, texel, 0).xyz;
  if (mire.y == 1) {
    color = renodx::color::srgb::DecodeSafe(color);
  }

  if (mire.x == 1) {
    color = float3(float(texel.x) / float(viewport.z));
  } else if (mire.x == 2) {
    color = float3(float(int((float(texel.x) / (float(viewport.z) * 0.975609719753265380859375)) * 40.0)) * 0.02500000037252902984619140625);
  } else {
    const float2 grain_position = float2(texel) * _541.g_cbComposite._vfGrainIrregularityU.w;
    const float3 grain_noise = float3(
        textureLod(noise_texture, (grain_position / _541.g_cbComposite._vfGrainSize.x) + float2(_541.g_cbComposite._vfGrainIrregularityU.x, _541.g_cbComposite._vfGrainIrregularityV.x), 0.0).x,
        textureLod(noise_texture, (grain_position / _541.g_cbComposite._vfGrainSize.y) + float2(_541.g_cbComposite._vfGrainIrregularityU.y, _541.g_cbComposite._vfGrainIrregularityV.y), 0.0).x,
        textureLod(noise_texture, (grain_position / _541.g_cbComposite._vfGrainSize.z) + float2(_541.g_cbComposite._vfGrainIrregularityU.z, _541.g_cbComposite._vfGrainIrregularityV.z), 0.0).x);
    color = max(color + ((_541.g_cbComposite._vfGrainIntensity.w * ((2.0 * grain_noise) - 1.0)) * (color * (_541.g_cbComposite._vfGrainIntensity.xyz + _541.g_cbComposite._vfGrainBlack.xyz))), float3(0.0));
  }

  // Per-channel 1D grade in log space, nearly linear gains with a mild toe.
  const bool luma_only = _372.ColorGrading.ColorSpace == 2;
  const float luma = dot(color, BT709_LUMA);
  const float3 lut_coord = log(((luma_only ? float3(luma) : color) * 10.0) + float3(1.0)) * 0.1447438895702362060546875;
  const float3 lut_sample = float3(
      textureLod(grading_lut_r, float2(lut_coord.x, 0.0), 0.0).x,
      textureLod(grading_lut_g, float2(lut_coord.y, 0.0), 0.0).x,
      textureLod(grading_lut_b, float2(lut_coord.z, 0.0), 0.0).x);
  float3 graded = luma_only ? ((color * lut_sample.x) / float3(luma)) : lut_sample;

  if (_372.ColorGrading.ColorSpace != 3) {
    const float3 grade_luma = float3(dot(graded, BT709_LUMA));
    graded = grade_luma + (float3(_372.ColorGrading.Saturation) * (graded - grade_luma));
    if (_372.ColorGrading.Hue != 0.0) {
      const float4 k = lerp(float4(graded.b, graded.g, -1.0, 0.666666686534881591796875), float4(graded.g, graded.b, 0.0, -0.3333333432674407958984375), float4(step(graded.b, graded.g)));
      const float4 q = lerp(float4(k.x, k.y, k.w, graded.r), float4(graded.r, k.y, k.z, k.x), float4(step(k.x, graded.r)));
      const float chroma = q.x - min(q.w, q.y);
      const float hue = abs(q.z + ((q.w - q.y) / ((6.0 * chroma) + 1.0000000133514319600180897396058e-10))) + _372.ColorGrading.Hue;
      const float saturation = chroma / (q.x + 1.0000000133514319600180897396058e-10);
      graded = lerp(float3(1.0), clamp(abs((frac(float3(hue) + float3(1.0, 0.666666686534881591796875, 0.3333333432674407958984375)) * 6.0) - float3(3.0)) - float3(1.0), float3(0.0), float3(1.0)), float3(saturation)) * q.x;
    }
  }

  // CUSTOM: Native look is scaled in from the ungraded scene, 1.0 is the original grade.
  const bool use_psychov = (RENODX_TONE_MAP_TYPE == RENODX_TONE_MAP_TYPE_PSYCHOV35) && (mire.x == 0);
  float3 mapped = max(graded, float3(0.0));
  if (use_psychov) {
    mapped = lerp(color, mapped, RENODX_COLOR_GRADE_STRENGTH);
  }

  float3 output_color;
  if (use_psychov) {
    float peak_value = RENODX_PEAK_WHITE_NITS / RENODX_DIFFUSE_WHITE_NITS;
    if (RENODX_GAMMA_CORRECTION == 1.f) peak_value = renodx::color::correct::GammaSafe(peak_value, true, 2.2f);
    if (RENODX_GAMMA_CORRECTION == 2.f) peak_value = renodx::color::correct::GammaSafe(peak_value, true, 2.4f);
    peak_value = max(1.f, peak_value);

    // The native shoulder is the identity below 0.5, so mid gray maps to itself with unit slope.
    const float mid_gray = 0.18f;
    const float3 untonemapped = mapped;
    mapped = float3(0.f);
    const bool input_finite = all((asuint(untonemapped) & 0x7f800000u) != 0x7f800000u);
    const float3 input_lms = mul(renodx::tonemap::psychov::PSYCHO35_BT709_TO_LMS_MAT, untonemapped);
    const float3 anchor_lms = mul(renodx::tonemap::psychov::PSYCHO35_BT709_TO_LMS_MAT, float3(mid_gray));

    // PsychoV-35 conditioning requires finite input, positive physiological Yf, and positive unweighted LMS cone mean.
    if (input_finite
        && (renodx::color::yf::from::LMS(input_lms) > 0.f)
        && (renodx::tonemap::psychov::psycho35_Mean(input_lms / anchor_lms) > 0.f)) {
      mapped = renodx::tonemap::psychov::psychotm_test35(
          untonemapped,
          peak_value,
          RENODX_TONE_MAP_EXPOSURE,
          RENODX_TONE_MAP_HIGHLIGHTS,
          RENODX_TONE_MAP_SHADOWS,
          RENODX_TONE_MAP_CONTRAST,
          RENODX_TONE_MAP_SATURATION,
          1.f,
          100.f,
          1.f,
          1.f,
          0,
          max(0.01f, RENODX_PSYCHOV_CONE_RESPONSE),
          mid_gray,
          mid_gray,
          1.f,
          1,  // Gamut hull: always BT.2020
          1.f,
          0.f,
          renodx::tonemap::psychov::PSYCHO35_SOURCE_BOUNDARY_BT709,
          true);

      if (any((asuint(mapped) & 0x7f800000u) == 0x7f800000u)) {
        mapped = float3(0.f);
      }

      mapped = renodx::color::grade::UserColorGrading(mapped, 1.f, 1.f, 1.f, 1.f, 1.f, RENODX_TONE_MAP_BLOWOUT, 0.f);
    }

    // CUSTOM: The game is authored in BT.709. Saturated colors are pushed toward the BT.2020 gamut at constant
    // luminance, 0 keeps the game's colors. Components below zero are BT.709 coordinates outside sRGB.
    if (CUSTOM_GAMUT_EXPANSION > 0.f) {
      const float3 bt2020 = renodx::color::bt2020::from::BT709(mapped);
      const float luminance_bt2020 = dot(bt2020, float3(0.2627002120112671f, 0.6779980715188708f, 0.0593017164698620f));
      const float source_saturation = saturate(1.f - (renodx::math::Min(mapped) / max(renodx::math::Max(mapped), 1e-6f)));
      mapped = renodx::color::bt709::from::BT2020(
          luminance_bt2020 + ((bt2020 - luminance_bt2020) * (1.f + (CUSTOM_GAMUT_EXPANSION * source_saturation))));
    }

    output_color = renodx::draw::RenderIntermediatePass(mapped);
  } else {
    if (mire.z == 1) {
      mapped = float3(Shoulder(mapped.x), Shoulder(mapped.y), Shoulder(mapped.z));
    }
    const bool apply_oetf = (_372.ColorGrading.ColorSpace == 0) || luma_only;
    output_color = apply_oetf ? renodx::color::srgb::EncodeSafe(mapped) : mapped;
  }

  if (mire.x == 3) {
    // Native calibration chart (brightness menu), drawn over the encoded image.
    const int box = viewport.z / 5;
    const int half_box = box / 2;
    const int quarter_box = box / 4;
    const int2 checker_origin = int2(2 * box, box);

    if (InRect(texel, checker_origin, int2(box))) {
      output_color = float3(((((texel.x / 2) + (texel.y / 2)) & 1) == 0) ? 1.0 : 0.0);
    } else {
      for (int i = 0; i < 4; i++) {
        if (InRect(texel, int2(box + (i * quarter_box), box), int2(quarter_box, box))) {
          output_color = float3(kBarShadesA[i]);
        }
      }
    }

    if (InRect(texel, checker_origin + int2(half_box / 2), int2(half_box))) {
      output_color = float3(0.729411780834197998046875);
    }
    for (int i = 0; i < 4; i++) {
      if (InRect(texel, int2((3 * box) + (i * quarter_box), box), int2(quarter_box, box))) {
        output_color = float3(kBarShadesC[i]);
      }
    }
  }

  imageStore(OutputTexture, texel + viewport.xy, float4(output_color, 1.0));
}
