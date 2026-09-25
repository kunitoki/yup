// Configuration for SLEEF compiled without its CMake build.
// Generated once for thirdparty/sleef_library from upstream sleef-config.h.in
// (SLEEF 3.9.0, static-lib configuration) - do not edit by hand.

#ifndef SLEEF_CONFIG_H
#define SLEEF_CONFIG_H

#define SLEEF_VERSION_MAJOR 3
#define SLEEF_VERSION_MINOR 9

// None of the YUP targets expose __float128 or an IEEE-quad long double, so
// quaddef.h falls back to its 64-bit-emulated Sleef_quad type.
/* #undef SLEEF_FLOAT128_IS_IEEEQP */
/* #undef SLEEF_LONGDOUBLE_IS_IEEEQP */

#ifndef SLEEF_STATIC_LIBS
#define SLEEF_STATIC_LIBS
#endif

#endif // SLEEF_CONFIG_H
