// Unreal Engine Slate UI pixel shader (desktop, browser and other UI). Decompiled from the original shader with the DevKit.
#include "./common.hlsli"

cbuffer cb1 : register(b1)
{
  float4 cb1[4];
}

cbuffer cb0 : register(b0)
{
  float4 cb0[2];
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

  r0.xy = v5.xy * v5.zw + float2(-0.5,-0.5);
  r0.zw = r0.xy + r0.xy;
  r1.x = max(abs(r0.w), abs(r0.z));
  r1.x = 1 / r1.x;
  r1.y = min(abs(r0.w), abs(r0.z));
  r1.x = r1.y * r1.x;
  r1.y = r1.x * r1.x;
  r1.z = r1.y * 0.0208350997 + -0.0851330012;
  r1.z = r1.y * r1.z + 0.180141002;
  r1.z = r1.y * r1.z + -0.330299497;
  r1.y = r1.y * r1.z + 0.999866009;
  r1.z = r1.x * r1.y;
  r1.z = r1.z * -2 + 1.57079637;
  r1.w = cmp(abs(r0.z) < abs(r0.w));
  r1.z = r1.w ? r1.z : 0;
  r1.x = r1.x * r1.y + r1.z;
  r1.y = cmp(r0.z < -r0.z);
  r1.y = r1.y ? -3.141593 : 0;
  r1.x = r1.x + r1.y;
  r1.y = min(r0.w, r0.z);
  r1.y = cmp(r1.y < -r1.y);
  r1.z = max(r0.w, r0.z);
  r0.z = dot(r0.zw, r0.zw);
  r0.z = sqrt(r0.z);
  r0.w = cmp(r1.z >= -r1.z);
  r0.w = r0.w ? r1.y : 0;
  r0.w = r0.w ? -r1.x : r1.x;
  r0.w = -3.14159012 + r0.w;
  r1.x = 0.5 * cb1[3].y;
  r1.x = max(1, r1.x);
  r1.x = 3.14159274 / r1.x;
  r0.w = r1.x * 0.5 + r0.w;
  r0.w = r0.w / r1.x;
  r1.y = cmp(r0.w >= -r0.w);
  r0.w = frac(abs(r0.w));
  r0.w = r1.y ? r0.w : -r0.w;
  r1.y = r0.w * r1.x + r1.x;
  r0.w = r0.w * r1.x;
  r1.z = cmp(r0.w < 0);
  r0.w = r1.z ? r1.y : r0.w;
  r0.w = -r1.x * 0.5 + r0.w;
  r0.w = sin(r0.w);
  r0.w = r0.z * abs(r0.w) + -cb1[3].x;
  r0.z = -cb1[3].z + r0.z;
  r0.z = -cb1[3].z + -abs(r0.z);
  r0.zw = float2(1,-0.00999999978) + r0.zw;
  r0.w = saturate(999.999817 * r0.w);
  r1.x = r0.w * -2 + 3;
  r0.w = r0.w * r0.w;
  r0.w = r1.x * r0.w;
  r1.x = cb1[3].w + -cb1[3].z;
  r1.x = 1 / r1.x;
  r0.z = saturate(r1.x * r0.z);
  r1.x = r0.z * -2 + 3;
  r0.z = r0.z * r0.z;
  r0.z = r1.x * r0.z;
  r0.z = r0.w * r0.z;
  r1.y = dot(r0.xy, float2(-1,9.00000003e-008));
  r1.x = dot(r0.xy, float2(-8.64102034e-008,-1));
  r0.xy = r1.xy + r1.xy;
  r0.w = max(abs(r0.x), abs(r0.y));
  r0.w = 1 / r0.w;
  r1.x = min(abs(r0.x), abs(r0.y));
  r0.w = r1.x * r0.w;
  r1.x = r0.w * r0.w;
  r1.y = r1.x * 0.0208350997 + -0.0851330012;
  r1.y = r1.x * r1.y + 0.180141002;
  r1.y = r1.x * r1.y + -0.330299497;
  r1.x = r1.x * r1.y + 0.999866009;
  r1.y = r1.x * r0.w;
  r1.y = r1.y * -2 + 1.57079637;
  r1.z = cmp(abs(r0.y) < abs(r0.x));
  r1.y = r1.z ? r1.y : 0;
  r0.w = r0.w * r1.x + r1.y;
  r1.x = cmp(r0.y < -r0.y);
  r1.x = r1.x ? -3.141593 : 0;
  r0.w = r1.x + r0.w;
  r1.x = min(r0.x, r0.y);
  r0.x = max(r0.x, r0.y);
  r0.x = cmp(r0.x >= -r0.x);
  r0.y = cmp(r1.x < -r1.x);
  r0.x = r0.x ? r0.y : 0;
  r0.x = r0.x ? -r0.w : r0.w;
  r0.x = 0.159154952 * r0.x;
  r0.x = frac(r0.x);
  r0.y = -cb1[2].w + r0.x;
  r0.y = ceil(r0.y);
  r0.y = 1 + -r0.y;
  r1.w = saturate(r0.y * r0.z);
  r0.yzw = -cb1[1].xyz + cb1[0].xyz;
  r0.xyz = r0.xxx * r0.yzw + cb1[1].xyz;
  r2.xyz = cb1[2].xyz + -r0.xyz;
  r0.xyz = cb1[1].www * r2.xyz + r0.xyz;
  r1.xyz = max(float3(0,0,0), r0.xyz);
  r0.xyzw = v1.xyzw * r1.xyzw;
  r1.xyz = r1.xyz * v1.xyz + float3(-0.25,-0.25,-0.25);
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
  r0.x = 0.449999988 * r0.w;
  r0.y = cmp(cb0[1].x != 0.000000);
  o0.w = r0.y ? r0.x : r0.w;
  // RenoDX: scale the desktop independently of the other UI
  o0.xyz = ApplyDesktopBrightness(o0.xyz);
  return;
}
