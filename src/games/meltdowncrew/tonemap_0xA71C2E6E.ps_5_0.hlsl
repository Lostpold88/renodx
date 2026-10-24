// Unreal Engine tonemap variant of 0xEB865AA4 with chromatic aberration, sharpen and luminance masked tint before the grading LUT.
// Decompiled from the original shader with the DevKit.
#include "./common.hlsli"

StructuredBuffer<float4> t0 : register(t0);  // eye adaptation
Texture2D<float4> t1 : register(t1);         // scene color
Texture2D<float4> t2 : register(t2);         // bloom
StructuredBuffer<float4> t3 : register(t3);  // scene color scale
Texture3D<float4> t4 : register(t4);         // local exposure grid
Texture2D<float4> t5 : register(t5);         // local exposure
Texture2D<float4> t6 : register(t6);         // tint texture
StructuredBuffer<float4> t7 : register(t7);  // tint color
Texture2D<float4> t8 : register(t8);         // lens dirt
Texture3D<float4> t9 : register(t9);         // grading LUT

SamplerState s0 : register(s0);
SamplerState s1 : register(s1);
SamplerState s2 : register(s2);
SamplerState s3 : register(s3);
SamplerState s4 : register(s4);
SamplerState s5 : register(s5);
SamplerState s6 : register(s6);

cbuffer cb0 : register(b0) {
  float4 cb0[64];
};

cbuffer cb1 : register(b1) {
  float4 cb1[157];
};

float4 main(
    linear noperspective float2 uv : TEXCOORD0,
    linear noperspective float2 vignette_position : TEXCOORD1,
    linear noperspective float4 noise_seed : TEXCOORD2,
    linear noperspective float2 dirt_position : TEXCOORD3,
    linear noperspective float2 exposure_uv : TEXCOORD4,
    float4 position : SV_Position) : SV_Target0 {
  const float pre_exposure = cb1[156].w;
  const float4 eye_adaptation = t0[0];

  // Chromatic aberration: red and green are sampled at ndc positions pushed away from the center, blue stays in place
  const float2 dirt_ndc = cb0[60].zw * dirt_position + cb0[60].xy;
  const float2 aberration_push = sign(dirt_ndc) * saturate(abs(dirt_ndc) - cb0[58].z);
  const float4 aberration_ndc = dirt_ndc.xyxy + aberration_push.xyxy * cb0[58].xxyy;
  const float4 aberration_uv = clamp(
      ((aberration_ndc * cb0[61].zwzw + cb0[61].xyxy) * cb0[11].xyxy + cb0[11].zwzw) * cb0[10].zwzw,
      cb0[16].xyxy, cb0[16].zwzw);
  const float2 scene_uv = clamp(uv, cb0[16].xy, cb0[16].zw);
  float3 scene = float3(
      t1.Sample(s0, aberration_uv.xy).x,
      t1.Sample(s0, aberration_uv.zw).y,
      t1.Sample(s0, scene_uv).z);

  const float2 bloom_uv = clamp(cb0[37].xy * uv + cb0[37].zw, cb0[38].xy, cb0[38].zw);
  const float3 bloom = t2.Sample(s1, bloom_uv).rgb;

  const float3 dirt = t8.Sample(s5, float2(dirt_ndc.x * 0.5f + 0.5f, 0.5f - dirt_ndc.y * 0.5f)).rgb;
  const float3 bloom_dirt = (dirt * cb0[57].xyz + 1.f) * bloom;

  // Local exposure
  const float log_luma = log2(max(dot(scene * pre_exposure, cb0[28].xyz), cb0[26].w));
  const float log_target = log2(eye_adaptation.w * 0.18f * cb0[34].x);
  const float3 grid_uv = float3(cb0[34].zw * exposure_uv, (cb0[26].y * log_luma + cb0[26].z) * 0.96875f + 0.015625f);
  const float2 grid = t4.Sample(s2, grid_uv).xy;
  const float local_luma = t5.Sample(s3, exposure_uv).x;
  const float local_log = ((grid.y < 0.001f) ? local_luma : (grid.x / grid.y));
  const float log_exposure = log2(eye_adaptation.x);
  const float log_local_luma = local_log + (local_luma - local_log) * cb0[33].w + log_exposure;
  const float log_scene = log_exposure + log_luma;
  const float error_to_target = log_local_luma - log_target;

  // Soft knee on the distance between the local exposure and the target, separate for brightening and darkening
  const bool brightening = error_to_target > 0.f;
  const float knee_start = (brightening ? cb0[35].x : cb0[35].y);
  const float knee_range = (brightening ? cb0[35].z : cb0[35].w);
  const float knee_offset = (brightening ? max(0.f, error_to_target - knee_start) : min(0.f, knee_start + error_to_target));
  const float knee_weight = smoothstep(0.f, 1.f, saturate((abs(error_to_target) - knee_start) * (1.f - knee_range)));
  const float knee_shaped = (1.f - knee_weight) * (error_to_target - knee_offset);
  const float local_exposure = exp2(
      (log_target - log_scene) + (log_scene - log_local_luma) * cb0[33].z + knee_shaped
      + (error_to_target - knee_shaped) * (brightening ? cb0[33].x : cb0[33].y));

  const float exposure = eye_adaptation.x * pre_exposure;

  // Sharpen: laplacian of the horizontal and vertical neighbors, one texel fetched and one from the pixel quad derivatives.
  // The strength fades out where the neighbors differ strongly after exposure.
  const float2 texel_parity = floor(cb0[10].xy * uv) % 2.f * 2.f - 1.f;
  const float3 neighbor_x = t1.Sample(s0, clamp(uv + float2(cb0[10].z * texel_parity.x, 0.f), cb0[16].xy, cb0[16].zw)).rgb;
  const float3 neighbor_y = t1.Sample(s0, clamp(uv + float2(0.f, cb0[10].w * texel_parity.y), cb0[16].xy, cb0[16].zw)).rgb;
  const float3 quad_neighbor_x = scene - ddx_fine(scene) * texel_parity.x;
  const float3 quad_neighbor_y = scene - ddy_fine(scene) * texel_parity.y;
  const float scene_luma = renodx::color::y::from::BT709(scene);
  const float4 luma_difference = scene_luma - float4(
      renodx::color::y::from::BT709(neighbor_x),
      renodx::color::y::from::BT709(neighbor_y),
      scene_luma - ddx_fine(scene_luma) * texel_parity.x,
      scene_luma - ddy_fine(scene_luma) * texel_parity.y);
  const float max_luma_difference = max(max(abs(luma_difference.x), abs(luma_difference.y)), max(abs(luma_difference.z), abs(luma_difference.w)));
  const float sharpen = -cb0[59].y * saturate(1.f - exposure * local_exposure * max_luma_difference);
  scene += (neighbor_x + neighbor_y + quad_neighbor_x + quad_neighbor_y - scene * 4.f) * sharpen;

  float vignette = 1.f;
  if (cb0[59].x > 0.f) {
    const float2 scaled = cb0[59].x * vignette_position;
    const float falloff = 1.f / (dot(scaled, scaled) + 1.f);
    vignette = falloff * falloff;
  }

  float3 untonemapped = (bloom_dirt * (exposure * vignette))
                        + (cb0[56].xyz * scene * t3[0].xyz * (exposure * local_exposure * vignette));

  // Tint texture blended in by luminance: shadows, midtones and highlights each get their own strength
  const float tint_luma = dot(untonemapped, float3(0.0405549854f, 0.73296982f, -0.0315675102f));
  const float shadow_weight = 1.f - smoothstep(0.f, 1.f, saturate(tint_luma / cb0[3].z));
  const float highlight_weight = smoothstep(0.f, 1.f, saturate((tint_luma - cb0[3].w) / (cb0[4].x - cb0[3].w)));
  const float midtone_weight = 1.f - shadow_weight - highlight_weight;
  const float tint_strength = cb0[2].w * shadow_weight + cb0[3].x * midtone_weight + cb0[3].y * highlight_weight;
  const float3 tint = t6.SampleLevel(s4, dirt_position * cb0[6].xy + cb0[6].zw, 0).rgb * t7[0].xyz;
  untonemapped *= lerp(1.f, tint, tint_strength);

  const float3 lut_input = saturate(log2(untonemapped + LUT_LOG_OFFSET) * LUT_LOG_SCALE + LUT_LOG_BIAS);
  const float3 lut_coord = lut_input * cb0[62].z + cb0[62].w;
  float3 color = t9.Sample(s6, lut_coord).rgb * LUT_OUTPUT_SCALE;

  if (RENODX_TONE_MAP_TYPE != 0.f) {
    // RenoDX: replace the vanilla clipped LUT output with the HDR tonemap; no dither on the float output
    color = ToneMapLut(untonemapped, color, t9, s6, cb0[62].zw);
  } else {
    const float noise = frac(sin(noise_seed.w * 543.31f + noise_seed.z) * 493013.f) * 2.f - 1.f;
    color += (sign(noise) - sign(noise) * sqrt(saturate(1.f - abs(noise)))) * cb0[63].y;
  }

  return float4(color, 0.f);
}
