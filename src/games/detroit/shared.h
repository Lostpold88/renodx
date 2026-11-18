#ifndef SRC_DETROIT_SHARED_H_
#define SRC_DETROIT_SHARED_H_

#define RENODX_TONE_MAP_TYPE_PSYCHOV35 8.f

// Must be 32bit aligned
struct ShaderInjectData {
  float peak_white_nits;
  float diffuse_white_nits;
  float graphics_white_nits;
  float color_grade_strength;
  float tone_map_type;
  float tone_map_exposure;
  float tone_map_highlights;
  float tone_map_shadows;
  float tone_map_contrast;
  float tone_map_saturation;
  float tone_map_blowout;
  float gamma_correction;
  float psychov_cone_response;
  float custom_gamut_expansion;
};

#ifndef __cplusplus
// The native grade/composite pipelines have no push constants of their own.
struct PushData {
  [[vk::offset(0)]]
  ShaderInjectData shader_injection;
};

[[vk::push_constant]]
PushData gPush;
#define shader_injection gPush.shader_injection

#define RENODX_TONE_MAP_TYPE            shader_injection.tone_map_type
#define RENODX_PEAK_WHITE_NITS          shader_injection.peak_white_nits
#define RENODX_DIFFUSE_WHITE_NITS       shader_injection.diffuse_white_nits
#define RENODX_GRAPHICS_WHITE_NITS      shader_injection.graphics_white_nits
#define RENODX_GAMMA_CORRECTION         shader_injection.gamma_correction
#define RENODX_TONE_MAP_EXPOSURE        shader_injection.tone_map_exposure
#define RENODX_TONE_MAP_HIGHLIGHTS      shader_injection.tone_map_highlights
#define RENODX_TONE_MAP_SHADOWS         shader_injection.tone_map_shadows
#define RENODX_TONE_MAP_CONTRAST        shader_injection.tone_map_contrast
#define RENODX_TONE_MAP_SATURATION      shader_injection.tone_map_saturation
#define RENODX_TONE_MAP_BLOWOUT         shader_injection.tone_map_blowout
#define RENODX_COLOR_GRADE_STRENGTH     shader_injection.color_grade_strength
#define RENODX_PSYCHOV_CONE_RESPONSE    shader_injection.psychov_cone_response
#define CUSTOM_GAMUT_EXPANSION          shader_injection.custom_gamut_expansion

#define RENODX_SWAP_CHAIN_CLAMP_COLOR_SPACE    renodx::color::convert::COLOR_SPACE_BT2020
#define RENODX_SWAP_CHAIN_ENCODING             ENCODING_PQ
#define RENODX_SWAP_CHAIN_ENCODING_COLOR_SPACE renodx::color::convert::COLOR_SPACE_BT2020

#include "../../shaders/renodx.hlsl"
#endif

#endif  // SRC_DETROIT_SHARED_H_
