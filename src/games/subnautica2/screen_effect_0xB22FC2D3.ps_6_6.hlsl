#include "./common.hlsli"

// Unreal Engine post process material after the tonemap: six flickering spots over a barrel warped screen with a
// rotating pattern, faded to a tint. It runs every frame, so its final clip limited the whole image to sRGB.
// Decompiled by the DevKit.

Texture2D<float4> t0 : register(t0);  // pattern offsets
Texture2D<float4> t1 : register(t1);  // pattern
Texture2D<float4> t2 : register(t2);  // tonemapped scene

SamplerState s0 : register(s0);
SamplerState s1 : register(s1);
SamplerState s2 : register(s2);

cbuffer cb0 : register(b0) {
  float4 cb0[6];
};

cbuffer cb1 : register(b1) {
  float4 cb1[630];
};

cbuffer cb2 : register(b2) {
  float4 cb2[8];
};

static const float EPSILON = 9.999999747378752e-06f;

bool IsInsideFalloff(float value) {
  return (value >= 0.f) && (abs(value) > EPSILON);
}

// Soft ring around a spot, wide variant
float WideSpotFalloff(float distance_to_spot) {
  const float edge = 1.f - (distance_to_spot * 1.25f);
  if (!IsInsideFalloff(edge)) return 0.f;
  return 1.f - (1.f / exp2((1.009886622428894f - (distance_to_spot * 1.2623581886291504f)) * (0.699999988079071f - (distance_to_spot * 0.875f))));
}

// Soft ring around a spot, narrow variant
float NarrowSpotFalloff(float distance_to_spot) {
  const float edge = 1.f - (distance_to_spot * 3.3333332538604736f);
  if (!IsInsideFalloff(edge)) return 0.f;
  return 1.f - (1.f / exp2((1.442695140838623f - (distance_to_spot * 4.80898380279541f)) * edge));
}

float SpotGlow(float falloff, float boost) {
  return PowZero(falloff, 5.f) * boost + falloff;
}

float Flicker(float time, float frequency_a, float frequency_b) {
  return min(max(frac(abs(sin(time * frequency_a))), frac(abs(sin(time * frequency_b)))), 0.20000000298023224f);
}

float4 main(noperspective float4 position : SV_Position, linear float4 texcoord : TEXCOORD) : SV_Target {
  const float time = cb1[163].z;
  const float2 pixel = position.xy - float2(asuint(cb0[5].xy));
  const float2 screen_uv = cb0[5].zw * pixel;
  const float4 scene = t2.Sample(s2, screen_uv * cb0[0].zw + cb0[0].xy);

  // Vignette that shrinks the scene
  const float centered_x = screen_uv.x - 0.5f;
  const float vignette_y = ((7.f - (cb2[0].x * 4.f)) * screen_uv.y) - (3.5f - (cb2[0].x * 2.f));
  const float vignette_edge = 1.f - (sqrt((vignette_y * vignette_y) + (centered_x * centered_x)) / (cb2[0].x * 5.f));
  const float vignette_mask = saturate(IsInsideFalloff(vignette_edge) ? (1.f - (1.f / exp2((vignette_edge * vignette_edge) * 5.770780563354492f))) : 0.f);
  const float3 vignetted = vignette_mask * scene.rgb;

  // Spots on a barrel warped screen
  const float warp = dot(screen_uv, 1.f - screen_uv) * 0.20000000298023224f;
  const float2 warped_uv = (warp * screen_uv) + screen_uv;
  const float2 inverse_view_size = 1.f / cb1[149].xy;
  const float spot_glow = saturate(
      (Flicker(time, 3.3049557209014893f, 11.302947998046875f) * SpotGlow(NarrowSpotFalloff(length(warped_uv - cb2[1].xy * inverse_view_size)), 5.f))
      + (Flicker(time, 3.631681442260742f, 10.89504337310791f) * SpotGlow(WideSpotFalloff(length(warped_uv - cb2[0].zw * inverse_view_size)), 2.f))
      + (Flicker(time, 4.033804893493652f, 12.90817642211914f) * SpotGlow(WideSpotFalloff(length(warped_uv - cb2[1].zw * inverse_view_size)), 2.f))
      + (Flicker(time, 3.769911289215088f, 12.893096923828125f) * SpotGlow(NarrowSpotFalloff(length(warped_uv - cb2[2].xy * inverse_view_size)), 5.f))
      + (Flicker(time, 4.146902561187744f, 14.182406425476074f) * SpotGlow(WideSpotFalloff(length(warped_uv - cb2[2].zw * inverse_view_size)), 2.f))
      + (Flicker(time, 3.3929202556610107f, 11.603787422180176f) * SpotGlow(NarrowSpotFalloff(length(warped_uv - cb2[3].xy * inverse_view_size)), 5.f)));

  // Rotating pattern around the screen center
  const float centered_y = screen_uv.y - 0.5f;
  const float angle = atan2(centered_x, centered_y);
  const float radius = sqrt((centered_x * centered_x) + (centered_y * centered_y));
  const float radius_offset = radius + 1.f;
  const float2 pattern_uv_a = float2((radius_offset - (time * 0.029999999329447746f)) * 2.f, angle * 0.6666666865348816f);
  const float scroll = time * 0.20000000298023224f;
  const float phase_a = frac(scroll - 0.5f);
  const float4 offsets = t0.Sample(s0, pattern_uv_a);
  const float4 pattern_a0 = t1.Sample(s1, float2((pattern_uv_a.x + 0.5f) + (phase_a * -offsets.x), (pattern_uv_a.y + 0.5f) + (offsets.y * phase_a)));
  const float phase_b = frac(scroll);
  const float4 pattern_a1 = t1.Sample(s1, float2((phase_b * -offsets.x) + pattern_uv_a.x, (phase_b * offsets.y) + pattern_uv_a.y));
  const float phase_blend_input = phase_b * 2.f;
  const float phase_blend = ((floor(phase_blend_input) * 2.f) * ((phase_b * -2.f) + 1.f)) + phase_blend_input;
  const float pattern_a = saturate((((phase_blend * (pattern_a1.x - pattern_a0.x)) + pattern_a0.x) * 2.799999952316284f) - 0.8999999761581421f);
  const float2 pattern_uv_b = float2((radius_offset - (time * 0.10000000149011612f)) * 0.5f, angle * 0.1666666716337204f);
  const float2 pattern_step = float2(offsets.x * -0.5f, offsets.y * 0.5f);
  const float4 pattern_b0 = t1.Sample(s1, float2(((pattern_step.x * phase_a) + 0.5f) + pattern_uv_b.x, ((pattern_step.y * phase_a) + 0.5f) + pattern_uv_b.y));
  const float4 pattern_b1 = t1.Sample(s1, float2(pattern_uv_b.x + (phase_b * pattern_step.x), pattern_uv_b.y + (phase_b * pattern_step.y)));
  const float pattern_b = saturate((lerp(pattern_b0.y, pattern_b1.y, phase_blend) * 2.5999999046325684f) - 0.800000011920929f);
  const float pattern_mask = max((pattern_a * pattern_a) * 0.5f, (pattern_b <= 2.980232949312267e-08f) ? 0.f : (pattern_b * pattern_b));

  const float radial_edge = 1.f - (radius * 2.f);
  const float side_ramp = saturate(abs(centered_x) * 5.f);
  const float radial_falloff = (((side_ramp * side_ramp) * (3.f - (side_ramp * 2.f))) + 0.07500000298023224f)
                               * (IsInsideFalloff(radial_edge) ? (1.f / exp2((3.3658077716827393f - (radius * 6.7316155433654785f)) * (2.3329999446868896f - (radius * 4.665999889373779f)))) : 1.f);

  const float3 glow_scale = float3(5.428802490234375f, 7.0777506828308105f, 8.59375f);
  const float3 glow_base = float3(0.2492769956588745f, 0.6364673376083374f, 1.f);
  const float3 color = (((cb2[0].y * (scene.rgb - vignetted)) + vignetted))
                       + (cb2[3].z * (saturate(((spot_glow * glow_scale) * pattern_mask) * radial_falloff) + (spot_glow * glow_base)));
  float3 result = ((cb2[4].xyz - color) * cb2[3].w) + color;

  // RenoDX: negative channels carry colors outside BT.709, so only clip them for vanilla
  if (RENODX_TONE_MAP_TYPE == 0.f) {
    result = max(result, 0.f);
  }
  return float4(result, scene.w);
}
