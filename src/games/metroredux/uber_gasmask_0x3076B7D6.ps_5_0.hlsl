#include "./common.hlsl"

// Uber composite with gas mask (DOF, bloom, lens dirt, filmic tonemap, mask glint, LUT), decompiled by the DevKit. RenoDX edits are marked.
Texture2D<float4> t10 : register(t10);

Texture2D<float4> t9 : register(t9);

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

SamplerState s2_s : register(s2);

cbuffer cb10 : register(b10)
{
  float4 cb10[1];
}

cbuffer cb3 : register(b3)
{
  float4 cb3[8];
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
  r0.zw = float2(5,3) * r0.xy;
  r1.xyz = t10.Sample(s2_s, r0.zw).xyw;
  r0.z = r1.z * 2 + cb3[6].w;
  r1.xy = float2(-0.501960814,-0.501960814) + r1.xy;
  r0.z = -1 + r0.z;
  r0.w = -3 + cb3[6].w;
  r2.xyz = t9.Sample(s5_s, r0.xy).xyw;
  r0.w = r2.z * r0.w;
  r0.z = r0.w * r0.z;
  r0.w = r2.z * 2 + -1;
  r0.z = saturate(r0.z * r0.w);
  r3.xyz = float3(1.89999998,-0.501960814,-0.501960814) + r2.zxy;
  r1.xy = -r3.yz * float2(0.100000001,0.100000001) + r1.xy;
  r1.zw = float2(0.100000001,0.100000001) * r3.yz;
  r0.w = r3.x + -r2.x;
  r0.w = r0.w + -r2.y;
  r0.w = saturate(10 * r0.w);
  r1.xy = r0.zz * r1.xy + r1.zw;
  r0.zw = r1.xy * r0.ww;
  r1.xy = t0.Sample(s6_s, r0.xy).xy;
  r0.zw = r1.xy * float2(0.0350000001,0.0350000001) + r0.zw;
  r1.xy = v0.xy * cb2[0].xy + r0.zw;
  r0.zw = cb10[0].xy * v0.xy;
  r3.xy = (int2)r0.zw;
  r3.zw = float2(0,0);
  r0.z = t1.Load(r3.xyz).z;
  r3.xy = saturate(r0.zz * cb3[5].wz + -cb3[5].xy);
  r0.z = cmp(1024 < r0.z);
  r0.w = 1 + -r3.x;
  r0.w = -r3.y + r0.w;
  r3.xyzw = t3.Sample(s6_s, r1.xy).xyzw;
  r3.xyzw = r3.xyzw + r3.xyzw;
  r0.z = r0.z ? -r3.w : r0.w;
  r0.z = saturate(-r0.z);
  r4.xyzw = t4.Sample(s6_s, r1.xy).xyzw;
  r0.w = saturate(3 * r4.w);
  r4.xyzw = r4.xyzw + r4.xyzw;
  r1.w = max(r0.z, r0.w);
  r5.xyzw = cb2[0].xyxy * float4(-0.400000006,0.800000012,0.800000012,0.400000006) + r0.xyxy;
  r6.xyzw = cb2[0].xyxy * float4(0.400000006,-0.800000012,-0.800000012,-0.400000006) + r0.xyxy;
  r7.xyz = t2.Sample(s6_s, r5.xy).xyz;
  r5.xyz = t2.Sample(s6_s, r5.zw).xyz;
  r0.xy = r7.xy + r5.xy;
  r2.w = min(r7.z, r5.z);
  r5.xyz = t2.Sample(s6_s, r6.xy).xyz;
  r6.xyz = t2.Sample(s6_s, r6.zw).xyz;
  r0.xy = r5.xy + r0.xy;
  r2.w = min(r5.z, r2.w);
  r2.w = min(r2.w, r6.z);
  r0.xy = r0.xy + r6.xy;
  r0.xy = float2(0.25,0.25) * r0.xy;
  r5.x = dot(r0.xy, r0.xy);
  r5.x = sqrt(r5.x);
  r5.x = 9.99999975e-006 + r5.x;
  r5.x = 0.00200000009 / r5.x;
  r5.x = min(1, r5.x);
  r5.yz = r5.xx * r0.xy;
  r0.x = cb2[0].x / cb2[0].y;
  r5.x = r5.y * r0.x;
  r6.xyzw = float4(-4,-4,-3,-3) * r5.xzxz;
  r7.xyzw = cb2[0].xyxy * float4(-1.3125,0.1875,-0.9375,-0.9375) + -r6.xyzw;
  r6.xyzw = r1.wwww * r7.xyzw + r6.xyzw;
  r6.xyzw = r6.xyzw + r1.xyxy;
  r7.xyzw = t6.Sample(s6_s, r6.zw).xyzw;
  r6.xyzw = t6.Sample(s6_s, r6.xy).xyzw;
  r0.y = -r7.w + r2.w;
  r0.y = max(0, r0.y);
  r0.y = r0.y * 3 + 1;
  r0.y = 1 / r0.y;
  r7.xyz = r7.xyz * r0.yyy;
  r5.y = -r6.w + r2.w;
  r5.y = max(0, r5.y);
  r5.y = r5.y * 3 + 1;
  r5.y = 1 / r5.y;
  r6.xyz = r6.xyz * r5.yyy + r7.xyz;
  r0.y = r5.y + r0.y;
  r7.xyzw = float4(-2,-2,3,3) * r5.xzxz;
  r5.yw = cb2[0].xy * float2(-0.5625,0.9375) + -r7.xy;
  r5.yw = r1.ww * r5.yw + r7.xy;
  r5.yw = r5.yw + r1.xy;
  r8.xyzw = t6.Sample(s6_s, r5.yw).xyzw;
  r5.y = -r8.w + r2.w;
  r5.y = max(0, r5.y);
  r5.y = r5.y * 3 + 1;
  r5.y = 1 / r5.y;
  r6.xyz = r8.xyz * r5.yyy + r6.xyz;
  r0.y = r5.y + r0.y;
  r5.yw = cb2[0].xy * float2(-0.1875,-0.5625) + r5.xz;
  r5.yw = r1.ww * r5.yw + -r5.xz;
  r5.yw = r5.yw + r1.xy;
  r8.xyzw = t6.Sample(s6_s, r5.yw).xyzw;
  r5.y = -r8.w + r2.w;
  r5.y = max(0, r5.y);
  r5.y = r5.y * 3 + 1;
  r5.y = 1 / r5.y;
  r6.xyz = r8.xyz * r5.yyy + r6.xyz;
  r0.y = r5.y + r0.y;
  r5.yw = cb2[0].xy * r1.ww;
  r5.yw = r5.yw * float2(0.1875,0.5625) + r1.xy;
  r8.xyzw = t6.Sample(s6_s, r5.yw).xyzw;
  r5.y = -r8.w + r2.w;
  r5.y = max(0, r5.y);
  r5.y = r5.y * 3 + 1;
  r5.y = 1 / r5.y;
  r6.xyz = r8.xyz * r5.yyy + r6.xyz;
  r0.y = r5.y + r0.y;
  r5.yw = cb2[0].xy * float2(0.5625,-1.3125) + -r5.xz;
  r5.yw = r1.ww * r5.yw + r5.xz;
  r5.xz = r5.xz + r5.xz;
  r5.yw = r5.yw + r1.xy;
  r8.xyzw = t6.Sample(s6_s, r5.yw).xyzw;
  r5.y = -r8.w + r2.w;
  r5.y = max(0, r5.y);
  r5.y = r5.y * 3 + 1;
  r5.y = 1 / r5.y;
  r6.xyz = r8.xyz * r5.yyy + r6.xyz;
  r0.y = r5.y + r0.y;
  r5.yw = cb2[0].xy * float2(0.9375,-0.1875) + -r5.xz;
  r5.xy = r1.ww * r5.yw + r5.xz;
  r5.xy = r5.xy + r1.xy;
  r5.xyzw = t6.Sample(s6_s, r5.xy).xyzw;
  r5.w = -r5.w + r2.w;
  r5.w = max(0, r5.w);
  r5.w = r5.w * 3 + 1;
  r5.w = 1 / r5.w;
  r5.xyz = r5.xyz * r5.www + r6.xyz;
  r0.y = r5.w + r0.y;
  r6.xy = cb2[0].xy * float2(1.3125,1.3125) + -r7.zw;
  r6.xy = r1.ww * r6.xy + r7.zw;
  r6.xy = r6.xy + r1.xy;
  r6.xyzw = t6.Sample(s6_s, r6.xy).xyzw;
  r1.w = -r6.w + r2.w;
  r1.w = max(0, r1.w);
  r1.w = r1.w * 3 + 1;
  r1.w = 1 / r1.w;
  r5.xyz = r6.xyz * r1.www + r5.xyz;
  r0.y = r1.w + r0.y;
  r5.xyz = r5.xyz / r0.yyy;
  r0.y = max(9.99999975e-005, abs(r3.w));
  r3.xyz = r3.xyz / r0.yyy;
  r3.xyz = r3.xyz + -r5.xyz;
  r3.xyz = r0.zzz * r3.xyz + r5.xyz;
  r0.y = max(9.99999975e-005, r4.w);
  r4.xyz = r4.xyz / r0.yyy;
  r4.xyz = r4.xyz + -r3.xyz;
  r0.yzw = r0.www * r4.xyz + r3.xyz;
  r1.w = -0.5 + r1.y;
  r1.z = r1.w * r0.x + 0.5;
  r3.xyzw = t8.Sample(s5_s, r1.xz).xyzw;
  r1.xyz = t5.Sample(s6_s, r1.xy).xyz;
  r3.xyz *= CUSTOM_LENS_DIRT;     // RenoDX: slider
  r1.xyz *= CUSTOM_BLOOM_STRENGTH;  // RenoDX: slider
  r3.xyz = r3.xyz * r3.www;
  r3.xyz = float3(5,5,5) * r3.xyz;
  r0.x = cb3[6].w * 5 + 1;
  r3.xyz = r3.xyz * r0.xxx + float3(1,1,1);
  r0.xyz = r1.xyz * r3.xyz + r0.yzw;
  r1.xyz = r3.xyz * r1.xyz;
  r1.xyz = float3(0.125,0.125,0.125) * r1.xyz;
  r0.xyz = max(float3(0,0,0), r0.xyz);
  float3 scene_color = r0.xyz;  // RenoDX: input of the filmic curve
  // Vanilla filmic curve, additive glow and LUT grade; replaced by ToneMapAndGrade in common.hlsl:
  // r3.xyz = r0.xyz * float3(0.150000006,0.150000006,0.150000006) + float3(0.0500000007,0.0500000007,0.0500000007);
  // r3.xyz = r0.xyz * r3.xyz + float3(0.00400000019,0.00400000019,0.00400000019);
  // r4.xyz = r0.xyz * float3(0.150000006,0.150000006,0.150000006) + float3(0.5,0.5,0.5);
  // r0.xyz = r0.xyz * r4.xyz + float3(0.0599999987,0.0599999987,0.0599999987);
  // r0.xyz = r3.xyz / r0.xyz;
  // r0.xyz = float3(-0.0666666701,-0.0666666701,-0.0666666701) + r0.xyz;
  // r0.xyz = r0.xyz * float3(4.53191471,4.53191471,4.53191471) + r1.xyz;
  float3 post_filmic_add = r1.xyz;  // RenoDX: 0.125 * bloom * lens dirt
  r1.xy = float2(-0.5,-0.5) + r2.xy;
  r0.w = r2.z * r2.z;
  r1.z = 1;
  r1.w = dot(r1.xyz, r1.xyz);
  r1.w = rsqrt(r1.w);
  r1.xyz = r1.xyz * r1.www;
  r1.x = dot(-cb3[6].xyz, r1.xyz);
  r1.x = r1.x * 0.5 + 0.5;
  r1.x = r1.x * r1.x;
  r1.x = max(0.100000001, r1.x);
  r1.xyz = cb3[7].xyz * r1.xxx;
  post_filmic_add += r0.www * r1.xyz;  // RenoDX: gas mask glint, vanilla adds it to the filmic output
  // r0.xyz = r0.www * r1.xyz + r0.xyz;
  // r1.xyz = t7.Sample(s6_s, r0.zyx).xyz;
  // r0.xyz = saturate(r0.xyz);
  // r0.xyz = r1.zyx * float3(2,2,2) + r0.xyz;
  // r0.xyz = float3(-1,-1,-1) + r0.xyz;
  // r0.w = dot(r0.xyz, float3(0.298999995,0.587000012,0.114));
  // o0.xyz = r0.xyz;
  // o0.w = sqrt(r0.w);
  o0 = ToneMapAndGrade(scene_color, post_filmic_add, coords, t7, s6_s);
  return;
}