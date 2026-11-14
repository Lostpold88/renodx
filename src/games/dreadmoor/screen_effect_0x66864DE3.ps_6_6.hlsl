#include "./screen_effect.hlsli"

float4 main(noperspective float4 position : SV_Position, linear float4 texcoord : TEXCOORD) : SV_Target {
  return ScreenEffect(position, true);
}
