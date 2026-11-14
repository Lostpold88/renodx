#include "./tonemap.hlsli"

// Variant without local exposure (in-game)
StructuredBuffer<float4> t0 : register(t0);  // eye adaptation
Texture2D<float4> t1 : register(t1);         // scene color
Texture2D<float4> t2 : register(t2);         // bloom
StructuredBuffer<float4> t3 : register(t3);  // scene color scale
Texture2D<float4> t4 : register(t4);         // lens dirt
Texture3D<float4> t5 : register(t5);         // grading LUT

SamplerState s0 : register(s0);
SamplerState s1 : register(s1);
SamplerState s2 : register(s2);
SamplerState s3 : register(s3);

float4 main(
    noperspective float2 uv : TEXCOORD0,
    noperspective float2 vignette_position : TEXCOORD1,
    noperspective float4 noise_seed : TEXCOORD2,
    noperspective float2 dirt_position : TEXCOORD3,
    noperspective float4 position : SV_Position,
    out float luma : SV_Target1) : SV_Target0 {
  const float3 scene = t1.Sample(s0, clamp(uv, cb0[16].xy, cb0[16].zw)).rgb;
  const float3 bloom_dirt = SampleBloomWithDirt(t2, s1, t4, s2, uv, dirt_position);

  const float exposure = t0[0].x * cb1[156].w * ComputeVignette(vignette_position);
  const float3 untonemapped = (bloom_dirt * exposure) + (cb0[56].xyz * scene * t3[0].xyz * exposure);

  return GradeWithLut(untonemapped, noise_seed, t5, s3, luma);
}
