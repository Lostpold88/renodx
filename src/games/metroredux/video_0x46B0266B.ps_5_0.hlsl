// Video player YUV to RGB composite (intro and cutscenes), decompiled by the DevKit. RenoDX edits are marked.
Texture2D<float4> t0 : register(t0);

SamplerState s6_s : register(s6);




// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float3 v1 : TEXCOORD0,
  float w1 : TEXCOORD2,
  float4 v2 : TEXCOORD1,
  out float4 o0 : SV_Target0)
{
  float4 r0,r1,r2;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.x = cmp(v2.y < v1.x);
  r0.y = cmp(v1.x < v2.z);
  r0.x = (int)r0.y | (int)r0.x;
  r0.y = cmp(1 < v1.y);
  r0.z = cmp(v1.y < 0);
  r0.y = (int)r0.z | (int)r0.y;
  r0.x = (int)r0.y | (int)r0.x;
  r1.xyzw = t0.Sample(s6_s, v1.xy).xyzw;
  r2.xyzw = float4(1.16406,0,-0.390625,2.01561999) * r1.zyyy;
  r0.yzw = r2.xxx + r2.yzw;
  r0.yzw = r1.xxx * float3(1.59765005,-0.8125,0) + r0.yzw;
  r0.yzw = float3(-0.869610012,0.53075999,-1.07860005) + r0.yzw;
  r0.yzw = r0.yzw * r1.www;
  // RenoDX: the RGBA8 back buffer clamped negative YUV overshoot to 0 and the NaN of sqrt along with it. The float
  // clone keeps NaN, which breaks the pixel and the HDR10 encode.
  r0.yzw = max(float3(0,0,0), r0.yzw);
  r0.yzw = sqrt(r0.yzw);
  r0.xyz = r0.xxx ? float3(0,0,0) : r0.yzw;
  r1.xy = v1.xy + w1;  // RenoDX: the decompiler emitted the invalid swizzle w1.xw; the disassembly adds v1.w to both
  r0.w = dot(r1.xy, float2(12.9898005,78.2330017));
  r0.w = sin(r0.w);
  r0.w = 43758.5469 * r0.w;
  r0.w = frac(r0.w);
  r0.w = -0.5 + r0.w;
  r0.w = dot(r0.www, float3(0.00333329989,0.00333329989,0.00333329989));
  r0.xyz = r0.xyz + r0.www;
  o0.xyz = saturate(r0.xyz * r0.xyz);  // RenoDX: keeps the RGBA8 write clamp for the SDR video
  o0.w = 1;
  return;
}