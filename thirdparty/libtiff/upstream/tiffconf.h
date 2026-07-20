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

#ifndef _TIFFCONF_
#define _TIFFCONF_

#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>

/* Signed 8-bit type */
#define TIFF_INT8_T int8_t

/* Unsigned 8-bit type */
#define TIFF_UINT8_T uint8_t

/* Signed 16-bit type */
#define TIFF_INT16_T int16_t

/* Unsigned 16-bit type */
#define TIFF_UINT16_T uint16_t

/* Signed 32-bit type */
#define TIFF_INT32_T int32_t

/* Unsigned 32-bit type */
#define TIFF_UINT32_T uint32_t

/* Signed 64-bit type */
#define TIFF_INT64_T int64_t

/* Unsigned 64-bit type */
#define TIFF_UINT64_T uint64_t

/* Signed size type */
#if defined(_WIN64) || defined(__LP64__) || defined(_LP64) || defined(__arm64__) || defined(__aarch64__)
#define TIFF_SSIZE_T int64_t
#else
#define TIFF_SSIZE_T int32_t
#endif

/* Define as 0 or 1 according to the floating point format supported by the
   machine */
#define HAVE_IEEEFP 1

/* The concept of HOST_FILLORDER is broken. Since libtiff 4.5.1
 * this macro will always be hardcoded to FILLORDER_LSB2MSB on all
 * architectures, to reflect past long behavior of doing so on x86 architecture.
 * Note however that the default FillOrder used by libtiff is FILLORDER_MSB2LSB,
 * as mandated per the TIFF specification.
 * The influence of HOST_FILLORDER is only when passing the 'H' mode in
 * TIFFOpen().
 * You should NOT rely on this macro to decide the CPU endianness!
 * This macro will be removed in libtiff 4.6
 */
#define HOST_FILLORDER FILLORDER_LSB2MSB

/* Native cpu byte order: 1 if big-endian (Motorola) or 0 if little-endian
   (Intel) */
#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__)
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define HOST_BIGENDIAN 1
#else
#define HOST_BIGENDIAN 0
#endif
#elif defined(__BIG_ENDIAN__)
#define HOST_BIGENDIAN 1
#else
#define HOST_BIGENDIAN 0
#endif

/* Support CCITT Group 3 & 4 algorithms */
#define CCITT_SUPPORT 1

/* Support JPEG compression (requires IJG JPEG library) */
#define JPEG_SUPPORT 1

/* Support JBIG compression (requires JBIG-KIT library) */
/* #undef JBIG_SUPPORT */

/* Support LERC compression */
/* #undef LERC_SUPPORT */

/* Support LogLuv high dynamic range encoding */
#define LOGLUV_SUPPORT 1

/* Support LZW algorithm */
#define LZW_SUPPORT 1

/* Support NeXT 2-bit RLE algorithm */
#define NEXT_SUPPORT 1

/* Support Old JPEG compression (read contrib/ojpeg/README first! Compilation
   fails with unpatched IJG JPEG library) */
#define OJPEG_SUPPORT 1

/* Support Macintosh PackBits algorithm */
#define PACKBITS_SUPPORT 1

/* Support Pixar log-format algorithm (requires Zlib) */
#define PIXARLOG_SUPPORT 1

/* Support ThunderScan 4-bit RLE algorithm */
#define THUNDER_SUPPORT 1

/* Support Deflate compression */
#define ZIP_SUPPORT 1

/* Support libdeflate enhanced compression */
/* #undef LIBDEFLATE_SUPPORT */

/* Support strip chopping (whether or not to convert single-strip uncompressed
   images to multiple strips of ~8Kb to reduce memory usage) */
#define STRIPCHOP_DEFAULT TIFF_STRIPCHOP

/* Enable SubIFD tag (330) support */
#define SUBIFD_SUPPORT 1

/* Treat extra sample as alpha (default enabled). The RGBA interface will
   treat a fourth sample with no EXTRASAMPLE_ value as being ASSOCALPHA. Many
   packages produce RGBA files but don't mark the alpha properly. */
#define DEFAULT_EXTRASAMPLE_AS_ALPHA 1

/* Pick up YCbCr subsampling info from the JPEG data stream to support files
   lacking the tag (default enabled). */
#define CHECK_JPEG_YCBCR_SUBSAMPLING 1

/* Support MS MDI magic number files as TIFF */
#define MDI_SUPPORT 1

/*
 * Feature support definitions.
 * XXX: These macros are obsoleted. Don't use them in your apps!
 * Macros stays here for backward compatibility and should be always defined.
 */
#define COLORIMETRY_SUPPORT 1
#define YCBCR_SUPPORT 1
#define CMYK_SUPPORT 1
#define ICC_SUPPORT 1
#define PHOTOSHOP_SUPPORT 1
#define IPTC_SUPPORT 1

#endif /* _TIFFCONF_ */
