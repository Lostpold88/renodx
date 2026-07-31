#ifndef SRC_ASSCREEDBLACKFLAGRESYNCED_SHARED_H_
#define SRC_ASSCREEDBLACKFLAGRESYNCED_SHARED_H_

// Must be 32bit aligned
// Should be 4x32
struct ShaderInjectData {
  float tone_map_type;
  float tone_map_cone_response;

  float custom_color_filter_strength;
  float tone_map_exposure;
  float tone_map_highlights;
  float tone_map_shadows;
  float tone_map_contrast;
  float tone_map_flare;
  float tone_map_saturation;
  float tone_map_highlight_saturation;
  float tone_map_dechroma;
  float tone_map_coolness;

  float custom_bloom;
  float custom_bloom_scaling;

  float graphics_white_nits;

  // Display peak over diffuse white, filled in by the addon from the swapchain
  // metadata. Only used as a reference point for the purity controls, which run
  // in the scene-referred grading LUT where the game's own peak setting is not
  // bound.
  float custom_peak_ratio;
};

#ifndef __cplusplus
cbuffer shader_injection : register(b13, space50) {
  ShaderInjectData shader_injection : packoffset(c0);
}

#define RENODX_UI_MODE                                       0  // 1u = per pixel BT.2020 PQ Conversion
#define RENODX_UI_GAMMA_CORRECTION                           1
#define RENODX_GAME_GAMMA_CORRECTION                         1
#define USE_LUM_TM_WITH_CHROMINANCE_CORRECTION               1
#define USE_LUM_GAMMA_CORRECTION_WITH_CHROMINANCE_CORRECTION 1

// Tone mapper selection. 2 = PsychoV-22.
#define BLACKFLAG_TONE_MAP_TYPE_VANILLA   0.f
#define BLACKFLAG_TONE_MAP_TYPE_RENODX    1.f
#define BLACKFLAG_TONE_MAP_TYPE_PSYCHOV22 2.f

#define RENODX_TONE_MAP_TYPE          shader_injection.tone_map_type
#define RENODX_TONE_MAP_CONE_RESPONSE shader_injection.tone_map_cone_response

#define RENODX_GRAPHICS_WHITE_NITS shader_injection.graphics_white_nits

#define CUSTOM_COLOR_FILTER_STRENGTH         shader_injection.custom_color_filter_strength
#define RENODX_TONE_MAP_EXPOSURE             shader_injection.tone_map_exposure
#define RENODX_TONE_MAP_HIGHLIGHTS           shader_injection.tone_map_highlights
#define RENODX_TONE_MAP_SHADOWS              shader_injection.tone_map_shadows
#define RENODX_TONE_MAP_CONTRAST             shader_injection.tone_map_contrast
#define RENODX_TONE_MAP_SATURATION           shader_injection.tone_map_saturation
#define RENODX_TONE_MAP_HIGHLIGHT_SATURATION shader_injection.tone_map_highlight_saturation
#define RENODX_TONE_MAP_DECHROMA             shader_injection.tone_map_dechroma
#define RENODX_TONE_MAP_FLARE                shader_injection.tone_map_flare
#define RENODX_COLOR_GRADE_COOLNESS          shader_injection.tone_map_coolness
#define CUSTOM_PEAK_RATIO                    shader_injection.custom_peak_ratio

#define CUSTOM_BLOOM         shader_injection.custom_bloom
#define CUSTOM_BLOOM_SCALING shader_injection.custom_bloom_scaling

#include "../../shaders/renodx.hlsl"
#endif

#endif  // SRC_ASSCREEDBLACKFLAGRESYNCED_SHARED_H_
