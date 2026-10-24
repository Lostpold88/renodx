// Unreal Engine Slate UI pixel shader (desktop, browser and other UI). Decompiled from the original shader with the DevKit.
#include "./common.hlsli"

Texture2D<float4> t0 : register(t0);

SamplerState s0_s : register(s0);

cbuffer cb0 : register(b0)
{
  float4 cb0[1];
}




// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_POSITION0,
  float4 v1 : COLOR0,
  float4 v2 : COLOR1,
  float4 v3 : ORIGINAL_POSITION0,
  float4 v4 : TEXCOORD0,
  float4 v5 : TEXCOORD1,
  out float4 o0 : SV_Target0)
{
  float4 r0,r1,r2,r3;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.x = 1;
  r1.xyz = v1.xyz * r0.xxx + float3(-0.25,-0.25,-0.25);
  r1.xyz = saturate(r1.xyz * cb0[0].www + float3(0.25,0.25,0.25));
  r0.w = t0.Sample(s0_s, v5.xy).w;
  r0.xyzw = v1.xyzw * r0.xxxw;
  r2.xy = cmp(cb0[0].wy != float2(1,1));
  r0.xyz = r2.xxx ? r1.xyz : r0.xyz;
  r1.xyz = log2(r0.xyz);
  r1.xyz = cb0[0].xxx * r1.xyz;
  r2.xzw = float3(0.416666657,0.416666657,0.416666657) * r1.xyz;
  r1.xyz = exp2(r1.xyz);
  r2.xzw = exp2(r2.xzw);
  r2.xzw = r2.xzw * float3(1.05499995,1.05499995,1.05499995) + float3(-0.0549999997,-0.0549999997,-0.0549999997);
  r3.xyz = float3(12.9200001,12.9200001,12.9200001) * r1.xyz;
  r1.xyz = cmp(r1.xyz >= float3(0.00313066994,0.00313066994,0.00313066994));
  r1.xyz = r1.xyz ? r2.xzw : r3.xyz;
  o0.xyz = r2.yyy ? r1.xyz : r0.xyz;
  r0.x = 0.449999988 * r0.w;
  r0.y = r0.w * -0.899999976 + 1;
  o0.w = cb0[0].z * r0.y + r0.x;
  // RenoDX: scale the desktop independently of the other UI
  o0.xyz = ApplyDesktopBrightness(o0.xyz);
  return;
}
