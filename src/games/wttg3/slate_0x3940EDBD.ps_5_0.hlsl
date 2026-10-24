// Unreal Engine Slate UI pixel shader (desktop, browser and other UI). Decompiled from the original shader with the DevKit.
#include "./common.hlsli"

Texture2D<float4> t0 : register(t0);

SamplerState s0_s : register(s0);

cbuffer cb0 : register(b0)
{
  float4 cb0[3];
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
  float4 r0,r1,r2,r3,r4;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xy = cb0[2].xw + -cb0[2].yz;
  r0.zw = cmp(float2(0.5,0.5) >= v4.xy);
  r0.zw = r0.zw ? float2(1,1) : 0;
  r0.xy = r0.zz * r0.xy + cb0[2].yz;
  r0.x = r0.x + -r0.y;
  r0.x = r0.w * r0.x + r0.y;
  r0.yz = cb0[1].zw * float2(0.5,0.5) + -r0.xx;
  r1.xy = float2(0.5,0.5) * cb0[1].zw;
  r1.xy = cb0[1].zw * v4.xy + -r1.xy;
  r0.yz = abs(r1.xy) + -r0.yz;
  r1.zw = r0.yz + -r0.xx;
  r0.w = max(r1.z, r1.w);
  r1.z = dot(r0.yz, r0.yz);
  r0.yz = cmp(float2(0,0) >= r0.yz);
  r0.y = (int)r0.z | (int)r0.y;
  r0.y = r0.y ? 1.000000 : 0;
  r0.z = sqrt(r1.z);
  r0.z = r0.z + -r0.x;
  r0.w = r0.w + -r0.z;
  r0.y = r0.y * r0.w + r0.z;
  r0.y = -1 + r0.y;
  r0.y = saturate(-0.5 * r0.y);
  r0.z = r0.y * -2 + 3;
  r0.y = r0.y * r0.y;
  r0.y = r0.z * r0.y;
  r0.z = cmp(r0.x < cb0[1].y);
  r0.x = -cb0[1].y + r0.x;
  r0.x = max(0, r0.x);
  r0.z = r0.z ? 1.000000 : 0;
  r1.zw = v5.xy * v5.zw;
  r2.xyzw = t0.Sample(s0_s, r1.zw).xyzw;
  r3.xyz = r2.xyz * v1.xyz + -v2.xyz;
  r3.xyz = r0.zzz * r3.xyz + v2.xyz;
  r3.w = 0;
  r4.xyzw = v2.xyzw + -r3.xyzw;
  r3.xyzw = r0.yyyy * r4.xyzw + r3.xyzw;
  r2.xyzw = r2.xyzw * v1.xyzw + -r3.xyzw;
  r0.y = cb0[1].y + r0.x;
  r0.yz = cb0[1].zw * float2(0.5,0.5) + -r0.yy;
  r0.yz = abs(r1.xy) + -r0.yz;
  r1.xy = r0.yz + -r0.xx;
  r0.w = max(r1.x, r1.y);
  r1.x = dot(r0.yz, r0.yz);
  r0.yz = cmp(float2(0,0) >= r0.yz);
  r0.y = (int)r0.z | (int)r0.y;
  r0.y = r0.y ? 1.000000 : 0;
  r0.z = sqrt(r1.x);
  r0.x = r0.z + -r0.x;
  r0.z = r0.w + -r0.x;
  r0.x = r0.y * r0.z + r0.x;
  r0.x = -0.5 + r0.x;
  r0.x = saturate(-r0.x);
  r0.y = r0.x * -2 + 3;
  r0.x = r0.x * r0.x;
  r0.x = r0.y * r0.x;
  r0.xyzw = r0.xxxx * r2.xyzw + r3.xyzw;
  r1.xyz = float3(-0.25,-0.25,-0.25) + r0.xyz;
  r1.xyz = saturate(r1.xyz * cb0[0].www + float3(0.25,0.25,0.25));
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
  r0.x = r0.w * -2 + 1;
  o0.w = cb0[0].z * r0.x + r0.w;
  // RenoDX: scale the desktop independently of the other UI
  o0.xyz = ApplyDesktopBrightness(o0.xyz);
  return;
}
