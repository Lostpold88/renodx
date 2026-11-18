#version 450
// Native HDR10 output without AMD CAS (game option CAS off): sRGB/2.2 decode, 300 nits, BT.2020 and PQ.
// CUSTOM: PsychoV-35 mode encodes with SwapChainPass instead.
#include "./shared.h"

struct OETF_CONSTANT_BUFFER {
  float4 _vfGamma;
};

layout(set = 0, binding = 53, std140) uniform dyn_CONSTANT_BUFFER_BLOCK_1 {
  OETF_CONSTANT_BUFFER g_cbOETF;
}
_444;

layout(set = 0, binding = 0) uniform sampler2D InputTex;

layout(location = 8) in float4 in_TexCoord;
layout(location = 0) out float4 outColor;

void main() {
  const float3 canvas = texture(InputTex, in_TexCoord.xy).xyz;

  if (RENODX_TONE_MAP_TYPE != 0.f) {
    outColor = float4(renodx::draw::SwapChainPass(canvas), 1.0);
    return;
  }

  // Native: sRGB canvas, display gamma, shown on a 2.2 display at 300 nits.
  const float3 display_gamma = renodx::color::srgb::EncodeSafe(pow(renodx::color::srgb::DecodeSafe(canvas), float3(1.0 / _444.g_cbOETF._vfGamma.x)));
  const float3 display_linear = pow(display_gamma, float3(2.2000000476837158203125));
  outColor = float4(renodx::color::pq::Encode(renodx::color::bt2020::from::BT709(display_linear), 300.0), 1.0);
}
