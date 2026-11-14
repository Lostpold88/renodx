#include "./tonemap.hlsli"

float4 main(
    noperspective float2 uv : TEXCOORD0,
    noperspective float2 vignette_position : TEXCOORD1,
    noperspective float4 noise_seed : TEXCOORD2,
    noperspective float2 dirt_position : TEXCOORD3,
    noperspective float2 exposure_uv : TEXCOORD4,
    noperspective float4 position : SV_Position) : SV_Target {
  const float3 scene = t1.Sample(s0, clamp(uv, cb0[16].xy, cb0[16].zw)).rgb;
  return Tonemap(scene, uv, vignette_position, noise_seed, dirt_position, exposure_uv);
}
