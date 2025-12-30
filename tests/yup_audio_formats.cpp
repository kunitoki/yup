#if YUP_MODULE_AVAILABLE_dr_libs && (YUP_AUDIO_FORMAT_WAVE || YUP_AUDIO_FORMAT_MP3)
#include <dr_libs/dr_libs.h>
#endif

#if YUP_MODULE_AVAILABLE_opus_library && YUP_AUDIO_FORMAT_OPUS
#include <opus_library/opus_library.h>
#endif

#if YUP_MODULE_AVAILABLE_flac_library && YUP_AUDIO_FORMAT_FLAC
#include <flac_library/flac_library.h>
#endif

#include "yup_audio_formats/yup_AudioFormatManager.cpp"

#if YUP_MODULE_AVAILABLE_dr_libs && YUP_AUDIO_FORMAT_WAVE
#include "yup_audio_formats/yup_WaveAudioFormat.cpp"
#endif

#if YUP_MODULE_AVAILABLE_dr_libs && YUP_AUDIO_FORMAT_MP3
#include "yup_audio_formats/yup_Mp3AudioFormat.cpp"
#endif

#if YUP_MODULE_AVAILABLE_opus_library && YUP_AUDIO_FORMAT_OPUS
#include "yup_audio_formats/yup_OpusAudioFormat.cpp"
#endif

#if YUP_MODULE_AVAILABLE_flac_library && YUP_AUDIO_FORMAT_FLAC
#include "yup_audio_formats/yup_FlacAudioFormat.cpp"
#endif
