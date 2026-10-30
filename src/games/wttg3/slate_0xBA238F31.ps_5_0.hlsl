// Unreal Engine Slate UI pixel shader (desktop, browser and other UI). Decompiled from the original shader with the DevKit.
#include "./common.hlsli"

Texture2D<float4> t12 : register(t12);

Texture2D<float4> t11 : register(t11);

Texture2D<float4> t10 : register(t10);

Texture2D<float4> t9 : register(t9);

Texture2D<float4> t8 : register(t8);

Texture2D<float4> t7 : register(t7);

Texture2D<float4> t6 : register(t6);

Texture2D<float4> t5 : register(t5);

Texture2D<float4> t4 : register(t4);

Texture2D<float4> t3 : register(t3);

Texture2D<float4> t2 : register(t2);

Texture2D<float4> t1 : register(t1);

Texture2D<float4> t0 : register(t0);

SamplerState s12_s : register(s12);

SamplerState s11_s : register(s11);

SamplerState s10_s : register(s10);

SamplerState s9_s : register(s9);

SamplerState s8_s : register(s8);

SamplerState s7_s : register(s7);

SamplerState s6_s : register(s6);

SamplerState s5_s : register(s5);

SamplerState s4_s : register(s4);

SamplerState s3_s : register(s3);

SamplerState s2_s : register(s2);

SamplerState s1_s : register(s1);

SamplerState s0_s : register(s0);

cbuffer cb2 : register(b2)
{
  float4 cb2[20];
}

cbuffer cb1 : register(b1)
{
  float4 cb1[164];
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
  float4 r0,r1,r2,r3,r4,r5,r6,r7;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.x = cb2[5].z + cb1[163].z;
  r0.xyz = float3(1.2566371,5.02654839,6.28318548) * r0.xxx;
  r0.xyz = sin(r0.xyz);
  r0.x = r0.x * r0.y;
  r0.x = max(0, r0.x);
  r0.x = frac(r0.x);
  r1.x = cb2[2].z * cb1[163].z;
  r1.y = 0.0333333351 * r1.x;
  r0.y = t2.Sample(s2_s, r1.xy).x;
  r0.y = -cb2[2].w + r0.y;
  r0.y = saturate(cb2[3].x * r0.y);
  r0.w = v5.x * v5.z + -0.5;
  r1.x = cb2[1].z * cb1[163].z;
  r2.yz = v5.xy * v5.zw;
  r1.y = r2.z * cb2[1].y + r1.x;
  r1.y = 6.28318548 * r1.y;
  r1.z = r2.z * cb2[1].w + -r1.x;
  r1.x = r2.z * cb2[2].x + r1.x;
  r1.xz = float2(6.28318548,6.28318548) * r1.xz;
  r1.xyz = sin(r1.xyz);
  r1.y = r1.y + r1.z;
  r1.x = r1.y + r1.x;
  r0.w = r0.w * 6 + r1.x;
  r0.w = cb2[2].y * r0.w;
  r0.w = 0.333333343 * r0.w;
  r2.x = r0.w * r0.y + r2.y;
  r0.yw = r0.yy * cb2[3].yy + r2.xz;
  r0.yw = -v5.xy * v5.zw + r0.yw;
  r0.yw = cb2[3].zz * r0.yw + r2.yz;
  r0.yw = r0.yw * cb2[4].xy + float2(0.5,0.5);
  r1.xy = -cb2[4].zw + r0.yw;
  r3.x = cb2[5].w * cb1[163].z;
  r3.y = cb2[6].x * cb1[163].z;
  r0.yw = r2.yz * float2(0.00100000005,0.00499999989) + r3.xy;
  r0.y = t4.Sample(s4_s, r0.yw).x;
  r0.y = cb2[6].y * r0.y;
  r0.x = r0.x * r0.y + r1.y;
  r0.y = cmp(r0.x >= -r0.x);
  r0.x = frac(abs(r0.x));
  r3.z = r0.y ? r0.x : -r0.x;
  r4.yw = float2(0.5,5) * cb1[163].zz;
  r0.xy = r1.xy * cb2[5].xx + r4.yy;
  r0.x = t3.Sample(s3_s, r0.xy).x;
  r3.y = -r0.x * cb2[5].y + r1.x;
  r5.xz = float2(0,0);
  r5.yw = float2(0.00100000005,-0.00499999989) * cb1[163].zz;
  r6.xy = r2.yz * float2(0.00100000005,0.00499999989) + r5.xy;
  r0.xy = r2.yz * float2(0,0.00499999989) + r5.zw;
  r0.x = t4.Sample(s4_s, r0.xy).x;
  r0.y = r0.z * 0.00039999999 + r6.y;
  r6.z = 15 * r0.y;
  r0.y = t4.Sample(s4_s, r6.xz).x;
  r0.z = 0.100000001 * r0.y;
  r0.y = r0.y * 0.200000003 + 0.800000012;
  r0.w = r0.z * r0.x;
  r3.yz = r0.ww * cb2[6].zz + r3.yz;
  r0.w = cb2[6].z * r0.w;
  r0.w = 0.5 * r0.w;
  r5.xyz = t6.SampleLevel(s6_s, r3.yz, cb2[9].y).xyz;
  r2.xw = float2(1,1) + -r5.yz;
  r1.w = r5.x * r2.x;
  r1.w = r1.w * r2.w;
  r2.x = cb2[9].z * cb1[163].z;
  r2.w = cb2[9].w * r2.x;
  r4.xy = float2(6.28318548,0.628318548) * r2.xx;
  r4.xy = sin(r4.xy);
  r2.x = 12.566371 * r2.w;
  r2.x = sin(r2.x);
  r2.x = r2.x * r4.y;
  r5.xy = float2(1,5) * r2.yz;
  r2.w = t8.Sample(s8_s, r5.xy).x;
  r2.x = r2.w * r4.x + r2.x;
  r5.xz = float2(0,0);
  r5.yw = float2(-0.800000012,-0.5) * cb1[163].zz;
  r4.xy = r2.yz * float2(0.200000003,0.200000003) + r5.xy;
  r5.xy = v5.xy * v5.zw + r5.zw;
  r2.w = t8.Sample(s8_s, r5.xy).x;
  r3.w = t8.Sample(s8_s, r4.xy).x;
  r2.x = r3.w * r2.x;
  r2.x = r2.x * r2.w + 0.100000001;
  r2.w = t7.Sample(s7_s, float2(1,1)).x;
  r2.x = r2.w * r2.x;
  r5.y = 0;
  r5.xz = float2(0.300000012,-5) * cb1[163].zz;
  r4.xy = r2.yz * float2(0.100000001,2) + r5.yz;
  r2.w = frac(r5.x);
  r2.w = 30 * r2.w;
  r2.w = ceil(r2.w);
  r3.w = t1.Sample(s1_s, r4.xy).x;
  r2.x = r3.w * r2.x;
  r2.x = r3.w * 0.00100000005 + r2.x;
  r2.x = cb2[9].w * r2.x;
  r1.w = r2.x * r1.w;
  r3.x = r1.w * 2 + r3.y;
  r5.xy = r1.ww * float2(0.800000012,1.10000002) + r3.yy;
  r4.xy = cb2[10].xy + r3.xz;
  r3.xw = cb2[11].xy + r3.xz;
  r1.w = t6.SampleLevel(s6_s, r3.xw, cb2[11].z).x;
  r1.w = saturate(-cb2[11].w + r1.w);
  r2.x = t6.SampleLevel(s6_s, r4.xy, cb2[9].y).x;
  r3.x = t6.SampleLevel(s6_s, r4.xy, cb2[10].z).x;
  r2.x = r3.x * cb2[10].w + r2.x;
  r1.w = r1.w * cb2[12].x + r2.x;
  r3.xy = r3.yz * cb2[7].yz + float2(0.5,0.5);
  r5.z = r3.z;
  r3.xy = -cb2[8].xy + r3.xy;
  r4.x = -cb2[8].z + r3.x;
  r4.y = cb2[8].w + r3.y;
  r2.x = t5.Sample(s5_s, r4.xy).x;
  r1.w = r2.x * cb2[9].x + r1.w;
  r3.x = t6.SampleLevel(s6_s, r5.yz, cb2[9].y).z;
  r3.y = t6.SampleLevel(s6_s, r5.yz, cb2[10].z).z;
  r5.xyzw = cb2[13].yzww + r5.xzxz;
  r3.x = r3.y * cb2[10].w + r3.x;
  r3.x = r2.x * cb2[9].x + r3.x;
  r3.y = -r1.w * cb2[14].x + r3.x;
  r3.z = t6.SampleLevel(s6_s, r5.xy, cb2[9].y).y;
  r3.w = t6.SampleLevel(s6_s, r5.xy, cb2[10].z).y;
  r4.x = t6.SampleLevel(s6_s, r5.zw, cb2[11].z).y;
  r4.x = saturate(-cb2[11].w + r4.x);
  r3.z = r3.w * cb2[10].w + r3.z;
  r3.z = r4.x * cb2[12].x + r3.z;
  r2.x = r2.x * cb2[9].x + r3.z;
  r3.y = -r2.x * cb2[14].x + r3.y;
  r1.z = cb2[12].w * r1.y;
  r3.zw = cb1[163].zz * cb2[12].yz + r1.xz;
  r1.x = t6.Sample(s6_s, r1.xy).w;
  r1.y = t9.Sample(s9_s, r3.zw).x;
  r1.z = t11.SampleLevel(s11_s, r3.zw, 3).x;
  r1.z = 1 + -r1.z;
  r1.z = saturate(cb2[14].z * r1.z);
  r1.z = cb2[14].w + -r1.z;
  r1.z = saturate(1 + r1.z);
  r1.y = saturate(cb2[13].x + r1.y);
  r3.y = r3.y * r1.y;
  r3.z = 1 + -r1.y;
  r3.y = r3.x * r3.z + r3.y;
  r3.y = r3.y * 0.666666687 + -r0.w;
  r3.w = 62.8318558 * r2.w;
  r2.w = cb1[163].z * r2.w;
  r4.xy = r2.ww * float2(0.162866458,0.32702288) + r2.yz;
  r2.w = sin(r3.w);
  r3.w = cb1[163].z * r2.w;
  r5.xy = float2(50000,6.28318548) * r2.ww;
  r6.xy = float2(0.000250000012,0.000500000024) * r3.ww;
  r2.w = sin(r5.y);
  r6.w = cb1[163].z * r2.w;
  r5.yw = v5.xy * v5.zw + r6.yw;
  r5.yw = r5.yw + r4.xy;
  r2.w = cmp(r5.y >= 1.5);
  r5.z = 0.333333343;
  r6.yw = r5.yw * r5.zx;
  r3.w = r5.y * r5.z + -0.5;
  r2.w = r2.w ? r3.w : r6.y;
  r3.w = cmp(9.99999975e-006 < abs(r3.w));
  r7.x = r3.w ? r2.w : r6.y;
  r7.y = 0.333333343 * r6.w;
  r2.w = t10.Sample(s10_s, r7.xy).x;
  r2.w = cb2[14].y * r2.w;
  r7.z = r2.w * 0.100000001 + r3.y;
  r2.w = -r1.w * cb2[14].x + r2.x;
  r2.w = -r3.x * cb2[14].x + r2.w;
  r3.y = r3.z * r2.x;
  r2.x = -r2.x * cb2[14].x + r1.w;
  r2.x = -r3.x * cb2[14].x + r2.x;
  r2.x = r2.x * r1.y;
  r1.y = r2.w * r1.y + r3.y;
  r1.y = r1.y * 0.666666687 + -r0.w;
  r1.w = r1.w * r3.z + r2.x;
  r7.x = r1.w * 0.666666687 + -r0.w;
  r6.z = 0;
  r2.xw = v5.xy * v5.zw + r6.xz;
  r2.xw = r2.xw + r4.xy;
  r3.xy = r2.xw * float2(0.333333343,0.333333343) + float2(0.100000001,-0.600000024);
  r3.z = r3.y * r5.x;
  r0.w = t10.Sample(s10_s, r3.xz).x;
  r0.w = cb2[14].y * r0.w;
  r7.y = r0.w * 0.0199999996 + r1.y;
  r1.yzw = r7.xyz * r1.zzz;
  r4.z = 0;
  r3.xyz = t1.Sample(s1_s, r4.zw).xyz;
  r4.xyzw = cb2[0].xyww * r2.yzyz;
  r2.xw = cb1[163].zz * float2(5,5) + r4.zw;
  r4.xy = float2(5.5,5.5) * r4.xy;
  r4.xyz = t0.Sample(s0_s, r4.xy).xyz;
  r4.xyz = saturate(cb2[0].zzz + r4.xyz);
  r5.xyz = t1.Sample(s1_s, r2.xw).xyz;
  r3.xyz = r5.xyz * r3.xyz;
  r1.yzw = saturate(r3.xyz * cb2[1].xxx + r1.yzw);
  r0.w = v5.y * v5.w + -cb2[15].w;
  r0.w = max(0, r0.w);
  r0.w = min(cb2[16].x, r0.w);
  r0.x = r0.z * r0.x + r0.w;
  r3.xw = float2(1,2);
  r3.yz = cb2[15].zz;
  r0.zw = r3.xy * r2.yz;
  r0.zw = r0.zw * r3.zw;
  r0.zw = cb1[163].zz * float2(10,3) + r0.zw;
  r2.x = t4.Sample(s4_s, r0.zw).x;
  r0.zw = float2(0.400000006,0.400000006) * r0.zw;
  r0.z = t4.Sample(s4_s, r0.zw).x;
  r0.z = cb2[16].y + r0.z;
  r0.x = r2.x * r0.x;
  r0.x = saturate(r0.x * r0.z + -cb2[15].w);
  r0.xyz = saturate(r0.xxx * r0.yyy + r1.yzw);
  r0.xyz = float3(1,1,1) + -r0.xyz;
  r0.xyz = r4.xyz + -r0.xyz;
  r1.yz = cb2[17].xx * r2.yz;
  r1.yz = cb1[163].zz * cb2[16].zw + r1.yz;
  r0.w = t7.Sample(s7_s, r1.yz).x;
  r1.y = saturate(5 * r0.w);
  r0.w = saturate(0.200000003 + r0.w);
  r0.w = 0.5 + r0.w;
  r1.y = 0.5 + r1.y;
  r1.y = min(1, r1.y);
  r0.w = saturate(r1.y * r0.w + cb2[17].y);
  r0.xyz = r0.xyz * r0.www;
  r1.yzw = t12.Sample(s12_s, r2.yz).xyz;
  r0.w = t6.Sample(s6_s, r2.yz).w;
  r2.w = saturate(r1.x + r0.w);
  r1.xyz = saturate(r1.yzw * cb2[17].zzz + cb2[17].www);
  r0.xyz = r1.xyz * r0.xyz;
  r1.xyz = cb2[18].zzz * r0.xyz;
  r0.xyz = -r0.xyz * cb2[18].zzz + cb2[19].xyz;
  r0.xyz = cb2[18].www * r0.xyz + r1.xyz;
  r2.xyz = max(float3(0,0,0), r0.xyz);
  r0.xyzw = v1.xyzw * r2.xyzw;
  r1.xyz = r2.xyz * v1.xyz + float3(-0.25,-0.25,-0.25);
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
