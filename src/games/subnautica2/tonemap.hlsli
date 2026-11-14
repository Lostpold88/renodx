#ifndef SRC_GAMES_SUBNAUTICA2_TONEMAP_HLSLI_
#define SRC_GAMES_SUBNAUTICA2_TONEMAP_HLSLI_

// Unreal Engine tonemap: scene color + bloom (+ local exposure) -> log -> 32^3 grading LUT, written as R10G10B10A2 to an
// intermediate that later passes copy to the swapchain. A second target receives the luma for edge detection.
// The game ships variants of this shader that differ in resource slots and in the local exposure; the pieces they share
// live here. Decompiled by the DevKit; same Unreal shader as the Dreadmoor translation apart from the luma output.
#include "./common.hlsli"

cbuffer cb0 : register(b0) {
  float4 cb0[64];
};

cbuffer cb1 : register(b1) {
  float4 cb1[630];
};

float3 SampleBloomWithDirt(
    Texture2D<float4> bloom_texture,
    SamplerState bloom_sampler,
    Texture2D<float4> dirt_texture,
    SamplerState dirt_sampler,
    float2 uv,
    float2 dirt_position) {
  const float2 bloom_uv = clamp(cb0[37].xy * uv + cb0[37].zw, cb0[38].xy, cb0[38].zw);
  const float3 bloom = bloom_texture.Sample(bloom_sampler, bloom_uv).rgb;

  const float2 dirt_ndc = cb0[60].zw * dirt_position + cb0[60].xy;
  const float3 dirt = dirt_texture.Sample(dirt_sampler, float2(dirt_ndc.x * 0.5f + 0.5f, 0.5f - dirt_ndc.y * 0.5f)).rgb;
  return (cb0[57].xyz * dirt + 1.f) * bloom;
}

float ComputeVignette(float2 vignette_position) {
  if (cb0[59].x <= 0.f) return 1.f;

  const float2 scaled = cb0[59].x * vignette_position;
  const float falloff = 1.f / (dot(scaled, scaled) + 1.f);
  return falloff * falloff;
}

// untonemapped: linear scene color after exposure and bloom, input of the vanilla grading LUT.
// luma: Rec.601 luma of the vanilla LUT result. It stays vanilla so edge detection behaves the same with HDR.
float4 GradeWithLut(
    float3 untonemapped,
    float4 noise_seed,
    Texture3D<float4> lut,
    SamplerState lut_sampler,
    out float luma) {
  const float3 lut_input = saturate(log2(untonemapped + LUT_LOG_OFFSET) * LUT_LOG_SCALE + LUT_LOG_BIAS);
  const float3 lut_coord = lut_input * cb0[62].z + cb0[62].w;
  float3 color = lut.Sample(lut_sampler, lut_coord).rgb * LUT_OUTPUT_SCALE;
  luma = dot(color, float3(0.299f, 0.587f, 0.114f));

  if (RENODX_TONE_MAP_TYPE != 0.f) {
    // RenoDX: replace the vanilla clipped LUT output with the HDR tonemap; no dither on the float output
    color = ToneMapLut(untonemapped, color, lut, lut_sampler, cb0[62].zw);
  } else {
    const float noise = frac(sin(noise_seed.w * 543.31f + noise_seed.z) * 493013.f) * 2.f - 1.f;
    color += (sign(noise) - sign(noise) * sqrt(saturate(1.f - abs(noise)))) * cb0[63].y;
  }

  return float4(color, 0.f);
}

#endif  // SRC_GAMES_SUBNAUTICA2_TONEMAP_HLSLI_
