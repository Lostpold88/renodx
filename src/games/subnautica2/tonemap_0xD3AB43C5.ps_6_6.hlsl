#include "./tonemap.hlsli"

// Variant with local exposure and lens dirt (main menu)
StructuredBuffer<float4> t0 : register(t0);  // eye adaptation
Texture2D<float4> t1 : register(t1);         // scene color
Texture2D<float4> t2 : register(t2);         // bloom
StructuredBuffer<float4> t3 : register(t3);  // scene color scale
Texture3D<float4> t4 : register(t4);         // local exposure grid
Texture2D<float4> t5 : register(t5);         // local exposure
Texture2D<float4> t6 : register(t6);         // lens dirt
Texture3D<float4> t7 : register(t7);         // grading LUT

SamplerState s0 : register(s0);
SamplerState s1 : register(s1);
SamplerState s2 : register(s2);
SamplerState s3 : register(s3);
SamplerState s4 : register(s4);
SamplerState s5 : register(s5);

float4 main(
    noperspective float2 uv : TEXCOORD0,
    noperspective float2 vignette_position : TEXCOORD1,
    noperspective float4 noise_seed : TEXCOORD2,
    noperspective float2 dirt_position : TEXCOORD3,
    noperspective float2 exposure_uv : TEXCOORD4,
    noperspective float4 position : SV_Position,
    out float luma : SV_Target1) : SV_Target0 {
  const float3 scene = t1.Sample(s0, clamp(uv, cb0[16].xy, cb0[16].zw)).rgb;
  const float pre_exposure = cb1[156].w;
  const float4 eye_adaptation = t0[0];
  const float3 bloom_dirt = SampleBloomWithDirt(t2, s1, t6, s4, uv, dirt_position);

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

  const float vignette = ComputeVignette(vignette_position);
  const float exposure = eye_adaptation.x * pre_exposure;
  const float3 untonemapped = (bloom_dirt * (exposure * vignette))
                              + (cb0[56].xyz * scene * t3[0].xyz * (exposure * local_exposure * vignette));

  return GradeWithLut(untonemapped, noise_seed, t7, s5, luma);
}
