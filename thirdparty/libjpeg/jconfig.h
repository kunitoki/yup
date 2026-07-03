/* Version ID for the JPEG library.
 * Might be useful for tests like "#if JPEG_LIB_VERSION >= 60".
 */
#define JPEG_LIB_VERSION  62

/* libjpeg-turbo version */
#define LIBJPEG_TURBO_VERSION  3.1.4.1

/* libjpeg-turbo version in integer form */
#define LIBJPEG_TURBO_VERSION_NUMBER  3001004

/* Arithmetic-coded JPEGs are intentionally not supported by this vendored build. */
#undef C_ARITH_CODING_SUPPORTED
#undef D_ARITH_CODING_SUPPORTED

/* Support in-memory source/destination managers */
#define MEM_SRCDST_SUPPORTED  1

#if defined(__aarch64__) || defined(_M_ARM64) || defined(_M_ARM64EC) || defined(__ARM_NEON)
#define WITH_SIMD  1
#define NEON_INTRINSICS  1
#else
#undef WITH_SIMD
#endif

#ifndef BITS_IN_JSAMPLE
#define BITS_IN_JSAMPLE  8
#endif

#ifdef _WIN32

#undef RIGHT_SHIFT_IS_UNSIGNED

#ifndef __RPCNDR_H__
typedef unsigned char boolean;
#endif
#define HAVE_BOOLEAN

#if !(defined(_BASETSD_H_) || defined(_BASETSD_H))
typedef short INT16;
typedef signed int INT32;
#endif
#define XMD_H

#else

#undef RIGHT_SHIFT_IS_UNSIGNED

#endif
