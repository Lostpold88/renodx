#version 450
#extension GL_EXT_samplerless_texture_functions : require
// Native HDR10 variant (game option HDR on): AMD CAS sharpen, then sRGB/2.2 decode, 300 nits, BT.2020 and PQ.
// CUSTOM: PsychoV-35 mode skips the 0..1 clamp and encodes with SwapChainPass instead.
#include "./shared.h"

struct OETF_CONSTANT_BUFFER {
  float4 _vfGamma;
};

struct CAS_CONSTANTS {
  uint4 vC0;
  uint4 vC1;
  float4 fViewport;
};

layout(set = 0, binding = 53, std140) uniform dyn_CONSTANT_BUFFER_BLOCK_1 {
  OETF_CONSTANT_BUFFER g_cbOETF;
}
_261;

layout(set = 0, binding = 54, std140) uniform dyn_CONSTANT_BUFFER_BLOCK_2 {
  CAS_CONSTANTS g_cbCAS;
}
_2573;

layout(set = 0, binding = 0) uniform sampler2D InputTex;

layout(location = 8) in float4 in_TexCoord;
layout(location = 0) out float4 outColor;

void main() {
  const bool hdr_output = RENODX_TONE_MAP_TYPE != 0.f;
  // CUSTOM: Highlights above SDR white survive CAS, their sharpening weight falls to zero there.
  const float cas_output_max = hdr_output ? 65504.0 : 1.0;
  // CUSTOM: Negative channels carry colors outside BT.709, keep them for the BT.2020 output.
  const float cas_output_min = hdr_output ? -65504.0 : 0.0;

  float2 _3495 = (float2(uint2(in_TexCoord.xy * _2573.g_cbCAS.fViewport.xy)) * uintBitsToFloat(_2573.g_cbCAS.vC0.xy)) + uintBitsToFloat(_2573.g_cbCAS.vC0.zw);
  float2 _3497 = floor(_3495);
  float2 _3500 = _3495 - _3497;
  int2 _3502 = int2(_3497);
  float4 _4972 = texelFetch(InputTex, int2(uint2(_3502 + int2(0, -1))), 0);
  float4 _4979 = texelFetch(InputTex, int2(uint2(_3502 + int2(-1, 0))), 0);
  float4 _4986 = texelFetch(InputTex, int2(uint2(_3502)), 0);
  float4 _4993 = texelFetch(InputTex, int2(uint2(_3502 + int2(1, -1))), 0);
  float4 _5007 = texelFetch(InputTex, int2(uint2(_3502 + int2(1, 0))), 0);
  float4 _5014 = texelFetch(InputTex, int2(uint2(_3502 + int2(2, 0))), 0);
  float4 _5021 = texelFetch(InputTex, int2(uint2(_3502 + int2(-1, 1))), 0);
  float4 _5028 = texelFetch(InputTex, int2(uint2(_3502 + int2(0, 1))), 0);
  float4 _5042 = texelFetch(InputTex, int2(uint2(_3502 + int2(0, 2))), 0);
  float4 _5049 = texelFetch(InputTex, int2(uint2(_3502 + int2(1, 1))), 0);
  float4 _5056 = texelFetch(InputTex, int2(uint2(_3502 + int2(2, 1))), 0);
  float4 _5063 = texelFetch(InputTex, int2(uint2(_3502 + int2(1, 2))), 0);
  float _3582 = max(_4972.y, 0.0);
  float _3595 = max(_4993.y, 0.0);
  float _3621 = max(_4979.y, 0.0);
  float _3634 = max(_4986.y, 0.0);
  float _3647 = max(_5007.y, 0.0);
  float _3660 = max(_5014.y, 0.0);
  float _3673 = max(_5021.y, 0.0);
  float _3686 = max(_5028.y, 0.0);
  float _3699 = max(_5049.y, 0.0);
  float _3712 = max(_5056.y, 0.0);
  float _3738 = max(_5042.y, 0.0);
  float _3751 = max(_5063.y, 0.0);
  float _5095 = min(min(_3582, min(_3621, _3634)), min(_3647, _3686));
  float _5131 = max(max(_3582, max(_3621, _3634)), max(_3647, _3686));
  float _5167 = min(min(_3595, min(_3634, _3647)), min(_3660, _3699));
  float _5203 = max(max(_3595, max(_3634, _3647)), max(_3660, _3699));
  float _5239 = min(min(_3634, min(_3673, _3686)), min(_3699, _3738));
  float _5275 = max(max(_3634, max(_3673, _3686)), max(_3699, _3738));
  float _5311 = min(min(_3647, min(_3686, _3699)), min(_3712, _3751));
  float _5347 = max(max(_3647, max(_3686, _3699)), max(_3712, _3751));
  float _4208 = uintBitsToFloat(_2573.g_cbCAS.vC1.x);
  float _4247 = _3500.x;
  float _4248 = 1.0 - _4247;
  float _4251 = _3500.y;
  float _4252 = 1.0 - _4251;
  float _4280 = (_4248 * _4252) * uintBitsToFloat(2129690299u - floatBitsToUint(0.03125 + (_5131 - _5095)));
  float _4288 = (_4247 * _4252) * uintBitsToFloat(2129690299u - floatBitsToUint(0.03125 + (_5203 - _5167)));
  float _4296 = (_4248 * _4251) * uintBitsToFloat(2129690299u - floatBitsToUint(0.03125 + (_5275 - _5239)));
  float _4304 = (_4247 * _4251) * uintBitsToFloat(2129690299u - floatBitsToUint(0.03125 + (_5347 - _5311)));
  float _4310 = (uintBitsToFloat((floatBitsToUint(clamp(min(_5095, 1.0 - _5131) * uintBitsToFloat(2129690299u - floatBitsToUint(_5131)), 0.0, 1.0)) >> 1u) + 532432441u) * _4208) * _4280;
  float _4319 = (uintBitsToFloat((floatBitsToUint(clamp(min(_5167, 1.0 - _5203) * uintBitsToFloat(2129690299u - floatBitsToUint(_5203)), 0.0, 1.0)) >> 1u) + 532432441u) * _4208) * _4288;
  float _4337 = (uintBitsToFloat((floatBitsToUint(clamp(min(_5239, 1.0 - _5275) * uintBitsToFloat(2129690299u - floatBitsToUint(_5275)), 0.0, 1.0)) >> 1u) + 532432441u) * _4208) * _4296;
  float _4338 = _4319 + _4337;
  float _4340 = _4338 + _4280;
  float _4364 = (uintBitsToFloat((floatBitsToUint(clamp(min(_5311, 1.0 - _5347) * uintBitsToFloat(2129690299u - floatBitsToUint(_5347)), 0.0, 1.0)) >> 1u) + 532432441u) * _4208) * _4304;
  float _4365 = _4310 + _4364;
  float _4367 = _4365 + _4288;
  float _4394 = _4365 + _4296;
  float _4421 = _4338 + _4304;
  float _4471 = (((((((2.0 * _4310) + (2.0 * _4319)) + (2.0 * _4337)) + (2.0 * _4364)) + _4340) + _4367) + _4394) + _4421;
  float _5852 = uintBitsToFloat(2129764351u - floatBitsToUint(_4471));
  float _5860 = _5852 * (((-_5852) * _4471) + 2.0);
  float _5871 = clamp(((((((((((((_4972.x * _4310) + (_4979.x * _4310)) + (_4993.x * _4319)) + (_5014.x * _4319)) + (_5021.x * _4337)) + (_5042.x * _4337)) + (_5056.x * _4364)) + (_5063.x * _4364)) + (_4986.x * _4340)) + (_5007.x * _4367)) + (_5028.x * _4394)) + (_5049.x * _4421)) * _5860, cas_output_min, cas_output_max);
  float _5882 = clamp(((((((((((((_4972.y * _4310) + (_4979.y * _4310)) + (_4993.y * _4319)) + (_5014.y * _4319)) + (_5021.y * _4337)) + (_5042.y * _4337)) + (_5056.y * _4364)) + (_5063.y * _4364)) + (_4986.y * _4340)) + (_5007.y * _4367)) + (_5028.y * _4394)) + (_5049.y * _4421)) * _5860, cas_output_min, cas_output_max);
  float _5893 = clamp(((((((((((((_4972.z * _4310) + (_4979.z * _4310)) + (_4993.z * _4319)) + (_5014.z * _4319)) + (_5021.z * _4337)) + (_5042.z * _4337)) + (_5056.z * _4364)) + (_5063.z * _4364)) + (_4986.z * _4340)) + (_5007.z * _4367)) + (_5028.z * _4394)) + (_5049.z * _4421)) * _5860, cas_output_min, cas_output_max);
  const float3 sharpened = float3(_5871, _5882, _5893);


  if (hdr_output) {
    outColor = float4(renodx::draw::SwapChainPass(sharpened), 1.0);
    return;
  }

  // Native: sRGB canvas, display gamma, shown on a 2.2 display at 300 nits.
  const float3 display_gamma = renodx::color::srgb::EncodeSafe(pow(renodx::color::srgb::DecodeSafe(sharpened), float3(1.0 / _261.g_cbOETF._vfGamma.x)));
  const float3 display_linear = pow(display_gamma, float3(2.2000000476837158203125));
  outColor = float4(renodx::color::pq::Encode(renodx::color::bt2020::from::BT709(display_linear), 300.0), 1.0);
}
