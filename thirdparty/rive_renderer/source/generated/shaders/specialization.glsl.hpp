#pragma once

#include "specialization.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char specialization[] = R"===(layout(constant_id=ea)const bool Sa=false;layout(constant_id=fa)const bool Ta=false;layout(constant_id=ga)const bool Ua=false;layout(constant_id=ha)const bool Va=false;layout(constant_id=ia)const bool Wa=false;layout(constant_id=ja)const bool Xa=false;
#define B Sa
#define U Ta
#define BB Ua
#define DC Va
#define SC Wa
#define LB Xa
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive