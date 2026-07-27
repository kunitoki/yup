/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

#define OPUS_BUILD 1
#define USE_ALLOCA 1

#include "opus_library.h"

#include <opus_library/silk/CNG.c>
#include <opus_library/silk/code_signs.c>
#include <opus_library/silk/init_decoder.c>
#include <opus_library/silk/decode_core.c>
#include <opus_library/silk/decode_frame.c>
#include <opus_library/silk/decode_parameters.c>
#include <opus_library/silk/decode_indices.c>
#include <opus_library/silk/decode_pulses.c>
#include <opus_library/silk/decoder_set_fs.c>
#include <opus_library/silk/dec_API.c>
#include <opus_library/silk/enc_API.c>
#include <opus_library/silk/encode_indices.c>
#include <opus_library/silk/encode_pulses.c>
#include <opus_library/silk/gain_quant.c>
#include <opus_library/silk/interpolate.c>
#include <opus_library/silk/LP_variable_cutoff.c>
#include <opus_library/silk/NLSF_decode.c>
#include <opus_library/silk/NSQ.c>
#include <opus_library/silk/NSQ_del_dec.c>
#include <opus_library/silk/PLC.c>
#include <opus_library/silk/shell_coder.c>
#include <opus_library/silk/tables_gain.c>
#include <opus_library/silk/tables_LTP.c>
#include <opus_library/silk/tables_NLSF_CB_NB_MB.c>
#include <opus_library/silk/tables_NLSF_CB_WB.c>
#include <opus_library/silk/tables_other.c>
#include <opus_library/silk/tables_pitch_lag.c>
#include <opus_library/silk/tables_pulses_per_block.c>
#include <opus_library/silk/VAD.c>
#include <opus_library/silk/control_audio_bandwidth.c>
#include <opus_library/silk/quant_LTP_gains.c>
#include <opus_library/silk/VQ_WMat_EC.c>
#include <opus_library/silk/HP_variable_cutoff.c>
#include <opus_library/silk/NLSF_encode.c>
#include <opus_library/silk/NLSF_VQ.c>
#include <opus_library/silk/NLSF_unpack.c>
#include <opus_library/silk/NLSF_del_dec_quant.c>
#include <opus_library/silk/process_NLSFs.c>
#include <opus_library/silk/stereo_LR_to_MS.c>
#include <opus_library/silk/stereo_MS_to_LR.c>
#include <opus_library/silk/check_control_input.c>
#include <opus_library/silk/control_SNR.c>
#include <opus_library/silk/init_encoder.c>
#include <opus_library/silk/control_codec.c>
#include <opus_library/silk/A2NLSF.c>
#include <opus_library/silk/ana_filt_bank_1.c>
#include <opus_library/silk/biquad_alt.c>
#include <opus_library/silk/bwexpander_32.c>
#include <opus_library/silk/bwexpander.c>
#include <opus_library/silk/debug.c>
#include <opus_library/silk/decode_pitch.c>
#include <opus_library/silk/inner_prod_aligned.c>
#include <opus_library/silk/lin2log.c>
#include <opus_library/silk/log2lin.c>
#include <opus_library/silk/LPC_analysis_filter.c>
#include <opus_library/silk/LPC_inv_pred_gain.c>
#undef QA
#include <opus_library/silk/table_LSF_cos.c>
#include <opus_library/silk/NLSF2A.c>
#undef QA
#include <opus_library/silk/NLSF_stabilize.c>
#include <opus_library/silk/NLSF_VQ_weights_laroia.c>
#include <opus_library/silk/pitch_est_tables.c>
#include <opus_library/silk/resampler.c>
#include <opus_library/silk/resampler_down2_3.c>
#include <opus_library/silk/resampler_down2.c>
#include <opus_library/silk/resampler_private_AR2.c>
#include <opus_library/silk/resampler_private_down_FIR.c>
#include <opus_library/silk/resampler_private_IIR_FIR.c>
#include <opus_library/silk/resampler_private_up2_HQ.c>
#include <opus_library/silk/resampler_rom.c>
#include <opus_library/silk/sigm_Q15.c>
#include <opus_library/silk/sort.c>
#include <opus_library/silk/sum_sqr_shift.c>
#include <opus_library/silk/stereo_decode_pred.c>
#include <opus_library/silk/stereo_encode_pred.c>
#include <opus_library/silk/stereo_find_predictor.c>
#include <opus_library/silk/stereo_quant_pred.c>
#include <opus_library/silk/LPC_fit.c>

#if defined(OPUS_ARM_PRESUME_NEON_INTR)
#include <opus_library/silk/arm/biquad_alt_neon_intr.c>
#include <opus_library/silk/arm/LPC_inv_pred_gain_neon_intr.c>
#include <opus_library/silk/arm/NSQ_del_dec_neon_intr.c>
#include <opus_library/silk/arm/NSQ_neon.c>
#if defined(FIXED_POINT)
#include <opus_library/silk/fixed/arm/warped_autocorrelation_FIX_neon_intr.c>
#endif
#endif

#include <opus_library/silk/float/apply_sine_window_FLP.c>
#include <opus_library/silk/float/corrMatrix_FLP.c>
#include <opus_library/silk/float/encode_frame_FLP.c>
#include <opus_library/silk/float/find_LPC_FLP.c>
#include <opus_library/silk/float/find_LTP_FLP.c>
#include <opus_library/silk/float/find_pitch_lags_FLP.c>
#include <opus_library/silk/float/find_pred_coefs_FLP.c>
#include <opus_library/silk/float/LPC_analysis_filter_FLP.c>
#include <opus_library/silk/float/LTP_analysis_filter_FLP.c>
#include <opus_library/silk/float/LTP_scale_ctrl_FLP.c>
#include <opus_library/silk/float/noise_shape_analysis_FLP.c>
#include <opus_library/silk/float/process_gains_FLP.c>
#include <opus_library/silk/float/regularize_correlations_FLP.c>
#include <opus_library/silk/float/residual_energy_FLP.c>
#include <opus_library/silk/float/warped_autocorrelation_FLP.c>
#include <opus_library/silk/float/wrappers_FLP.c>
#include <opus_library/silk/float/autocorrelation_FLP.c>
#include <opus_library/silk/float/burg_modified_FLP.c>
#include <opus_library/silk/float/bwexpander_FLP.c>
#include <opus_library/silk/float/energy_FLP.c>
#include <opus_library/silk/float/inner_product_FLP.c>
#include <opus_library/silk/float/k2a_FLP.c>
#include <opus_library/silk/float/LPC_inv_pred_gain_FLP.c>
#include <opus_library/silk/float/pitch_analysis_core_FLP.c>
#include <opus_library/silk/float/scale_copy_vector_FLP.c>
#include <opus_library/silk/float/scale_vector_FLP.c>
#include <opus_library/silk/float/schur_FLP.c>
#include <opus_library/silk/float/sort_FLP.c>
