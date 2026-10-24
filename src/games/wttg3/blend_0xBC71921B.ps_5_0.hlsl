// Unreal Engine overlay pass (peephole lens texture blended over the tonemapped scene), before the UI.
// Decompiled from the original shader with the DevKit.
#include "./shared.h"

Texture2D<float4> t4 : register(t4);

Texture2D<float4> t3 : register(t3);

Texture2D<uint4> t2 : register(t2);

Texture2D<float4> t1 : register(t1);

Texture2D<float4> t0 : register(t0);

SamplerState s3_s : register(s3);

SamplerState s2_s : register(s2);

SamplerState s1_s : register(s1);

SamplerState s0_s : register(s0);

cbuffer cb2 : register(b2)
{
  float4 cb2[7];
}

cbuffer cb1 : register(b1)
{
  float4 cb1[164];
}

cbuffer cb0 : register(b0)
{
  float4 cb0[11];
}




// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_POSITION0,
  out float4 o0 : SV_Target0)
{
  float4 r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13,r14;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xy = asuint(cb0[10].xy);
  r0.xy = v0.xy + -r0.xy;
  r0.zw = r0.xy * cb0[10].zw + float2(-0.5,-0.5);
  r0.xy = cb0[10].zw * r0.xy;
  r1.x = dot(r0.zw, r0.zw);
  r1.y = sqrt(r1.x);
  r1.y = cb2[0].x * r1.y + cb2[0].y;
  r1.x = r1.y * r1.x + 1;
  r1.x = cb2[0].z * r1.x;
  r0.zw = r1.xx * r0.zw + float2(0.5,0.5);
  r1.xy = frac(r0.zw);
  r0.zw = saturate(r0.zw);
  r1.z = cmp(cb2[0].w >= 1);
  r0.zw = r1.zz ? r1.xy : r0.zw;
  r1.x = cb2[1].x * r0.w;
  r1.x = 6.28318548 * r1.x;
  sincos(r1.x, r1.x, r2.x);
  r1.xz = r1.xx;
  r1.y = r2.x;
  r1.xyz = cb2[1].yyy * r1.xyz;
  r1.w = r0.z * r0.w;
  r0.zw = r0.zw * cb0[0].zw + cb0[0].xy;
  r0.zw = max(cb0[1].xy, r0.zw);
  r0.zw = min(cb0[1].zw, r0.zw);
  r2.xyzw = t4.Sample(s3_s, r0.zw).xyzw;
  r0.z = cb1[163].z * r1.w;
  r0.zw = float2(76.9230804,8.13008118) * r0.zz;
  r3.xy = cmp(r0.zw >= -r0.zw);
  r0.zw = frac(abs(r0.zw));
  r0.zw = r3.xy ? r0.zw : -r0.zw;
  r0.z = 13 * r0.z;
  r0.z = r0.w * r0.z;
  r0.z = 12300 * r0.z;
  r0.w = cmp(r0.z >= -r0.z);
  r0.z = frac(abs(r0.z));
  r0.z = r0.w ? r0.z : -r0.z;
  r0.z = saturate(r0.z * 1 + 0.100000001);
  r3.xyz = r2.xyz * r0.zzz + r2.xyz;
  r1.xyz = r1.xyz * r2.xyz + r3.xyz;
  r1.xyz = r1.xyz + -r2.xyz;
  r1.xyz = cb2[1].zzz * r1.xyz + r2.xyz;
  o0.w = r2.w;
  r0.zw = r0.xy * cb1[149].xy + cb1[148].xy;
  r0.zw = cb1[152].zw * r0.zw;
  r2.xy = cb1[152].xy * r0.zw;
  r2.xy = (uint2)r2.xy;
  r2.zw = float2(0,0);
  r1.w = t2.Load(r2.xyz).y;
  r1.w = (uint)r1.w;
  r1.w = -cb2[1].w + r1.w;
  r1.w = cmp(9.99999975e-006 < abs(r1.w));
  r2.x = t0.SampleLevel(s0_s, r0.zw, 0).x;
  r0.z = t1.SampleLevel(s0_s, r0.zw, 0).x;
  r0.w = r2.x * cb1[78].z + -cb1[78].w;
  r2.x = r2.x * cb1[78].x + cb1[78].y;
  r0.w = 1 / r0.w;
  r0.w = r2.x + r0.w;
  r1.w = r1.w ? -0 : -r0.w;
  r2.x = r0.z * cb1[78].z + -cb1[78].w;
  r0.z = r0.z * cb1[78].x + cb1[78].y;
  r2.x = 1 / r2.x;
  r0.z = r2.x + r0.z;
  r1.w = r0.z + r1.w;
  r0.z = r0.z + -r0.w;
  r0.w = cb2[4].z * r0.w;
  r2.xyzw = cmp(cb2[2].xyzw >= float4(0,0,0,1));
  r1.w = r2.x ? r1.w : r0.z;
  r3.xyz = cmp(float3(9.99999975e-006,9.99999975e-006,9.99999975e-006) < abs(cb2[2].xyz));
  r0.z = r3.x ? r1.w : r0.z;
  r0.z = saturate(ceil(r0.z));
  r0.z = 1 + -r0.z;
  r1.w = 1 + -r0.z;
  r1.w = r2.y ? r1.w : r0.z;
  r0.z = r3.y ? r1.w : r0.z;
  r2.xy = r0.xy * cb0[0].zw + cb0[0].xy;
  r0.xy = r0.xy * cb2[3].yz + float2(0.5,0.5);
  r0.xy = -cb2[4].xy + r0.xy;
  r0.x = t3.Sample(s1_s, r0.xy).x;
  r3.xyw = t4.Sample(s2_s, r2.xy).xyz;
  r4.xyz = -r3.xyw + r1.xyz;
  r4.xyz = r0.zzz * r4.xyz + r3.xyw;
  r2.xyz = r2.zzz ? r4.xyz : r1.xyz;
  r2.xyz = r3.zzz ? r2.xyz : r1.xyz;
  r4.xyz = r3.xyw + -r2.xyz;
  r5.xyz = float3(1,1,1) + -r2.xyz;
  r6.xyz = r3.xyw / r5.xyz;
  r6.xyz = r2.www ? r6.xyz : r2.xyz;
  r7.xyz = float3(1,1,1) + -r3.xyw;
  r8.xyz = r7.xyz / r2.xyz;
  r8.xyz = float3(1,1,1) + -r8.xyz;
  r9.xyzw = float4(-1,-3,-5,-7) + cb2[2].wwww;
  r9.xyzw = cmp(float4(9.99999975e-006,9.99999975e-006,9.99999975e-006,9.99999975e-006) < abs(r9.xyzw));
  r6.xyz = r9.xxx ? r6.xyz : r8.xyz;
  r8.xyz = float3(-0.5,-0.5,-0.5) + r2.xyz;
  r10.xyz = float3(-0.5,-0.5,-0.5) + r3.xyw;
  r10.xyz = r10.xyz * r8.xyz;
  r10.xyz = -r10.xyz * float3(2,2,2) + float3(0.5,0.5,0.5);
  r11.xyzw = cmp(cb2[2].wwww >= float4(3,5,7,9));
  r6.xyz = r11.xxx ? r10.xyz : r6.xyz;
  r4.xyz = r9.yyy ? r6.xyz : abs(r4.xyz);
  r6.xyz = r3.xyw + r2.xyz;
  r10.xyz = float3(-1,-1,-1) + r6.xyz;
  r4.xyz = r11.yyy ? r10.xyz : r4.xyz;
  r10.xyz = r2.xyz + r2.xyz;
  r12.xyz = r10.xyz * r3.xyw;
  r13.xyz = cmp(r2.xyz >= float3(0.5,0.5,0.5));
  r14.xyz = -r8.xyz * float3(2,2,2) + float3(1,1,1);
  r8.xyz = r8.xyz + r8.xyz;
  r8.xyz = max(r8.xyz, r3.xyw);
  r14.xyz = -r7.xyz * r14.xyz + float3(1,1,1);
  r12.xyz = r13.xyz ? r14.xyz : r12.xyz;
  r4.xyz = r9.zzz ? r4.xyz : abs(r12.xyz);
  r9.xyz = float3(-0.125,-0.125,-0.125) + r2.xyz;
  r9.xyz = r9.xyz * float3(1.25,1.25,1.25) + r3.xyw;
  r12.xyz = float3(-0.25,-0.25,-0.25) + r2.xyz;
  r12.xyz = r12.xyz * float3(2,2,2) + r3.xyw;
  r9.xyz = r13.xyz ? r9.xyz : r12.xyz;
  r4.xyz = r11.zzz ? r9.xyz : r4.xyz;
  r4.xyz = r9.www ? r4.xyz : r6.xyz;
  r6.xyz = -r2.xyz * float3(2,2,2) + float3(1,1,1);
  r6.xyz = -r6.xyz * r7.xyz + float3(1,1,1);
  r6.xyz = r6.xyz * r3.xyw;
  r9.xyz = -r3.xyw * r10.xyz + float3(1,1,1);
  r10.xyz = min(r10.xyz, r3.xyw);
  r8.xyz = r13.xyz ? r8.xyz : r10.xyz;
  r9.xyz = -r7.xyz * r9.xyz + float3(1,1,1);
  r6.xyz = r13.xyz ? r6.xyz : r9.xyz;
  r4.xyz = r11.www ? r6.xyz : r4.xyz;
  r6.xyzw = float4(-9,-11,-13,-15) + cb2[2].wwww;
  r6.xyzw = cmp(float4(9.99999975e-006,9.99999975e-006,9.99999975e-006,9.99999975e-006) < abs(r6.xyzw));
  r4.xyz = r6.xxx ? r4.xyz : r8.xyz;
  r8.xyz = max(r3.xyw, r2.xyz);
  r9.xyz = cmp(cb2[2].www >= float3(11,13,15));
  r4.xyz = r9.xxx ? r8.xyz : r4.xyz;
  r8.xyz = -r5.xyz * r7.xyz + float3(1,1,1);
  r5.xyz = r7.xyz * r5.xyz;
  r5.xyz = -r5.xyz * float3(2,2,2) + float3(1,1,1);
  r4.xyz = r6.yyy ? r4.xyz : r8.xyz;
  r7.xyz = r3.xyw * r2.xyz;
  r2.xyz = min(r3.xyw, r2.xyz);
  r8.xyz = r7.xyz + r7.xyz;
  r10.xyz = cmp(r3.xyw >= float3(0.5,0.5,0.5));
  r5.xyz = r10.xyz ? r5.xyz : r8.xyz;
  r4.xyz = r9.yyy ? r5.xyz : r4.xyz;
  r2.xyz = r6.zzz ? r4.xyz : r2.xyz;
  r2.xyz = r9.zzz ? float3(0,1,0.0428028107) : r2.xyz;
  r2.xyz = r6.www ? r2.xyz : r7.xyz;
  r2.xyz = r2.xyz + -r3.xyw;
  r2.xyz = cb2[3].xxx * r2.xyz;
  r0.xyz = r2.xyz * r0.xxx;
  r1.w = log2(r0.w);
  r0.w = cmp(2.98023295e-008 >= r0.w);
  r1.w = cb2[4].w * r1.w;
  r1.w = exp2(r1.w);
  r1.w = min(1, r1.w);
  r0.w = r0.w ? 0 : r1.w;
  r1.w = 1 + -r0.w;
  r2.xy = cmp(cb2[5].xy >= float2(0,0.5));
  r1.w = r2.x ? r1.w : r0.w;
  r2.x = cmp(9.99999975e-006 < abs(cb2[5].x));
  r0.w = r2.x ? r1.w : r0.w;
  r0.xyz = r0.www * r0.xyz + r3.xyw;
  r0.xyz = r2.yyy ? r1.xyz : r0.xyz;
  r1.xyz = cb2[6].xyz + -r0.xyz;
  r0.xyz = cb2[5].zzz * r1.xyz + r0.xyz;
  // RenoDX: vanilla clamps to zero, which would cut the wide gamut (negative BT.709 channels) of the HDR tonemap
  if (RENODX_TONE_MAP_TYPE != 0.f) {
    o0.xyz = r0.xyz;
  } else {
    o0.xyz = max(float3(0,0,0), r0.xyz);
  }
  return;
}