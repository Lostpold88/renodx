// Tonemap variant with chromatic aberration: red and green sample the scene color at UVs pushed away from the
// screen center, blue samples it unshifted.
#include "./tonemap.hlsli"

float4 main(
    noperspective float2 uv : TEXCOORD0,
    noperspective float2 vignette_position : TEXCOORD1,
    noperspective float4 noise_seed : TEXCOORD2,
    noperspective float2 dirt_position : TEXCOORD3,
    noperspective float2 exposure_uv : TEXCOORD4,
    noperspective float4 position : SV_Position) : SV_Target {
  const float2 dirt_ndc = cb0[60].zw * dirt_position + cb0[60].xy;
  const float2 dead_zone_excess = saturate(abs(dirt_ndc) - cb0[58].z);

  const float2 red_ndc = dirt_ndc - dead_zone_excess * cb0[58].x * sign(dirt_ndc);
  const float2 red_uv = clamp(((red_ndc * cb0[61].zw + cb0[61].xy) * cb0[11].xy + cb0[11].zw) * cb0[10].zw, cb0[16].xy, cb0[16].zw);
  const float2 green_ndc = dirt_ndc - dead_zone_excess * cb0[58].y * sign(dirt_ndc);
  const float2 green_uv = clamp(((green_ndc * cb0[61].zw + cb0[61].xy) * cb0[11].xy + cb0[11].zw) * cb0[10].zw, cb0[16].xy, cb0[16].zw);

  const float3 scene = float3(
      t1.Sample(s0, red_uv).r,
      t1.Sample(s0, green_uv).g,
      t1.Sample(s0, clamp(uv, cb0[16].xy, cb0[16].zw)).b);
  return Tonemap(scene, uv, vignette_position, noise_seed, dirt_position, exposure_uv);
}
