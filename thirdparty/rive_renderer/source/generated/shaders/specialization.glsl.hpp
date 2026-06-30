#pragma once

#include "specialization.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char specialization[] = R"===(#ifndef ME
layout(constant_id=xf)const bool Gg=true;layout(constant_id=yf)const bool Hg=true;layout(constant_id=zf)const bool Ig=true;layout(constant_id=Af)const bool Jg=true;layout(constant_id=Bf)const bool Kg=true;layout(constant_id=Cf)const bool Lg=true;layout(constant_id=Df)const bool Mg=true;layout(constant_id=Ef)const bool Ng=true;layout(constant_id=Ff)const bool Og=true;layout(constant_id=Gf)const bool Pg=true;layout(constant_id=Hf)const bool Qg=true;layout(constant_id=If)const uint Rg=0;
#define M Gg
#define Z Hg
#define FB Ig
#define GB Jg
#define IC Kg
#define NC Lg
#define SB Mg
#define IB Ng
#define YC Og
#define QB Pg
#define SC Qg
#define RC Rg
#else
#define M true
#define Z true
#define FB true
#define GB true
#define IC true
#define NC true
#define SB true
#define IB true
#define YC true
#define QB false
#define SC false
#define RC 0
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive