#include "./common.hlsli"

// Unreal Engine post process material after the tonemap: refraction ripples of the water surface blended over the scene,
// with a reflection of the sky added to the ripples. Decompiled by the DevKit.

Texture2D<float4> t0 : register(t0);  // sky reflection
Texture2D<float4> t1 : register(t1);  // scene depth
Texture2D<float4> t2 : register(t2);  // ripple noise
Texture2D<float4> t3 : register(t3);  // ripple normals
Texture2D<float4> t4 : register(t4);  // ripple pattern
Texture2D<float4> t5 : register(t5);  // tonemapped scene

SamplerState s0 : register(s0);
SamplerState s1 : register(s1);
SamplerState s2 : register(s2);
SamplerState s3 : register(s3);
SamplerState s4 : register(s4);
SamplerState s5 : register(s5);

cbuffer cb0 : register(b0) {
  float4 cb0[11];
};

cbuffer cb1 : register(b1) {
  float4 cb1[630];
};

cbuffer cb2 : register(b2) {
  float4 cb2[11];
};

cbuffer cb3 : register(b3) {
  float4 cb3[10];
};

// Mirrored repeat of a coordinate
float TriangleWave(float value) {
  return 1.f - abs((frac(value * 0.5f) * 2.f) - 1.f);
}

float4 main(noperspective float4 position : SV_Position, linear float4 texcoord : TEXCOORD) : SV_Target {
  const float time = cb1[163].z;
  const float2 pixel = position.xy - float2(asuint(cb0[10].xy));
  const float2 screen_uv = cb0[10].zw * pixel;

  // View ray through the pixel from the depth buffer
  const float depth = max(
      t1.SampleLevel(
            s1,
            float2(cb1[152].z * (cb1[148].x + cb1[149].x * screen_uv.x), cb1[152].w * (cb1[148].y + cb1[149].y * screen_uv.y)),
            0.f)
          .x,
      1.000000045813705e-18f);
  const float4 clip = float4(position.x * cb1[155].z, position.y * cb1[155].z, depth, 1.f);
  const float clip_w = dot(clip, float4(cb1[44].w, cb1[45].w, cb1[46].w, cb1[47].w));
  const float3 view_position = -float3(
                                   dot(clip, float4(cb1[44].x, cb1[45].x, cb1[46].x, cb1[47].x)),
                                   dot(clip, float4(cb1[44].y, cb1[45].y, cb1[46].y, cb1[47].y)),
                                   dot(clip, float4(cb1[44].z, cb1[45].z, cb1[46].z, cb1[47].z)))
                               / clip_w;
  const bool orthographic = (cb1[31].w >= 1.f);
  const float aspect_x = (cb1[149].x / cb1[149].y) * screen_uv.x;

  // Ripples
  const float4 noise = t2.Sample(s2, float2(aspect_x * 0.699999988079071f + cb3[0].x, cb3[0].x + screen_uv.y * 0.4000000059604645f));
  const float4 normal_a = t3.Sample(
      s3,
      float2((time * cb3[1].x + cb3[0].y * aspect_x) + cb3[1].z, (time * cb3[1].y + cb3[0].z * screen_uv.y) + cb3[1].z));
  const float time_offset = time - cb3[2].x;
  const float fade_fast = 1.f - saturate(abs(saturate(abs(time_offset * 0.8999999761581421f))));
  const float2 warped_uv = (((normal_a.xy - 0.5f) * cb3[1].w) * fade_fast) + float2(aspect_x, screen_uv.y);
  const float noise_scroll = time * 0.15000000596046448f + noise.w;
  const float4 pattern = t4.Sample(s4, float2(warped_uv.x * 2.f + noise_scroll, noise_scroll - warped_uv.y * 0.15000000596046448f));
  const float2 pattern_ripple = (pattern.x * round(noise.z)) * (noise.xy * 6.f - 3.f);
  const float fade_slow = 1.f - saturate(abs(saturate(abs(time_offset * 0.4000000059604645f))));
  const float pattern_exponent = fade_slow * 2.f + 1.f;
  const float4 normal_b = t3.Sample(
      s3,
      float2(
          (time * cb3[3].x + cb3[2].y * warped_uv.x) + cb3[3].z,
          (time * cb3[3].y + cb3[2].z * warped_uv.y) + cb3[3].z));
  const float2 centered = float2(screen_uv.x - 0.5f, (screen_uv.y - 0.5f) * 0.75f);
  const float edge_mask = saturate(PowZero((fade_slow * 0.25f + 0.6000000238418579f) + dot(centered, centered), 12.f));
  const float2 edge_ripple = edge_mask * (normal_b.xy * 2.f - 1.f);
  const float ripple_exponent = 10.f - (fade_fast * 9.25f);
  const float ripple_weight = cb3[4].x * saturate(((cb1[121].z * 2097152.f) + cb1[122].z) + cb2[2].z);
  const float2 ripple_offset = float2(
      (PowZero(edge_ripple.x, ripple_exponent) * fade_fast) * cb3[3].w + (PowZero(pattern_ripple.x, pattern_exponent) * fade_slow),
      (PowZero(edge_ripple.y, ripple_exponent) * fade_fast) * cb3[3].w + (PowZero(pattern_ripple.y, pattern_exponent) * fade_slow));
  const float2 scene_uv = float2(TriangleWave(screen_uv.x + ripple_weight * ripple_offset.x), TriangleWave(screen_uv.y + ripple_weight * ripple_offset.y));
  const float4 scene = t5.Sample(s5, clamp(cb0[0].zw * scene_uv + cb0[0].xy, cb0[1].xy, cb0[1].zw));

  const float ripple_strength = max(abs(edge_ripple.x), abs(edge_ripple.y)) * fade_fast;
  const float ripple_blend = saturate(ripple_strength * 8.f);
  const float2 ripple = (ripple_blend * (edge_ripple - pattern_ripple)) + pattern_ripple;
  const float ripple_lit = dot(ripple.x * cb1[16].xyz + ripple.y * cb1[17].xyz, cb1[182].xyz);

  // Reflection of the sky
  const float3 view_direction = -(orthographic ? -cb1[73].xyz : (rsqrt(dot(view_position, view_position)) * view_position));
  float3 sky = 0.f;
  if (cb1[204].w != 0.f || (asint(cb1[321].x) & 8) != 0) {
    float planet_height = cb1[194].w;
    if (orthographic) {
      const float w = (cb1[47].w + cb1[46].w) + (position.y * cb1[45].w + cb1[44].w * position.x);
      planet_height = cb1[197].z * ((cb1[37].y + (((cb1[47].z + cb1[46].z) + (position.y * cb1[45].z + cb1[44].z * position.x)) / w)) + cb1[194].w)
                      + cb1[197].y * (((cb1[47].y + cb1[46].y) + (position.y * cb1[45].y + cb1[44].y * position.x)) / w)
                      + cb1[197].x * ((((position.y * cb1[45].x + cb1[44].x * position.x) + cb1[46].x) + cb1[47].x) / w);
    }
    const float radius = planet_height * 9.999999747378752e-06f;
    const float3 sky_direction = float3(dot(cb1[195].xyz, view_direction), dot(cb1[196].xyz, view_direction), dot(cb1[197].xyz, view_direction));
    const float horizon_b = sky_direction.z * radius * 2.f;
    const float horizon_c = radius * radius - cb1[200].z * cb1[200].z;
    const float discriminant = (horizon_b * horizon_b) - ((dot(sky_direction, sky_direction) * 4.f) * horizon_c);
    bool hits_planet_near = false;
    bool hits_planet_far = false;
    if (discriminant >= 0.f) {
      const float root = sqrt(discriminant);
      const float denominator = dot(sky_direction, sky_direction) * 2.f;
      hits_planet_near = (((-horizon_b - root) / denominator) > 0.f);
      hits_planet_far = (((root - horizon_b) / denominator) > 0.f);
    }
    const float horizon_angle = acos(sqrt(radius * radius - cb1[200].z * cb1[200].z) / radius);
    const float opposite_angle = 3.1415927410125732f - horizon_angle;
    const float view_angle = acos(sky_direction.z);
    float sky_v = 0.f;
    if (hits_planet_near || hits_planet_far) {
      sky_v = (sqrt((view_angle - opposite_angle) / horizon_angle) * 0.5f) + 0.5f;
    } else {
      sky_v = (1.f - sqrt(1.f - (view_angle / opposite_angle))) * 0.5f;
    }
    const float azimuth = atan2(-sky_direction.y, -sky_direction.x);
    const float4 sky_sample = t0.SampleLevel(
        s0,
        float2(
            ((azimuth + 3.1415927410125732f) * 0.15915493667125702f + (cb1[192].z * 0.5f)) * (cb1[192].x / (cb1[192].x + 1.f)),
            (cb1[192].y / (cb1[192].y + 1.f)) * (sky_v + (cb1[192].w * 0.5f))),
        0.f);
    sky = (cb1[199].xyz * cb1[156].w) * sky_sample.xyz;
  }

  const float ripple_glint = (max(fade_slow * (pattern.x * round(noise.z)), ripple_strength) * ripple_weight) * (sign(ripple_lit) * PowZero(ripple_lit, 4.f));
  const float3 color = ((ripple_glint * max(sky, float3(0.10000000149011612f, 0.20000000298023224f, 0.33000001311302185f))) * cb3[4].y) + scene.rgb;
  float3 result = ((cb3[5].xyz - color) * cb3[4].z) + color;

  // RenoDX: negative channels carry colors outside BT.709, so only clip them for vanilla
  if (RENODX_TONE_MAP_TYPE == 0.f) {
    result = max(result, 0.f);
  }
  return float4(result, scene.w);
}
