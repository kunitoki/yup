/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

   YUP is an open source library subject to open-source licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   to use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   YUP IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#ifndef _TIF_CONFIG_
#define _TIF_CONFIG_

#include "tiffconf.h"

/* Support CCITT Group 3 & 4 algorithms */
#define CCITT_SUPPORT 1

/* Pick up YCbCr subsampling info from the JPEG data stream to support files
   lacking the tag (default enabled). */
#define CHECK_JPEG_YCBCR_SUBSAMPLING 1

/* enable partial strip reading for large strips (experimental) */
/* #undef CHUNKY_STRIP_READ_SUPPORT */

/* Support C++ stream API (requires C++ compiler) */
/* #undef CXX_SUPPORT */

/* enable deferred strip/tile offset/size loading */
/* #undef DEFER_STRILE_LOAD */

/* Define to 1 if you have the <assert.h> header file. */
#define HAVE_ASSERT_H 1

/* Define to 1 if you have the declaration of `optarg', and to 0 if you don't. */
#define HAVE_DECL_OPTARG 0

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if fseeko (and presumably ftello) exists and is declared. */
#if !defined(_WIN32)
#define HAVE_FSEEKO 1
#else
/* #undef HAVE_FSEEKO */
#endif

/* Define to 1 if you have the `getopt' function. */
/* #undef HAVE_GETOPT */

/* Define to 1 if you have the <GLUT/glut.h> header file. */
/* #undef HAVE_GLUT_GLUT_H */

/* Define to 1 if you have the <GL/glut.h> header file. */
/* #undef HAVE_GL_GLUT_H */

/* Define to 1 if you have the <GL/glu.h> header file. */
/* #undef HAVE_GL_GLU_H */

/* Define to 1 if you have the <GL/gl.h> header file. */
/* #undef HAVE_GL_GL_H */

/* Define to 1 if you have the <io.h> header file. */
#ifdef _WIN32
#define HAVE_IO_H 1
#else
/* #undef HAVE_IO_H */
#endif

/* Define to 1 if you have the `jbg_newlen' function. */
/* #undef HAVE_JBG_NEWLEN */

/* Define to 1 if you have the `mmap' function. */
/* #undef HAVE_MMAP */

/* Define to 1 if you have the <OpenGL/glu.h> header file. */
/* #undef HAVE_OPENGL_GLU_H */

/* Define to 1 if you have the <OpenGL/gl.h> header file. */
/* #undef HAVE_OPENGL_GL_H */

/* Define to 1 if you have the `setmode' function. */
/* #undef HAVE_SETMODE */

/* Define to 1 if you have the `snprintf' function. */
#define HAVE_SNPRINTF 1

/* Define to 1 if you have the <strings.h> header file. */
#if !defined(_WIN32)
#define HAVE_STRINGS_H 1
#else
/* #undef HAVE_STRINGS_H */
#endif

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#if !defined(_WIN32)
#define HAVE_UNISTD_H 1
#else
/* #undef HAVE_UNISTD_H */
#endif

/* 8/12 bit libjpeg dual mode enabled — disabled in single-TU build to avoid
   duplicate static symbols when tif_jpeg.c is included twice */
/* #undef JPEG_DUAL_MODE_8_12 */

/* 8/12 bit dual mode JPEG built into libjpeg-turbo 3.0+ */
/* #undef HAVE_JPEGTURBO_DUAL_MODE_8_12 */

/* Support LERC compression */
/* #undef LERC_SUPPORT */

/* Define to 1 when building a static libtiff with LERC enabled. */
/* #undef LERC_STATIC */

/* 12bit libjpeg primary include file with path */
/* #undef LIBJPEG_12_PATH */

/* Support LZMA2 compression */
/* #undef LZMA_SUPPORT */

/* Name of package */
/* #undef PACKAGE */

/* Define to the address where bug reports for this package should be sent. */
/* #undef PACKAGE_BUGREPORT */

/* Define to the full name of this package. */
/* #undef PACKAGE_NAME */

/* Define to the one symbol short name of this package. */
/* #undef PACKAGE_TARNAME */

/* Define to the home page for this package. */
/* #undef PACKAGE_URL */

/* The size of `size_t', as computed by sizeof. */
#if defined(_WIN64) || defined(__LP64__) || defined(_LP64) || defined(__arm64__) || defined(__aarch64__)
#define SIZEOF_SIZE_T 8
#else
#define SIZEOF_SIZE_T 4
#endif

/* Default size of the strip in bytes (when strip chopping enabled) */
#define STRIP_SIZE_DEFAULT 8192

/* Maximum number of TIFF IFDs that libtiff can iterate through in a file. */
#define TIFF_MAX_DIR_COUNT 1048576

/* define to use win32 IO system */
#ifdef _WIN32
#define USE_WIN32_FILEIO 1
#else
/* #undef USE_WIN32_FILEIO */
#endif

/* Support webp compression */
#define WEBP_SUPPORT 1

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__)
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define WORDS_BIGENDIAN 1
#else
#define WORDS_BIGENDIAN 0
#endif
#elif defined(__BIG_ENDIAN__)
#define WORDS_BIGENDIAN 1
#else
#define WORDS_BIGENDIAN 0
#endif

/* Support zstd compression */
/* #undef ZSTD_SUPPORT */

/* Enable large inode numbers on Mac OS X 10.5. */
#ifndef _DARWIN_USE_64_BIT_INODE
#define _DARWIN_USE_64_BIT_INODE 1
#endif

/* Number of bits in a file offset, on hosts where this is settable. */
/* #undef _FILE_OFFSET_BITS */

/* Define to 1 to make fseeko visible on some hosts (e.g. glibc 2.2). */
/* #undef _LARGEFILE_SOURCE */

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */

#if !defined(__MINGW32__)
#define TIFF_SIZE_FORMAT "zu"
#endif
#if SIZEOF_SIZE_T == 8
#define TIFF_SSIZE_FORMAT PRId64
#if defined(__MINGW32__)
#define TIFF_SIZE_FORMAT PRIu64
#endif
#elif SIZEOF_SIZE_T == 4
#define TIFF_SSIZE_FORMAT PRId32
#if defined(__MINGW32__)
#define TIFF_SIZE_FORMAT PRIu32
#endif
#else
#error "Unsupported size_t size; please submit a bug report"
#endif

#endif /* _TIF_CONFIG_ */
