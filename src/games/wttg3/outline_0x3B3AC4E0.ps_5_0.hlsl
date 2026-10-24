// Unreal Engine overlay pass blended on top of the tonemapped scene (stencil mask based), before the UI.
// Decompiled from the original shader with the DevKit.
#include "./common.hlsli"

struct t5_t {
  float val[4];
};
StructuredBuffer<t5_t> t5 : register(t5);

Texture2D<float4> t4 : register(t4);

Texture2D<float4> t3 : register(t3);

Texture2D<uint4> t2 : register(t2);

Texture2D<float4> t1 : register(t1);

Texture2D<float4> t0 : register(t0);

SamplerState s1_s : register(s1);

SamplerState s0_s : register(s0);

cbuffer cb3 : register(b3)
{
  float4 cb3[2];
}

cbuffer cb2 : register(b2)
{
  float4 cb2[1];
}

cbuffer cb1 : register(b1)
{
  float4 cb1[153];
}

cbuffer cb0 : register(b0)
{
  float4 cb0[6];
}




// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_POSITION0,
  out float4 o0 : SV_Target0)
{
  float4 r0,r1,r2;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xy = asuint(cb0[5].xy);
  r0.xy = v0.xy + -r0.xy;
  r0.xy = cb0[5].zw * r0.xy;
  r0.zw = r0.xy * cb1[149].xy + cb1[148].xy;
  r0.zw = cb1[152].zw * r0.zw;
  r1.x = t1.SampleLevel(s0_s, r0.zw, 0).x;
  r1.y = r1.x * cb1[78].z + -cb1[78].w;
  r1.x = r1.x * cb1[78].x + cb1[78].y;
  r1.y = 1 / r1.y;
  r1.x = r1.x + r1.y;
  r1.y = t0.SampleLevel(s0_s, r0.zw, 0).x;
  r0.zw = cb1[152].xy * r0.zw;
  r2.xy = (uint2)r0.zw;
  r0.z = r1.y * cb1[78].z + -cb1[78].w;
  r0.w = r1.y * cb1[78].x + cb1[78].y;
  r0.z = 1 / r0.z;
  r0.z = r0.w + r0.z;
  r0.z = -r0.z * 1.005 + r1.x;
  r0.z = saturate(ceil(r0.z));
  r0.z = 1 + -r0.z;
  r2.zw = float2(0,0);
  r0.w = t2.Load(r2.xyz).y;
  r0.w = (uint)r0.w;
  r0.w = -cb3[0].x + r0.w;
  r0.w = min(1, abs(r0.w));
  r0.w = ceil(r0.w);
  r0.w = 1 + -r0.w;
  r0.z = r0.w * r0.z;
  r1.xy = r0.xy * cb0[2].zw + cb0[2].xy;
  r0.xy = r0.xy * cb0[0].zw + cb0[0].xy;
  r2.xyzw = t3.Sample(s1_s, r0.xy).xyzw;
  r0.xyw = t4.Sample(s1_s, r1.xy).xyz;
  r1.x = t5[0].val[0/4];
  r0.xyw = r0.xyw / r1.xxx;
  // RenoDX: keep the scene overlay (in-world monitor) out of the UI brightness scaling
  r0.xyw = ScaleSceneOverlayToDefaultWhite(r0.xyw);
  r0.xyw = r0.xyw + -r2.xyz;
  r0.xyw = cb2[0].xxx * r0.xyw;
  r0.xyz = r0.zzz * r0.xyw + r2.xyz;
  o0.w = r2.w;
  r1.xyz = cb3[1].xyz + -r0.xyz;
  r0.xyz = cb3[0].yyy * r1.xyz + r0.xyz;
  // RenoDX: vanilla clamps to zero, which would cut the wide gamut (negative BT.709 channels) of the HDR tonemap
  if (RENODX_TONE_MAP_TYPE != 0.f) {
    o0.xyz = r0.xyz;
  } else {
    o0.xyz = max(float3(0,0,0), r0.xyz);
  }
  return;
}