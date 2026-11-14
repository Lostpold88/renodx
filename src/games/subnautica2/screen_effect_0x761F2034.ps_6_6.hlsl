#include "./common.hlsli"

// Unreal Engine post process material after the tonemap: scene color with radial blur, distortion, vignette and
// full screen effects (frost, damage, scanner) driven by material collection weights. Decompiled by the DevKit.
// Most effect branches are inactive during normal play, where the shader only copies the scene and fades it to a tint.

Texture2D<float4> t0 : register(t0);  // dither noise
Texture2D<float4> t1 : register(t1);  // distortion pattern
Texture2D<float4> t2 : register(t2);  // scrolling noise
Texture2D<float4> t3 : register(t3);  // frost pattern
Texture2D<float4> t4 : register(t4);  // frost mask
Texture2D<float4> t5 : register(t5);  // radial pattern
Texture2D<float4> t6 : register(t6);  // tonemapped scene

SamplerState s0 : register(s0);
SamplerState s1 : register(s1);
SamplerState s2 : register(s2);
SamplerState s3 : register(s3);
SamplerState s4 : register(s4);
SamplerState s5 : register(s5);
SamplerState s6 : register(s6);

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
  float4 cb3[3];
};

static const float BLUR_OFFSETS[4] = {-0.2199999988079071f, -0.07999999821186066f, 0.07999999821186066f, 0.2199999988079071f};

float3 SampleSceneClamped(float2 uv) {
  return t6.Sample(s6, clamp(cb0[0].zw * uv + cb0[0].xy, cb0[1].xy, cb0[1].zw)).rgb;
}

float PolarAngleTurns(float2 centered) {
  return atan2(centered.y, centered.x) * 0.1591549515724182f;
}

// Hash of a screen cell and a time seed
float CellNoise(float2 cell, uint time_seed) {
  const uint row = uint(int(floor(cell.y))) * 1664525u + 1013904223u;
  const uint column = (uint(int(floor(cell.x))) * 1664525u + 1013904223u) + (time_seed * row);
  const uint mixed = (column * time_seed) + row;
  return float(((((mixed * column) + time_seed) * mixed) + column) >> 16) * 1.5259021893143654e-05f;
}

float4 main(noperspective float4 position : SV_Position, linear float4 texcoord : TEXCOORD) : SV_Target {
  const float time = cb1[163].z;
  const float2 view_size = cb1[149].xy;
  const float2 pixel = position.xy - float2(asuint(cb0[10].xy));
  const float2 screen_uv = cb0[10].zw * pixel;

  const float4 scene = t6.Sample(s6, screen_uv * cb0[0].zw + cb0[0].xy);

  // Radial blur
  const float blur_weight = cb2[2].w;
  const float blur_noise = t0.Sample(s0, float2(frac(time) * 128.f + pixel.x * 0.0078125f, pixel.y * 0.0078125f)).x;
  const float blur_strength = (blur_weight * 0.16664999723434448f)
                              * (blur_noise + float((((uint(pixel.y + cb1[174].x) << 1) + uint(pixel.x + cb1[174].x)) % 5u)));
  float3 blur_sum = scene.rgb;
  [unroll] for (int i = 0; i < 4; i++) {
    const float2 zoomed_uv = saturate((blur_strength * (0.5f - screen_uv)) * BLUR_OFFSETS[i] + screen_uv);
    blur_sum += t6.Sample(s6, cb0[0].zw * zoomed_uv + cb0[0].xy).rgb;
  }
  const float3 blurred = (blur_weight > 0.f) ? (blur_sum * 0.20000000298023224f) : scene.rgb;

  const float2 centered = screen_uv - 0.5f;
  const float radius = length(centered);

  // Distortion
  const float4 distortion = t1.SampleGrad(
      s1,
      float2((cb3[1].x + cb3[0].x * time) + cb3[0].z * frac(PolarAngleTurns(centered)), (cb3[1].y + cb3[0].y * time) + cb3[0].w * radius),
      ddx_coarse(screen_uv), ddy_coarse(screen_uv), int2(0, 0));
  const float radial_mask = 1.f - saturate(2.f - radius * 2.3529410362243652f);
  const float2 distorted_uv = screen_uv - ((centered * cb2[1].y) * distortion.x * cb3[1].z) * radial_mask;
  const float2 pixel_size = 1.f / view_size;
  const float3 distorted_center = SampleSceneClamped(distorted_uv);
  const float3 ring_samples = SampleSceneClamped(distorted_uv + float2(pixel_size.x * 2.f, 0.f))
                              + SampleSceneClamped(distorted_uv - float2(pixel_size.x * 2.f, 0.f))
                              + SampleSceneClamped(distorted_uv + float2(pixel_size.x, pixel_size.y * 2.f))
                              + SampleSceneClamped(distorted_uv + float2(pixel_size.x, -pixel_size.y * 2.f))
                              + SampleSceneClamped(distorted_uv + float2(-pixel_size.x, pixel_size.y * 2.f))
                              + SampleSceneClamped(distorted_uv + float2(-pixel_size.x, -pixel_size.y * 2.f));
  const float pulse = min(max((sin(time * 1.2566370964050293f) + 1.f) * 0.5f, 0.20000000298023224f), 0.6000000238418579f) * radial_mask;
  const float pulse_scale = 7.f - (abs(sin(frac(time * 0.5555555820465088f) * 6.2831854820251465f)) * 5.f);
  const float distortion_blend = saturate(length(pulse_scale * centered));

  float3 color = blurred;
  if (cb2[1].y > 0.f) {
    color = (distortion_blend * (((distorted_center * 0.30000001192092896f) - blurred) + pulse + (ring_samples * 0.11666999757289886f))) + blurred;
  }

  // Vignette tint on green and blue
  const float vignette_scale = (min(cb2[1].z, 1.f) * 1.899999976158142f) + 0.10000000149011612f;
  const float vignette_radius = saturate(length(vignette_scale * centered));
  const uint time_seed = uint(int(floor(time * 30.f))) * 1664525u + 1013904223u;
  const float vignette_noise = saturate(
      CellNoise(float2(floor(view_size.x * screen_uv.x) * 0.10000000149011612f, floor(view_size.y * screen_uv.y) * 0.5f), time_seed) + 0.10000000149011612f);
  const float vignette_amount = (saturate(PowZero(vignette_radius, 1.7999999523162842f)) * -3.f) * vignette_noise;
  if (cb2[1].z > 0.f) {
    color.g = ((color.g * -0.48333799839019775f) * vignette_amount) + color.g;
    color.b = ((color.b * -0.05733102560043335f) * vignette_amount) + color.b;
  }

  // Frost
  const float2 frost_scroll = (time * 0.5f) + (screen_uv * 2.f);
  const float4 frost_noise = t2.Sample(s2, frost_scroll);
  const float frost_radius_mask = 1.f - saturate(2.499999761581421f - (radius * 4.166666030883789f));
  const float frost_weight = saturate(cb3[1].w + cb2[2].x);
  const float frost_offset = ((frost_noise.x * 0.02500000037252903f) * frost_radius_mask) * frost_weight;
  const float3 frost_scene = SampleSceneClamped(screen_uv + frost_offset);
  const float flicker = saturate(sin(time * 0.6283185482025146f) + sin(time * 1.0053097009658813f));
  const float3 frost_dark = float3(frost_weight * 0.8999999761581421f, ((flicker * 0.2498999983072281f) + 0.1355999857187271f) * frost_weight, frost_weight * 0.017999980598688126f);
  const float frost_ramp = saturate((frost_weight - 0.5f) * 2.f);
  const float frost_blend = (frost_ramp * frost_ramp) * (3.f - (frost_ramp * 2.f));
  const float3 frost_color = ((frost_dark + (frost_blend * ((frost_scene * float3(7.f, -0.055999755859375f, -0.8400001525878906f)) - frost_dark))) * frost_radius_mask) + frost_scene;

  const float aspect_scale = view_size.y / view_size.x;
  const float frost_v = ((aspect_scale * screen_uv.y) + 0.5f) - (aspect_scale * 0.5f);
  const float4 frost_pattern = t3.Sample(s3, float2(((time * 0.012500000186264515f) + screen_uv.x) * 3.f, (frost_v + (time * 0.036250002682209015f)) * 3.f));
  const float4 frost_mask = t4.Sample(s4, screen_uv * 2.f);
  const float frost_fade = saturate((frost_weight - 0.4000000059604645f) * 1.1764705181121826f);
  const float frost_strength = ((frost_fade * frost_fade) * (3.f - (frost_fade * 2.f))) - 0.2199999988079071f;
  const float frost_scale = ((frost_pattern.y * 0.5f) * frost_mask.x) * frost_strength;
  const float2 frost_centered = float2(screen_uv.x * 2.f - 1.f, frost_v * 2.f - 1.f);
  const float frost_edge_distance = dot(frost_centered, frost_centered);
  const float frost_edge_ramp = saturate(((frost_edge_distance * frost_weight) - 0.4000000059604645f) * 4.999999523162842f);
  const float frost_edge = saturate((frost_edge_ramp * frost_edge_ramp) * (3.f - (frost_edge_ramp * 2.f)));
  const float frost_glow = PowZero(frost_edge * frost_strength, 0.675000011920929f);
  const float3 frost_input = frost_glow + ((frost_edge + min(max(color, 0.f), 0.6000000238418579f)) * frost_scale);
  const float3 frost_curve = float3(PowZero(frost_input.x, 8.f), PowZero(frost_input.y, 8.f), PowZero(frost_input.z, 8.f));
  const float frost_pattern_mix = frost_pattern.x * frost_noise.x;
  const float3 frost_curve_clamped = saturate(frost_curve);
  const float frost_crack = frost_mask.y * frost_pattern_mix;
  const float frost_highlight = (((frost_crack <= 2.980232949312267e-08f) ? -1.f : ((pow(frost_crack, 20.f) * 0.10000000149011612f) - 1.f)) * frost_curve_clamped.x) + 1.f;
  const float frost_rim = PowZero(frost_radius_mask, 0.30000001192092896f);
  if (cb2[2].x > 0.f) {
    const float3 frost_target = float3(
        ((saturate(frost_curve.x * 0.5f) * 1800.f) + 2.f) * frost_pattern_mix * frost_highlight,
        ((((flicker * 0.22866666316986084f) + 0.30133330821990967f) + ((154.86672973632812f - (flicker * 22.866666793823242f)) * saturate(frost_curve.y * 0.5f))) * frost_pattern_mix) * frost_highlight,
        ((saturate(frost_curve.z * 0.5f) * 16.000102996826172f) + 0.039999961853027344f) * frost_pattern_mix * frost_highlight);
    color = ((((frost_color - color) + ((frost_target - frost_color) * frost_curve_clamped)) * saturate(frost_rim + frost_curve_clamped)) + color);
  }

  // Film grain desaturation
  const float color_luma = renodx::color::y::from::BT709(color);
  const float grain_cell_x = floor(floor((screen_uv.x * 0.75f) * view_size.x) * 0.3499999940395355f);
  const float grain_cell_y = floor(floor((screen_uv.y * 0.75f) * view_size.y) * 0.5f);
  const float grain = saturate(CellNoise(float2(grain_cell_x, grain_cell_y), time_seed) + 0.10000000149011612f) * 0.5f;
  const float grain_falloff = saturate(1.2000000476837158f - saturate(frost_edge_distance * 0.8999999761581421f));
  if (cb2[1].w > 0.f) {
    const float3 grain_offset = float3(0.07829385250806808f, 0.11805565655231476f, 0.07919912785291672f);
    float3 grain_color = ((((grain * (color_luma - color)) + color) * 2.5f) + grain_offset) * grain_falloff;
    // RenoDX: this clip is the only one that limits the result to the SDR range, so only vanilla keeps it
    if (RENODX_TONE_MAP_TYPE == 0.f) {
      grain_color = saturate(grain_color);
    }
    color = ((grain_color - color) * cb2[1].w) + color;
  }

  // Radial pattern overlay
  const float2 scaled_centered = centered * 1.25f;
  const float radial_noise = t5.Sample(s5, float2(frac(PolarAngleTurns(scaled_centered)), length(scaled_centered) + (time * 0.125f))).x;
  const float radial_offset = (radial_noise - 0.6000000238418579f) * 0.019999999552965164f;
  const float3 radial_scene = SampleSceneClamped(screen_uv + radial_offset);
  const float radial_double = radial_noise * 2.f;
  const float radial_low = saturate(radial_double);
  const float3 radial_low_color = radial_low * float3(-0.14320087432861328f, -0.32659912109375f, -0.5260416269302368f);
  const float radial_high = saturate(radial_double - 1.f);
  const float radial_falloff = 1.f - (radius * 1.0416667461395264f);
  const float radial_curve_exponent = (3.3658077716827393f - (radius * 3.506049871444702f)) * (2.3329999446868896f - (radius * 2.430208444595337f));
  const float radial_weight = (((radial_falloff >= 0.f) && (abs(radial_falloff) > 9.999999747378752e-06f)) ? (1.f / exp2(radial_curve_exponent)) : 1.f) * cb2[3].z;
  if (cb2[3].z > 0.f) {
    const float3 radial_tint = float3(
        ((radial_low_color.x + 0.5225040316581726f) + ((0.07035225629806519f - radial_low_color.x) * radial_high)) * 0.10000000149011612f,
        ((radial_low_color.y + 0.6103416681289673f) + ((0.34130948781967163f - radial_low_color.y) * radial_high)) * 0.10000000149011612f,
        (((radial_low_color.z + 1.f) - (radial_low_color.z * radial_high))) * 0.10000000149011612f);
    color = (radial_weight * ((radial_scene - color) + radial_tint)) + color;
  }

  color = lerp(color, cb3[2].yzw, cb3[2].x);

  // RenoDX: negative channels carry colors outside BT.709, so only clip them for vanilla
  if (RENODX_TONE_MAP_TYPE == 0.f) {
    color = max(color, 0.f);
  }
  return float4(color, scene.w);
}
