#include <dr_libs/dr_libs.h>

#if YUP_MODULE_AVAILABLE_opus_library && YUP_AUDIO_FORMAT_OPUS
#include <opus_library/opus_library.h>
#endif

#if YUP_MODULE_AVAILABLE_flac_library && YUP_AUDIO_FORMAT_FLAC
#include <flac_library/flac_library.h>
#endif

#include "yup_audio_formats/yup_AudioFormatManager.cpp"
#include "yup_audio_formats/yup_WaveAudioFormat.cpp"
#include "yup_audio_formats/yup_Mp3AudioFormat.cpp"

#if YUP_MODULE_AVAILABLE_opus_library && YUP_AUDIO_FORMAT_OPUS
#include "yup_audio_formats/yup_OpusAudioFormat.cpp"
#endif

#if YUP_MODULE_AVAILABLE_flac_library && YUP_AUDIO_FORMAT_FLAC
#include "yup_audio_formats/yup_FlacAudioFormat.cpp"
#endif
