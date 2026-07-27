/* libjpeg-turbo build number */
#define BUILD  "20260628"

/* How to hide global symbols. */
#define HIDDEN

/* Compiler's inline keyword */
#undef inline

/* How to obtain function inlining. */
#define INLINE  inline

/* How to obtain thread-local storage */
#if defined(_MSC_VER)
#define THREAD_LOCAL  __declspec(thread)
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define THREAD_LOCAL  _Thread_local
#else
#define THREAD_LOCAL  __thread
#endif

/* Define to the full name of this package. */
#define PACKAGE_NAME  "libjpeg-turbo"

/* Version number of package */
#define VERSION  "3.1.4.1"

/* The size of `size_t'. */
#if defined(_WIN64) || defined(__x86_64__) || defined(__aarch64__) || defined(__LP64__)
#define SIZEOF_SIZE_T  8
#else
#define SIZEOF_SIZE_T  4
#endif

#if defined(__has_attribute)
#if __has_attribute(fallthrough)
#define FALLTHROUGH  __attribute__((fallthrough));
#else
#define FALLTHROUGH
#endif
#else
#define FALLTHROUGH
#endif

#ifndef BITS_IN_JSAMPLE
#define BITS_IN_JSAMPLE  8
#endif

/* Arithmetic-coded JPEGs are intentionally not supported by this vendored build. */
#undef C_ARITH_CODING_SUPPORTED
#undef D_ARITH_CODING_SUPPORTED

#if defined(__aarch64__) || defined(_M_ARM64) || defined(_M_ARM64EC) || defined(__ARM_NEON)
#define WITH_SIMD  1
#define NEON_INTRINSICS  1
#else
#undef WITH_SIMD
#endif
