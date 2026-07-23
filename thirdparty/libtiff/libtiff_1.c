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

#include "libtiff.h"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-qual"
#pragma clang diagnostic ignored "-Wcast-align"
#pragma clang diagnostic ignored "-Wcomma"
#pragma clang diagnostic ignored "-Wconditional-uninitialized"
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Wdouble-promotion"
#pragma clang diagnostic ignored "-Wduplicate-decl-specifier"
#pragma clang diagnostic ignored "-Wextra-semi"
#pragma clang diagnostic ignored "-Wflexible-array-extensions"
#pragma clang diagnostic ignored "-Wfloat-conversion"
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#pragma clang diagnostic ignored "-Wgcc-compat"
#pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#pragma clang diagnostic ignored "-Wimplicit-int-conversion"
#pragma clang diagnostic ignored "-Wincompatible-pointer-types-discards-qualifiers"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#pragma clang diagnostic ignored "-Wmissing-noreturn"
#pragma clang diagnostic ignored "-Wmissing-variable-declarations"
#pragma clang diagnostic ignored "-Wpadded"
#pragma clang diagnostic ignored "-Wpedantic"
#pragma clang diagnostic ignored "-Wreserved-macro-identifier"
#pragma clang diagnostic ignored "-Wshadow"
#pragma clang diagnostic ignored "-Wshift-sign-overflow"
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#pragma clang diagnostic ignored "-Wsign-compare"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wstrict-prototypes"
#pragma clang diagnostic ignored "-Wtautological-constant-out-of-range-compare"
#pragma clang diagnostic ignored "-Wunreachable-code-break"
#pragma clang diagnostic ignored "-Wunused-macros"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wused-but-marked-unused"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
#elif _MSC_VER
#pragma warning (push)
#pragma warning (disable : 4090)
#pragma warning (disable : 4100)
#pragma warning (disable : 4127)
#pragma warning (disable : 4244)
#pragma warning (disable : 4267)
#pragma warning (disable : 4305)
#pragma warning (disable : 4334)
#pragma warning (disable : 4389)
#pragma warning (disable : 4456)
#pragma warning (disable : 4457)
#pragma warning (disable : 4701)
#pragma warning (disable : 4703)
#endif

/* Core library */
#include "upstream/tif_aux.c"
#include "upstream/tif_close.c"
#include "upstream/tif_codec.c"
#include "upstream/tif_color.c"
#include "upstream/tif_compress.c"
#include "upstream/tif_dir.c"
#include "upstream/tif_dirinfo.c"
#include "upstream/tif_dirread.c"
#include "upstream/tif_dirwrite.c"
#include "upstream/tif_dumpmode.c"
#include "upstream/tif_error.c"
#include "upstream/tif_extension.c"
#include "upstream/tif_flush.c"
#include "upstream/tif_getimage.c"
#include "upstream/tif_hash_set.c"
#include "upstream/tif_open.c"
/* tif_predict.c → moved to libtiff_2.c (REPEAT4 conflict) */
#include "upstream/tif_print.c"
#include "upstream/tif_read.c"
#include "upstream/tif_strip.c"
#include "upstream/tif_swab.c"
#include "upstream/tif_tile.c"
#include "upstream/tif_version.c"
#include "upstream/tif_warning.c"
#include "upstream/tif_write.c"

/* Compression codecs — clean with minor #undef between conflicting macros */
#include "upstream/tif_fax3.c"
#include "upstream/tif_fax3sm.c"
#include "upstream/tif_jbig.c"
#include "upstream/tif_jpeg.c"
#include "upstream/tif_lerc.c"
/* tif_luv.c → moved to libtiff_2.c (PACK, DecoderState, multiply_ms) */
#include "upstream/tif_lzma.c"
#include "upstream/tif_lzw.c"
#include "upstream/tif_next.c"
#include "upstream/tif_ojpeg.c"
#include "upstream/tif_packbits.c"
#include "upstream/tif_pixarlog.c"
/* tif_thunder.c → moved to libtiff_2.c (SETPIXEL conflict) */
#include "upstream/tif_webp.c"
#include "upstream/tif_zip.c"
#include "upstream/tif_zstd.c"

/* Platform-specific I/O */
#if defined(_WIN32)
#include "upstream/tif_win32.c"
#else
#include "upstream/tif_unix.c"
#endif

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#elif _MSC_VER
#pragma warning (pop)
#endif
