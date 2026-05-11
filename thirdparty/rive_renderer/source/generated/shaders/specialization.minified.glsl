#ifndef SPEC_CONST_NONE
layout(constant_id=xf)const bool Gg=true;layout(constant_id=yf)const bool Hg=true;layout(constant_id=zf)const bool Ig=true;layout(constant_id=Af)const bool Jg=true;layout(constant_id=Bf)const bool Kg=true;layout(constant_id=Cf)const bool Lg=true;layout(constant_id=Df)const bool Mg=true;layout(constant_id=Ef)const bool Ng=true;layout(constant_id=Ff)const bool Og=true;layout(constant_id=Gf)const bool Pg=true;layout(constant_id=Hf)const bool Qg=true;layout(constant_id=If)const uint Rg=0;
#define ENABLE_CLIPPING Gg
#define ENABLE_CLIP_RECT Hg
#define ENABLE_ADVANCED_BLEND Ig
#define ENABLE_FEATHER Jg
#define ENABLE_EVEN_ODD Kg
#define ENABLE_NESTED_CLIPPING Lg
#define ENABLE_HSL_BLEND_MODES Mg
#define ENABLE_DITHER Ng
#define CLOCKWISE_FILL Og
#define BORROWED_COVERAGE_PASS Pg
#define NESTED_CLIP_UPDATE_ONLY Qg
#define VULKAN_VENDOR_ID Rg
#else
#define ENABLE_CLIPPING true
#define ENABLE_CLIP_RECT true
#define ENABLE_ADVANCED_BLEND true
#define ENABLE_FEATHER true
#define ENABLE_EVEN_ODD true
#define ENABLE_NESTED_CLIPPING true
#define ENABLE_HSL_BLEND_MODES true
#define ENABLE_DITHER true
#define CLOCKWISE_FILL true
#define BORROWED_COVERAGE_PASS false
#define NESTED_CLIP_UPDATE_ONLY false
#define VULKAN_VENDOR_ID 0
#endif
