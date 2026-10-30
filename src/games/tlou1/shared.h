#ifndef SRC_TLOU1_SHARED_H_
#define SRC_TLOU1_SHARED_H_

// Must be 32bit aligned
// Should be 4x32
struct ShaderInjectData {
  float tone_map_type;
  float peak_white_nits;
  float diffuse_white_nits;
  float graphics_white_nits;

  float gamma_correction;
  float psychov_cone_response;
  float psychov_gamut_mode;
  float color_grade_strength;

  float tone_map_saturation;
  float padding_0;
  float padding_1;
  float padding_2;
};

#ifndef __cplusplus
cbuffer shader_injection : register(b13, space50) {
  ShaderInjectData shader_injection : packoffset(c0);
}

#define RENODX_TONE_MAP_TYPE          shader_injection.tone_map_type
#define RENODX_PEAK_WHITE_NITS        shader_injection.peak_white_nits
#define RENODX_DIFFUSE_WHITE_NITS     shader_injection.diffuse_white_nits
#define RENODX_GRAPHICS_WHITE_NITS    shader_injection.graphics_white_nits
#define RENODX_SDR_EOTF_EMULATION     shader_injection.gamma_correction
#define RENODX_PSYCHOV_CONE_RESPONSE  shader_injection.psychov_cone_response
#define RENODX_PSYCHOV_GAMUT_MODE     shader_injection.psychov_gamut_mode
#define RENODX_COLOR_GRADE_STRENGTH   shader_injection.color_grade_strength
#define RENODX_TONE_MAP_SATURATION    shader_injection.tone_map_saturation

#include "../../shaders/renodx.hlsl"
#endif

#endif  // SRC_TLOU1_SHARED_H_
