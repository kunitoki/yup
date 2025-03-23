#pragma once

#include "specialization.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char specialization[] = R"===(layout(constant_id=gc)const bool pd=false;layout(constant_id=hc)const bool qd=false;layout(constant_id=ic)const bool rd=false;layout(constant_id=jc)const bool sd=false;layout(constant_id=kc)const bool td=false;layout(constant_id=lc)const bool ud=false;layout(constant_id=mc)const bool vd=false;layout(constant_id=nc)const bool wd=false;layout(constant_id=oc)const bool xd=false;
#define R pd
#define BB qd
#define FB rd
#define DB sd
#define IC td
#define YC ud
#define XB vd
#define XC wd
#define LC xd
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive