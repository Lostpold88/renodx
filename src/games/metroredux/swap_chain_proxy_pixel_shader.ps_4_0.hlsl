#include "./shared.h"

Texture2D t0 : register(t0);
SamplerState s0 : register(s0);
float4 main(float4 vpos: SV_POSITION, float2 uv: TEXCOORD0)
    : SV_TARGET {
  float4 outputColor = t0.Sample(s0, uv);
  // The RGBA8 back buffer turned a NaN written by the game into 0; the float clone keeps it
  if (any(isnan(outputColor.rgb))) {
    outputColor.rgb = 0.f;
  }
  return renodx::draw::SwapChainPass(outputColor);
}
