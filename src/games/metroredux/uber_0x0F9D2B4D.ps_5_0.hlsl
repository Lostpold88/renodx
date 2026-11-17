#include "./common.hlsl"

// Uber composite (DOF, bloom, lens dirt, filmic tonemap, LUT), decompiled by the DevKit. RenoDX edits are marked.
Texture2D<float4> t8 : register(t8);

Texture3D<float4> t7 : register(t7);

Texture2D<float4> t6 : register(t6);

Texture2D<float4> t5 : register(t5);

Texture2D<float4> t4 : register(t4);

Texture2D<float4> t3 : register(t3);

Texture2D<float4> t2 : register(t2);

Texture2D<float4> t1 : register(t1);

Texture2D<float4> t0 : register(t0);

SamplerState s6_s : register(s6);

SamplerState s5_s : register(s5);

cbuffer cb10 : register(b10)
{
  float4 cb10[1];
}

cbuffer cb3 : register(b3)
{
  float4 cb3[7];
}

cbuffer cb2 : register(b2)
{
  float4 cb2[1];
}




// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float3 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0)
{
  float4 r0,r1,r2,r3,r4,r5,r6,r7,r8;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xy = cb2[0].xy * v0.xy;
  float2 coords = r0.xy;  // RenoDX: film grain position
  r1.xyzw = cb2[0].xyxy * float4(-0.400000006,0.800000012,0.800000012,0.400000006) + r0.xyxy;
  r2.xyz = t2.Sample(s6_s, r1.xy).xyz;
  r1.xyz = t2.Sample(s6_s, r1.zw).xyz;
  r0.zw = r2.xy + r1.xy;
  r1.x = min(r2.z, r1.z);
  r2.xyzw = cb2[0].xyxy * float4(0.400000006,-0.800000012,-0.800000012,-0.400000006) + r0.xyxy;
  r1.yzw = t2.Sample(s6_s, r2.xy).xyz;
  r2.xyz = t2.Sample(s6_s, r2.zw).xyz;
  r0.zw = r1.yz + r0.zw;
  r1.x = min(r1.x, r1.w);
  r1.x = min(r1.x, r2.z);
  r0.zw = r0.zw + r2.xy;
  r0.zw = float2(0.25,0.25) * r0.zw;
  r1.y = dot(r0.zw, r0.zw);
  r1.y = sqrt(r1.y);
  r1.y = 9.99999975e-006 + r1.y;
  r1.y = 0.00200000009 / r1.y;
  r1.y = min(1, r1.y);
  r2.yz = r1.yy * r0.zw;
  r0.z = cb2[0].x / cb2[0].y;
  r2.x = r2.y * r0.z;
  r3.xyzw = float4(-4,-4,-3,-3) * r2.xzxz;
  r4.xyzw = cb2[0].xyxy * float4(-1.3125,0.1875,-0.9375,-0.9375) + -r3.xyzw;
  r1.yz = cb10[0].xy * v0.xy;
  r5.xy = (int2)r1.yz;
  r5.zw = float2(0,0);
  r0.w = t1.Load(r5.xyz).z;
  r1.yz = saturate(r0.ww * cb3[5].wz + -cb3[5].xy);
  r0.w = cmp(1024 < r0.w);
  r1.y = 1 + -r1.y;
  r1.y = -r1.z + r1.y;
  r1.zw = t0.Sample(s6_s, r0.xy).xy;
  r5.xy = r1.zw * float2(0.0350000001,0.0350000001) + r0.xy;
  r6.xyzw = t3.Sample(s6_s, r5.xy).xyzw;
  r6.xyzw = r6.xyzw + r6.xyzw;
  r0.x = r0.w ? -r6.w : r1.y;
  r0.x = saturate(-r0.x);
  r7.xyzw = t4.Sample(s6_s, r5.xy).xyzw;
  r0.y = saturate(3 * r7.w);
  r7.xyzw = r7.xyzw + r7.xyzw;
  r0.w = max(r0.x, r0.y);
  r3.xyzw = r0.wwww * r4.xyzw + r3.xyzw;
  r3.xyzw = r5.xyxy + r3.xyzw;
  r4.xyzw = t6.Sample(s6_s, r3.zw).xyzw;
  r3.xyzw = t6.Sample(s6_s, r3.xy).xyzw;
  r1.y = -r4.w + r1.x;
  r1.y = max(0, r1.y);
  r1.y = r1.y * 3 + 1;
  r1.y = 1 / r1.y;
  r4.xyz = r4.xyz * r1.yyy;
  r1.z = -r3.w + r1.x;
  r1.z = max(0, r1.z);
  r1.z = r1.z * 3 + 1;
  r1.z = 1 / r1.z;
  r3.xyz = r3.xyz * r1.zzz + r4.xyz;
  r1.y = r1.z + r1.y;
  r4.xyzw = float4(-2,-2,3,3) * r2.xzxz;
  r1.zw = cb2[0].xy * float2(-0.5625,0.9375) + -r4.xy;
  r1.zw = r0.ww * r1.zw + r4.xy;
  r1.zw = r5.xy + r1.zw;
  r8.xyzw = t6.Sample(s6_s, r1.zw).xyzw;
  r1.z = -r8.w + r1.x;
  r1.z = max(0, r1.z);
  r1.z = r1.z * 3 + 1;
  r1.z = 1 / r1.z;
  r3.xyz = r8.xyz * r1.zzz + r3.xyz;
  r1.y = r1.y + r1.z;
  r1.zw = cb2[0].xy * float2(-0.1875,-0.5625) + r2.xz;
  r1.zw = r0.ww * r1.zw + -r2.xz;
  r1.zw = r5.xy + r1.zw;
  r8.xyzw = t6.Sample(s6_s, r1.zw).xyzw;
  r1.z = -r8.w + r1.x;
  r1.z = max(0, r1.z);
  r1.z = r1.z * 3 + 1;
  r1.z = 1 / r1.z;
  r3.xyz = r8.xyz * r1.zzz + r3.xyz;
  r1.y = r1.y + r1.z;
  r1.zw = cb2[0].xy * r0.ww;
  r1.zw = r1.zw * float2(0.1875,0.5625) + r5.xy;
  r8.xyzw = t6.Sample(s6_s, r1.zw).xyzw;
  r1.z = -r8.w + r1.x;
  r1.z = max(0, r1.z);
  r1.z = r1.z * 3 + 1;
  r1.z = 1 / r1.z;
  r3.xyz = r8.xyz * r1.zzz + r3.xyz;
  r1.y = r1.y + r1.z;
  r1.zw = cb2[0].xy * float2(0.5625,-1.3125) + -r2.xz;
  r1.zw = r0.ww * r1.zw + r2.xz;
  r2.xy = r2.xz + r2.xz;
  r1.zw = r5.xy + r1.zw;
  r8.xyzw = t6.Sample(s6_s, r1.zw).xyzw;
  r1.z = -r8.w + r1.x;
  r1.z = max(0, r1.z);
  r1.z = r1.z * 3 + 1;
  r1.z = 1 / r1.z;
  r3.xyz = r8.xyz * r1.zzz + r3.xyz;
  r1.y = r1.y + r1.z;
  r1.zw = cb2[0].xy * float2(0.9375,-0.1875) + -r2.xy;
  r1.zw = r0.ww * r1.zw + r2.xy;
  r1.zw = r5.xy + r1.zw;
  r2.xyzw = t6.Sample(s6_s, r1.zw).xyzw;
  r1.z = -r2.w + r1.x;
  r1.z = max(0, r1.z);
  r1.z = r1.z * 3 + 1;
  r1.z = 1 / r1.z;
  r2.xyz = r2.xyz * r1.zzz + r3.xyz;
  r1.y = r1.y + r1.z;
  r1.zw = cb2[0].xy * float2(1.3125,1.3125) + -r4.zw;
  r1.zw = r0.ww * r1.zw + r4.zw;
  r1.zw = r5.xy + r1.zw;
  r3.xyzw = t6.Sample(s6_s, r1.zw).xyzw;
  r0.w = -r3.w + r1.x;
  r0.w = max(0, r0.w);
  r0.w = r0.w * 3 + 1;
  r0.w = 1 / r0.w;
  r1.xzw = r3.xyz * r0.www + r2.xyz;
  r0.w = r1.y + r0.w;
  r1.xyz = r1.xzw / r0.www;
  r0.w = max(9.99999975e-005, abs(r6.w));
  r2.xyz = r6.xyz / r0.www;
  r2.xyz = r2.xyz + -r1.xyz;
  r1.xyz = r0.xxx * r2.xyz + r1.xyz;
  r0.x = max(9.99999975e-005, r7.w);
  r2.xyz = r7.xyz / r0.xxx;
  r2.xyz = r2.xyz + -r1.xyz;
  r0.xyw = r0.yyy * r2.xyz + r1.xyz;
  r1.x = -0.5 + r5.y;
  r5.z = r1.x * r0.z + 0.5;
  r1.xyzw = t8.Sample(s5_s, r5.xz).xyzw;
  r2.xyz = t5.Sample(s6_s, r5.xy).xyz;
  r1.xyz *= CUSTOM_LENS_DIRT;     // RenoDX: slider
  r2.xyz *= CUSTOM_BLOOM_STRENGTH;  // RenoDX: slider
  r1.xyz = r1.xyz * r1.www;
  r1.xyz = float3(5,5,5) * r1.xyz;
  r0.z = cb3[6].w * 5 + 1;
  r1.xyz = r1.xyz * r0.zzz + float3(1,1,1);
  r0.xyz = r2.xyz * r1.xyz + r0.xyw;
  r1.xyz = r2.xyz * r1.xyz;
  r1.xyz = float3(0.125,0.125,0.125) * r1.xyz;
  r0.xyz = max(float3(0,0,0), r0.xyz);
  // Vanilla filmic curve, additive glow and LUT grade; replaced by ToneMapAndGrade in common.hlsl:
  // r2.xyz = r0.xyz * float3(0.150000006,0.150000006,0.150000006) + float3(0.0500000007,0.0500000007,0.0500000007);
  // r2.xyz = r0.xyz * r2.xyz + float3(0.00400000019,0.00400000019,0.00400000019);
  // r3.xyz = r0.xyz * float3(0.150000006,0.150000006,0.150000006) + float3(0.5,0.5,0.5);
  // r0.xyz = r0.xyz * r3.xyz + float3(0.0599999987,0.0599999987,0.0599999987);
  // r0.xyz = r2.xyz / r0.xyz;
  // r0.xyz = float3(-0.0666666701,-0.0666666701,-0.0666666701) + r0.xyz;
  // r0.xyz = r0.xyz * float3(4.53191471,4.53191471,4.53191471) + r1.xyz;
  // r1.xyz = t7.Sample(s6_s, r0.zyx).xyz;
  // r0.xyz = saturate(r0.xyz);
  // r0.xyz = r1.zyx * float3(2,2,2) + r0.xyz;
  // r0.xyz = float3(-1,-1,-1) + r0.xyz;
  // r0.w = dot(r0.xyz, float3(0.298999995,0.587000012,0.114));
  // o0.xyz = r0.xyz;
  // o0.w = sqrt(r0.w);
  o0 = ToneMapAndGrade(r0.xyz, r1.xyz, coords, t7, s6_s);  // RenoDX: r0 = scene color, r1 = 0.125 * bloom * lens dirt
  return;
}