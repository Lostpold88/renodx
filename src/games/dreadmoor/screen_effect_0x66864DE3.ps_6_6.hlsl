// Unreal Engine post process material pass after the tonemap: animated screen distortion blended over the tonemapped
// image and tinted, written to the swapchain. Decompiled by the DevKit.
#include "./common.hlsli"

Texture2D<float4> t0 : register(t0);  // distortion noise
Texture2D<float4> t1 : register(t1);  // scrolling distortion noise
Texture2D<float4> t2 : register(t2);  // distortion pattern
Texture2D<float4> t3 : register(t3);  // tonemapped scene

SamplerState s0 : register(s0);
SamplerState s1 : register(s1);
SamplerState s2 : register(s2);
SamplerState s3 : register(s3);

cbuffer cb0 : register(b0) {
  float4 cb0[11];
};

cbuffer cb1 : register(b1) {
  float4 cb1[378];
};

cbuffer cb2 : register(b2) {
  float4 cb2[1];
};

cbuffer cb3 : register(b3) {
  float4 cb3[3];
};

float4 main(
    noperspective float4 position : SV_Position,
    linear float4 texcoord : TEXCOORD) : SV_Target {
  const float time = cb1[163].z;
  const float2 screen_uv = cb0[10].zw * (position.xy - float2(asuint(cb0[10].xy)));
  const float4 scene = t3.Sample(s3, screen_uv * cb0[0].zw + cb0[0].xy);

  const float aspect_ratio = cb1[149].x / cb1[149].y;
  const float centered_x = (aspect_ratio * screen_uv.x + 0.5f) - (aspect_ratio * 0.5f);
  const float2 noise_uv = float2(centered_x, screen_uv.y) * (0.2857142984867096f * cb3[0].y);
  const float4 noise_warp = t0.Sample(s0, noise_uv);
  const float4 noise = t1.Sample(
      s1, noise_uv + float2(frac(time * -0.001f), frac(time * -0.3f)) + noise_warp.xy * float2(0.02f, 0.4f));
  const float noise_level = cb3[0].z * noise.x;
  const float4 pattern = t2.SampleLevel(s2, float2(centered_x * cb3[0].x, cb3[0].x * screen_uv.y), noise_level * 8.f);

  const float pattern_phase = frac(cb3[0].w + pattern.w);
  const float pattern_mask = saturate(
      (cb3[1].x * 3.f)
      * ((pattern.x + noise_level * 1.5f)
         + ((pattern_phase <= 2.980232949312267e-08f) ? -1.f : -(1.f - pow(pattern_phase, 4.f)))));

  const float2 pattern_offset = pattern.yz - 0.5f;
  const float2 offset_direction = lerp(pattern_offset, noise.yz - 0.5f, noise_level * 0.75f);

  const float2 centered_uv = screen_uv - 0.5f;
  const float edge_falloff = 1.f - sqrt(dot(centered_uv, centered_uv)) * 0.7692307829856873f;
  const float edge_strength = lerp(
      cb3[1].z, cb3[1].y,
      saturate(saturate((edge_falloff <= 2.980232949312267e-08f) ? 0.f : pow(edge_falloff, 0.7142857313156128f)) * 1.2000000476837158f));

  const float2 distorted_uv = clamp(
      ((pattern_mask * offset_direction * edge_strength + screen_uv) * cb0[0].zw) + cb0[0].xy,
      cb0[1].xy, cb0[1].zw);
  const float4 distorted = t3.Sample(s3, distorted_uv);

  const bool effect_active = (cb1[121].z * 2097152.f) >= (cb2[0].z - cb1[122].z);
  const float effect_weight = (effect_active ? 1.f : 0.f) * saturate(1.f - ((time - cb2[0].x) / cb2[0].y));
  const float3 effect_color = lerp(scene.rgb, distorted.rgb, effect_weight);

  float3 color = lerp(effect_color, cb3[2].xyz, cb3[1].w);

  // RenoDX: negative channels carry colors outside BT.709, so only clip them for vanilla
  if (RENODX_TONE_MAP_TYPE == 0.f) {
    color = max(color, 0.f);
  }
  return float4(color, saturate(scene.w));
}
