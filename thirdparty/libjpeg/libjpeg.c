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

#include <stddef.h>
#include <stdio.h>

#define JPEG_INTERNALS
#include "upstream/src/jpeglib.h"
#undef JPEG_INTERNALS

#include "libjpeg.h"

static void yup_jpeg_unsupported_data_precision (j_common_ptr cinfo)
{
    ERREXIT (cinfo, JERR_NOT_COMPILED);
}

GLOBAL (void)
j12init_c_main_controller (j_compress_ptr cinfo, boolean need_full_buffer)
{
    (void) need_full_buffer;
    yup_jpeg_unsupported_data_precision ((j_common_ptr) cinfo);
}

GLOBAL (void)
j16init_c_main_controller (j_compress_ptr cinfo, boolean need_full_buffer)
{
    (void) need_full_buffer;
    yup_jpeg_unsupported_data_precision ((j_common_ptr) cinfo);
}

GLOBAL (void)
j12init_c_prep_controller (j_compress_ptr cinfo, boolean need_full_buffer)
{
    (void) need_full_buffer;
    yup_jpeg_unsupported_data_precision ((j_common_ptr) cinfo);
}

GLOBAL (void)
j16init_c_prep_controller (j_compress_ptr cinfo, boolean need_full_buffer)
{
    (void) need_full_buffer;
    yup_jpeg_unsupported_data_precision ((j_common_ptr) cinfo);
}

GLOBAL (void)
j12init_c_coef_controller (j_compress_ptr cinfo, boolean need_full_buffer)
{
    (void) need_full_buffer;
    yup_jpeg_unsupported_data_precision ((j_common_ptr) cinfo);
}

GLOBAL (void)
j12init_color_converter (j_compress_ptr cinfo)
{
    yup_jpeg_unsupported_data_precision ((j_common_ptr) cinfo);
}

GLOBAL (void)
j16init_color_converter (j_compress_ptr cinfo)
{
    yup_jpeg_unsupported_data_precision ((j_common_ptr) cinfo);
}

GLOBAL (void)
j12init_downsampler (j_compress_ptr cinfo)
{
    yup_jpeg_unsupported_data_precision ((j_common_ptr) cinfo);
}

GLOBAL (void)
j16init_downsampler (j_compress_ptr cinfo)
{
    yup_jpeg_unsupported_data_precision ((j_common_ptr) cinfo);
}

GLOBAL (void)
j12init_forward_dct (j_compress_ptr cinfo)
{
    yup_jpeg_unsupported_data_precision ((j_common_ptr) cinfo);
}

GLOBAL (void)
j12init_c_diff_controller (j_compress_ptr cinfo, boolean need_full_buffer)
{
    (void) need_full_buffer;
    yup_jpeg_unsupported_data_precision ((j_common_ptr) cinfo);
}

GLOBAL (void)
j16init_c_diff_controller (j_compress_ptr cinfo, boolean need_full_buffer)
{
    (void) need_full_buffer;
    yup_jpeg_unsupported_data_precision ((j_common_ptr) cinfo);
}

GLOBAL (void)
j12init_lossless_compressor (j_compress_ptr cinfo)
{
    yup_jpeg_unsupported_data_precision ((j_common_ptr) cinfo);
}

GLOBAL (void)
j16init_lossless_compressor (j_compress_ptr cinfo)
{
    yup_jpeg_unsupported_data_precision ((j_common_ptr) cinfo);
}

GLOBAL (void)
j12init_d_main_controller (j_decompress_ptr cinfo, boolean need_full_buffer)
{
    (void) need_full_buffer;
    yup_jpeg_unsupported_data_precision ((j_common_ptr) cinfo);
}

GLOBAL (void)
j16init_d_main_controller (j_decompress_ptr cinfo, boolean need_full_buffer)
{
    (void) need_full_buffer;
    yup_jpeg_unsupported_data_precision ((j_common_ptr) cinfo);
}

GLOBAL (void)
j12init_d_coef_controller (j_decompress_ptr cinfo, boolean need_full_buffer)
{
    (void) need_full_buffer;
    yup_jpeg_unsupported_data_precision ((j_common_ptr) cinfo);
}

GLOBAL (void)
j12init_d_post_controller (j_decompress_ptr cinfo, boolean need_full_buffer)
{
    (void) need_full_buffer;
    yup_jpeg_unsupported_data_precision ((j_common_ptr) cinfo);
}

GLOBAL (void)
j16init_d_post_controller (j_decompress_ptr cinfo, boolean need_full_buffer)
{
    (void) need_full_buffer;
    yup_jpeg_unsupported_data_precision ((j_common_ptr) cinfo);
}

GLOBAL (void)
j12init_inverse_dct (j_decompress_ptr cinfo)
{
    yup_jpeg_unsupported_data_precision ((j_common_ptr) cinfo);
}

GLOBAL (void)
j12init_upsampler (j_decompress_ptr cinfo)
{
    yup_jpeg_unsupported_data_precision ((j_common_ptr) cinfo);
}

GLOBAL (void)
j16init_upsampler (j_decompress_ptr cinfo)
{
    yup_jpeg_unsupported_data_precision ((j_common_ptr) cinfo);
}

GLOBAL (void)
j12init_color_deconverter (j_decompress_ptr cinfo)
{
    yup_jpeg_unsupported_data_precision ((j_common_ptr) cinfo);
}

GLOBAL (void)
j16init_color_deconverter (j_decompress_ptr cinfo)
{
    yup_jpeg_unsupported_data_precision ((j_common_ptr) cinfo);
}

GLOBAL (void)
j12init_1pass_quantizer (j_decompress_ptr cinfo)
{
    yup_jpeg_unsupported_data_precision ((j_common_ptr) cinfo);
}

GLOBAL (void)
j12init_2pass_quantizer (j_decompress_ptr cinfo)
{
    yup_jpeg_unsupported_data_precision ((j_common_ptr) cinfo);
}

GLOBAL (void)
j12init_merged_upsampler (j_decompress_ptr cinfo)
{
    yup_jpeg_unsupported_data_precision ((j_common_ptr) cinfo);
}

GLOBAL (void)
j12init_d_diff_controller (j_decompress_ptr cinfo, boolean need_full_buffer)
{
    (void) need_full_buffer;
    yup_jpeg_unsupported_data_precision ((j_common_ptr) cinfo);
}

GLOBAL (void)
j16init_d_diff_controller (j_decompress_ptr cinfo, boolean need_full_buffer)
{
    (void) need_full_buffer;
    yup_jpeg_unsupported_data_precision ((j_common_ptr) cinfo);
}

GLOBAL (void)
j12init_lossless_decompressor (j_decompress_ptr cinfo)
{
    yup_jpeg_unsupported_data_precision ((j_common_ptr) cinfo);
}

GLOBAL (void)
j16init_lossless_decompressor (j_decompress_ptr cinfo)
{
    yup_jpeg_unsupported_data_precision ((j_common_ptr) cinfo);
}

#define c_pass_type c_pass_type_jcmaster
#define huff_opt_pass huff_opt_pass_jcmaster
#define main_pass main_pass_jcmaster
#define my_comp_master my_comp_master_jcmaster
#define my_master_ptr my_master_ptr_jcmaster
#define output_pass output_pass_jcmaster
#include "upstream/src/jcapimin.c"
#include "upstream/src/jcapistd.c"
#include "jsamplecomp_undef.h"
#define my_coef_controller my_coef_controller_jccoefct
#define my_coef_ptr my_coef_ptr_jccoefct
#define start_iMCU_row start_iMCU_row_jccoefct
#include "upstream/src/jccoefct.c"
#undef start_iMCU_row
#undef my_coef_ptr
#undef my_coef_controller
#include "jsamplecomp_undef.h"

#define my_cconvert_ptr my_cconvert_ptr_jccolor
#include "upstream/src/jccolor.c"
#undef my_cconvert_ptr
#undef TABLE_SIZE
#include "jsamplecomp_undef.h"

#undef FIX
#include "upstream/src/jcdctmgr.c"
#include "jsamplecomp_undef.h"

#define savable_state savable_state_jchuff
#define huff_entropy_encoder huff_entropy_encoder_jchuff
#define huff_entropy_ptr huff_entropy_ptr_jchuff
#define working_state working_state_jchuff
#define dump_buffer dump_buffer_jchuff
#define emit_restart emit_restart_jchuff
#define finish_pass_gather finish_pass_gather_jchuff
#define finish_pass_huff finish_pass_huff_jchuff
#define flush_bits flush_bits_jchuff
#include "upstream/src/jchuff.c"
#undef emit_byte
#undef flush_bits
#undef finish_pass_huff
#undef finish_pass_gather
#undef emit_restart
#undef dump_buffer
#undef working_state
#undef huff_entropy_ptr
#undef huff_entropy_encoder
#undef savable_state

#include "upstream/src/jcicc.c"
#include "upstream/src/jcinit.c"

#define savable_state savable_state_jclhuff
#define working_state working_state_jclhuff
#define dump_buffer dump_buffer_jclhuff
#define emit_bits emit_bits_jclhuff
#define emit_restart emit_restart_jclhuff
#define finish_pass_gather finish_pass_gather_jclhuff
#define finish_pass_huff finish_pass_huff_jclhuff
#define flush_bits flush_bits_jclhuff
#include "upstream/src/jclhuff.c"
#undef emit_byte
#undef flush_bits
#undef finish_pass_huff
#undef finish_pass_gather
#undef emit_restart
#undef emit_bits
#undef dump_buffer
#undef working_state
#undef savable_state

#include "upstream/src/jclossls.c"

#define my_main_controller my_main_controller_jcmainct
#define my_main_ptr my_main_ptr_jcmainct
#define process_data_simple_main process_data_simple_main_jcmainct
#include "upstream/src/jcmainct.c"
#undef process_data_simple_main
#undef my_main_ptr
#undef my_main_controller
#include "jsamplecomp_undef.h"

#include "upstream/src/jcmarker.c"
#include "upstream/src/jcmaster.c"
#undef output_pass
#undef my_master_ptr
#undef my_comp_master
#undef main_pass
#undef huff_opt_pass
#undef c_pass_type
#include "upstream/src/jcomapi.c"
#include "upstream/src/jcparam.c"

#define dump_buffer dump_buffer_jcphuff
#define emit_bits emit_bits_jcphuff
#define emit_restart emit_restart_jcphuff
#define encode_mcu_AC_first encode_mcu_AC_first_jcphuff
#define encode_mcu_AC_refine encode_mcu_AC_refine_jcphuff
#define encode_mcu_DC_first encode_mcu_DC_first_jcphuff
#define encode_mcu_DC_refine encode_mcu_DC_refine_jcphuff
#define flush_bits flush_bits_jcphuff
#include "upstream/src/jcphuff.c"
#undef emit_byte
#undef flush_bits
#undef encode_mcu_DC_refine
#undef encode_mcu_DC_first
#undef encode_mcu_AC_refine
#undef encode_mcu_AC_first
#undef emit_restart
#undef emit_bits
#undef dump_buffer

#include "upstream/src/jcprepct.c"
#include "jsamplecomp_undef.h"
#include "upstream/src/jcsample.c"
#include "jsamplecomp_undef.h"

#define my_coef_controller my_coef_controller_jctrans
#define my_coef_ptr my_coef_ptr_jctrans
#define compress_output compress_output_jctrans
#define start_iMCU_row start_iMCU_row_jctrans
#define start_pass_coef start_pass_coef_jctrans
#include "upstream/src/jctrans.c"
#undef start_pass_coef
#undef start_iMCU_row
#undef compress_output
#undef my_coef_ptr
#undef my_coef_controller

#define my_diff_controller my_diff_controller_jcdiffct
#define my_diff_ptr my_diff_ptr_jcdiffct
#define compress_first_pass compress_first_pass_jcdiffct
#define compress_output compress_output_jcdiffct
#define start_iMCU_row start_iMCU_row_jcdiffct
#include "upstream/src/jsamplecomp.h"
#include "upstream/src/jcdiffct.c"
#undef start_iMCU_row
#undef compress_output
#undef compress_first_pass
#undef my_diff_ptr
#undef my_diff_controller
#include "jsamplecomp_undef.h"

#define my_decomp_master my_decomp_master_jdmaster
#define my_master_ptr my_master_ptr_jdmaster
#include "upstream/src/jdapimin.c"
#define my_coef_controller my_coef_controller_jdcoefct
#define my_coef_ptr my_coef_ptr_jdcoefct
#define my_main_controller my_main_controller_jdmainct
#define my_main_ptr my_main_ptr_jdmainct
#define start_iMCU_row start_iMCU_row_jdcoefct
#define set_wraparound_pointers set_wraparound_pointers_jdmainct
#include "upstream/src/jdapistd.c"
#include "upstream/src/jdatadst.c"
#include "upstream/src/jdatasrc.c"

#include "upstream/src/jdcoefct.c"
#undef start_iMCU_row
#undef my_coef_ptr
#undef my_coef_controller
#include "jsamplecomp_undef.h"

#undef SCALEBITS
#undef ONE_HALF
#undef FIX
#define my_cconvert_ptr my_cconvert_ptr_jdcolor
#define build_ycc_rgb_table build_ycc_rgb_table_jdcolor
#define dither_matrix dither_matrix_jdcolor
#define grayscale_convert grayscale_convert_jdcolor
#define null_convert null_convert_jdcolor
#define rgb_gray_convert rgb_gray_convert_jdcolor
#define rgb_rgb_convert rgb_rgb_convert_jdcolor
#include "jdefines.h"
#include "upstream/src/jdcolor.c"
#undef rgb_rgb_convert_internal
#undef gray_rgb_convert_internal
#undef ycc_rgb_convert_internal
#undef rgb_rgb_convert
#undef rgb_gray_convert
#undef null_convert
#undef grayscale_convert
#undef dither_matrix
#undef build_ycc_rgb_table
#undef my_cconvert_ptr
#undef TABLE_SIZE
#include "jsamplecomp_undef.h"

#undef FIX
#include "upstream/src/jddctmgr.c"
#include "jsamplecomp_undef.h"

#define savable_state savable_state_jdhuff
#define huff_entropy_decoder huff_entropy_decoder_jdhuff
#define huff_entropy_ptr huff_entropy_ptr_jdhuff
#define add_huff_table add_huff_table_jdhuff
#define std_huff_tables std_huff_tables_jdhuff
#include "upstream/src/jdhuff.c"
#undef std_huff_tables
#undef add_huff_table
#undef huff_entropy_ptr
#undef huff_entropy_decoder
#undef savable_state

#include "upstream/src/jdicc.c"

#define initial_setup initial_setup_jdinput
#define per_scan_setup per_scan_setup_jdinput
#include "upstream/src/jdinput.c"
#undef per_scan_setup
#undef initial_setup

#define extend_offset extend_offset_jdlhuff
#define extend_test extend_test_jdlhuff
#define lhuff_entropy_ptr lhuff_entropy_ptr_jdlhuff
#include "upstream/src/jdlhuff.c"
#undef lhuff_entropy_ptr
#undef extend_test
#undef extend_offset
#undef HUFF_EXTEND
#undef NEG_1
#undef AVOID_TABLES

#define noscale noscale_jdlossls
#define start_pass_lossless start_pass_lossless_jdlossls
#include "upstream/src/jsamplecomp.h"
#include "upstream/src/jdlossls.c"
#undef start_pass_lossless
#undef noscale
#include "jsamplecomp_undef.h"

#define process_data_simple_main process_data_simple_main_jdmainct
#define start_pass_main start_pass_main_jdmainct
#include "upstream/src/jsamplecomp.h"
#include "upstream/src/jdmainct.c"
#undef start_pass_main
#undef process_data_simple_main
#undef set_wraparound_pointers
#undef my_main_ptr
#undef my_main_controller
#include "jsamplecomp_undef.h"

#define JPEG_MARKER JPEG_MARKER_jdmarker
#define M_APP0 M_APP0_jdmarker
#define M_APP1 M_APP1_jdmarker
#define M_APP2 M_APP2_jdmarker
#define M_APP3 M_APP3_jdmarker
#define M_APP4 M_APP4_jdmarker
#define M_APP5 M_APP5_jdmarker
#define M_APP6 M_APP6_jdmarker
#define M_APP7 M_APP7_jdmarker
#define M_APP8 M_APP8_jdmarker
#define M_APP9 M_APP9_jdmarker
#define M_APP10 M_APP10_jdmarker
#define M_APP11 M_APP11_jdmarker
#define M_APP12 M_APP12_jdmarker
#define M_APP13 M_APP13_jdmarker
#define M_APP14 M_APP14_jdmarker
#define M_APP15 M_APP15_jdmarker
#define M_COM M_COM_jdmarker
#define M_DAC M_DAC_jdmarker
#define M_DHP M_DHP_jdmarker
#define M_DHT M_DHT_jdmarker
#define M_DNL M_DNL_jdmarker
#define M_DQT M_DQT_jdmarker
#define M_DRI M_DRI_jdmarker
#define M_EOI M_EOI_jdmarker
#define M_ERROR M_ERROR_jdmarker
#define M_EXP M_EXP_jdmarker
#define M_JPG M_JPG_jdmarker
#define M_JPG0 M_JPG0_jdmarker
#define M_JPG13 M_JPG13_jdmarker
#define M_RST0 M_RST0_jdmarker
#define M_RST1 M_RST1_jdmarker
#define M_RST2 M_RST2_jdmarker
#define M_RST3 M_RST3_jdmarker
#define M_RST4 M_RST4_jdmarker
#define M_RST5 M_RST5_jdmarker
#define M_RST6 M_RST6_jdmarker
#define M_RST7 M_RST7_jdmarker
#define M_SOF0 M_SOF0_jdmarker
#define M_SOF1 M_SOF1_jdmarker
#define M_SOF2 M_SOF2_jdmarker
#define M_SOF3 M_SOF3_jdmarker
#define M_SOF5 M_SOF5_jdmarker
#define M_SOF6 M_SOF6_jdmarker
#define M_SOF7 M_SOF7_jdmarker
#define M_SOF9 M_SOF9_jdmarker
#define M_SOF10 M_SOF10_jdmarker
#define M_SOF11 M_SOF11_jdmarker
#define M_SOF13 M_SOF13_jdmarker
#define M_SOF14 M_SOF14_jdmarker
#define M_SOF15 M_SOF15_jdmarker
#define M_SOI M_SOI_jdmarker
#define M_SOS M_SOS_jdmarker
#define M_TEM M_TEM_jdmarker
#define my_marker_ptr my_marker_ptr_jdmarker
#include "upstream/src/jdmarker.c"
#undef my_marker_ptr
#undef M_TEM
#undef M_SOS
#undef M_SOI
#undef M_SOF15
#undef M_SOF14
#undef M_SOF13
#undef M_SOF11
#undef M_SOF10
#undef M_SOF9
#undef M_SOF7
#undef M_SOF6
#undef M_SOF5
#undef M_SOF3
#undef M_SOF2
#undef M_SOF1
#undef M_SOF0
#undef M_RST7
#undef M_RST6
#undef M_RST5
#undef M_RST4
#undef M_RST3
#undef M_RST2
#undef M_RST1
#undef M_RST0
#undef M_JPG13
#undef M_JPG0
#undef M_JPG
#undef M_EXP
#undef M_ERROR
#undef M_EOI
#undef M_DRI
#undef M_DQT
#undef M_DNL
#undef M_DHT
#undef M_DHP
#undef M_DAC
#undef M_COM
#undef M_APP15
#undef M_APP14
#undef M_APP13
#undef M_APP12
#undef M_APP11
#undef M_APP10
#undef M_APP9
#undef M_APP8
#undef M_APP7
#undef M_APP6
#undef M_APP5
#undef M_APP4
#undef M_APP3
#undef M_APP2
#undef M_APP1
#undef M_APP0
#undef JPEG_MARKER

#include "upstream/src/jdmaster.c"
#undef my_master_ptr
#undef my_decomp_master

#undef SCALEBITS
#undef ONE_HALF
#undef FIX
#define build_ycc_rgb_table build_ycc_rgb_table_jdmerge
#define dither_matrix dither_matrix_jdmerge
#define is_big_endian is_big_endian_jdmerge
#include "jdefines.h"
#include "upstream/src/jsamplecomp.h"
#include "upstream/src/jdmerge.c"
#undef is_big_endian
#undef dither_matrix
#undef build_ycc_rgb_table
#include "jsamplecomp_undef.h"

#define decode_mcu_AC_first decode_mcu_AC_first_jdphuff
#define decode_mcu_AC_refine decode_mcu_AC_refine_jdphuff
#define decode_mcu_DC_first decode_mcu_DC_first_jdphuff
#define decode_mcu_DC_refine decode_mcu_DC_refine_jdphuff
#define extend_offset extend_offset_jdphuff
#define extend_test extend_test_jdphuff
#define phuff_entropy_ptr phuff_entropy_ptr_jdphuff
#undef HUFF_EXTEND
#undef NEG_1
#undef AVOID_TABLES
#include "upstream/src/jdphuff.c"
#undef phuff_entropy_ptr
#undef extend_test
#undef extend_offset
#undef decode_mcu_DC_refine
#undef decode_mcu_DC_first
#undef decode_mcu_AC_refine
#undef decode_mcu_AC_first
#undef HUFF_EXTEND
#undef NEG_1
#undef AVOID_TABLES

#include "upstream/src/jdpostct.c"
#include "jsamplecomp_undef.h"

#define my_upsampler my_upsampler_jdsample
#define my_upsample_ptr my_upsample_ptr_jdsample
#include "upstream/src/jdsample.c"
#undef my_upsample_ptr
#undef my_upsampler
#include "jsamplecomp_undef.h"

#include "upstream/src/jdtrans.c"

#define my_diff_controller my_diff_controller_jddiffct
#define my_diff_ptr my_diff_ptr_jddiffct
#define start_iMCU_row start_iMCU_row_jddiffct
#include "upstream/src/jsamplecomp.h"
#include "upstream/src/jddiffct.c"
#undef start_iMCU_row
#undef my_diff_ptr
#undef my_diff_controller
#include "jsamplecomp_undef.h"

#include "upstream/src/jerror.c"
#undef FIX
#undef DESCALE
#include "upstream/src/jfdctflt.c"
#include "jsamplecomp_undef.h"

#undef CONST_BITS
#undef FIX
#undef DESCALE
#include "upstream/src/jfdctfst.c"
#include "jsamplecomp_undef.h"

#undef CONST_BITS
#undef FIX
#undef DESCALE
#undef FIX_0_541196100
#undef MULTIPLY
#include "upstream/src/jfdctint.c"
#include "jsamplecomp_undef.h"
#undef FIX
#undef DESCALE
#include "upstream/src/jidctflt.c"
#include "jsamplecomp_undef.h"

#undef CONST_BITS
#undef FIX
#undef DESCALE
#undef DEQUANTIZE
#undef FIX_1_847759065
#undef MULTIPLY
#include "upstream/src/jidctfst.c"
#include "jsamplecomp_undef.h"

#undef CONST_BITS
#undef FIX
#undef DESCALE
#undef DEQUANTIZE
#undef FIX_1_847759065
#undef MULTIPLY
#include "upstream/src/jidctint.c"
#include "jsamplecomp_undef.h"
#undef FIX
#undef DESCALE
#undef DEQUANTIZE
#undef MULTIPLY
#include "upstream/src/jidctred.c"
#include "jsamplecomp_undef.h"
#include "upstream/src/jmemmgr.c"
#include "upstream/src/jmemnobs.c"
#include "upstream/src/jpeg_nbits.c"

#include "jdefines.h"
#include "upstream/src/jquant1.c"
#include "jsamplecomp_undef.h"

#define FSERROR FSERROR_jquant2
#define FSERRPTR FSERRPTR_jquant2
#define LOCFSERROR LOCFSERROR_jquant2
#define my_cquantize_ptr my_cquantize_ptr_jquant2
#define my_cquantizer my_cquantizer_jquant2
#include "upstream/src/jquant2.c"
#undef my_cquantizer
#undef my_cquantize_ptr
#undef LOCFSERROR
#undef FSERRPTR
#undef FSERROR
#include "jsamplecomp_undef.h"

#include "upstream/src/jutils.c"
#include "jsamplecomp_undef.h"

#if defined(WITH_SIMD) && (defined(__aarch64__) || defined(_M_ARM64) || defined(_M_ARM64EC) || defined(__ARM_NEON))
#include "upstream/simd/arm/jcgray-neon.c"
#include "upstream/simd/arm/jcphuff-neon.c"
#include "upstream/simd/arm/jcsample-neon.c"

#define jsimd_ycc_rgb_convert_neon_consts jsimd_ycc_rgb_convert_neon_consts_jdmerge
#include "jdefines.h"
#include "upstream/simd/arm/jdmerge-neon.c"
#undef jsimd_ycc_rgb_convert_neon_consts

#include "upstream/simd/arm/jdsample-neon.c"
#include "upstream/simd/arm/jfdctfst-neon.c"
#include "upstream/simd/arm/jidctred-neon.c"
#include "upstream/simd/arm/jquanti-neon.c"

#include "jdefines.h"
#include "upstream/simd/arm/jccolor-neon.c"

#undef DESCALE_P1
#undef DESCALE_P2
#undef F_0_298
#undef F_0_541
#include "upstream/simd/arm/jidctint-neon.c"
#include "upstream/simd/arm/jidctfst-neon.c"

#include "jdefines.h"
#include "upstream/simd/arm/jdcolor-neon.c"
#undef DESCALE_P1
#undef DESCALE_P2
#undef F_0_298
#undef F_0_541
#include "upstream/simd/arm/jfdctint-neon.c"

#undef BIT_BUF_SIZE
#undef EMIT_BYTE
#undef FLUSH
#undef PUT_AND_FLUSH
#undef PUT_BITS
#undef PUT_CODE
#define savable_state savable_state_jchuff_neon
#define working_state working_state_jchuff_neon
#if defined(__aarch64__) || defined(_M_ARM64) || defined(_M_ARM64EC)
#include "upstream/simd/arm/aarch64/jchuff-neon.c"
#include "jdefines.h"
#include "upstream/simd/arm/aarch64/jsimd.c"
#else
#include "upstream/simd/arm/aarch32/jchuff-neon.c"
#include "jdefines.h"
#include "upstream/simd/arm/aarch32/jsimd.c"
#endif
#undef working_state
#undef savable_state
#undef PUT_CODE
#undef PUT_BITS
#undef PUT_AND_FLUSH
#undef FLUSH
#undef EMIT_BYTE
#undef BIT_BUF_SIZE
#endif
