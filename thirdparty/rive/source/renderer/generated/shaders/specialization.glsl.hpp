#pragma once

#include "specialization.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char specialization[] = R"===(layout(constant_id=Wg) const bool ki=true;layout(constant_id=Xg) const bool li=true;layout(constant_id=Yg) const bool mi=true;layout(constant_id=Zg) const bool ni=true;layout(constant_id=ah) const bool oi=true;layout(constant_id=bh) const bool pi=true;layout(constant_id=ch) const bool qi=true;layout(constant_id=dh) const bool ri=true;layout(constant_id=eh) const bool si=true;layout(constant_id=fh) const bool ti=true;layout(constant_id=gh) const bool ui=false;layout(constant_id=hh) const bool vi=false;layout(constant_id=ih) const bool wi=false;layout(constant_id=jh) const bool xi=false;layout(constant_id=kh) const bool yi=false;
#define A ki
#define AB li
#define N mi
#define HB ni
#define XC oi
#define AD pi
#define FC qi
#define OB ri
#define GB si
#define GE ti
#define JD ui
#define EC vi
#define QD wi
#define RD xi
#define HD yi
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive