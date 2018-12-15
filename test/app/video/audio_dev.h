// by wangh
#ifndef AUDIO_DEV_H
#define AUDIO_DEV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <alloca.h>
#include <alsa/asoundlib.h>
#include <libavformat/avformat.h>

typedef struct {
  enum AVSampleFormat fmt;
  int64_t wanted_channel_layout;
  int wanted_sample_rate;
} WantedAudioParams;

typedef struct AudioParams {
  int freq;
  int channels;
  int64_t channel_layout;
  enum AVSampleFormat fmt;
  int frame_size;
  int bytes_per_sec;

  WantedAudioParams wanted_params;
  snd_pcm_t* pcm_handle;
  uint32_t pcm_samples;
  snd_pcm_format_t pcm_format;
  int audio_hw_buf_size;
} AudioParams;

int audio_dev_init();
void audio_dev_deinit();
int audio_params_changed(enum AVSampleFormat fmt,
                         int64_t wanted_channel_layout,
                         int wanted_sample_rate);
int audio_dev_open(enum AVSampleFormat fmt,
                   int64_t wanted_channel_layout,
                   int wanted_sample_rate,
                   AudioParams** audio_hw_params);
void audio_dev_close();
int audio_dev_write(uint8_t* buf, snd_pcm_sframes_t* frames_left);

#ifdef __cplusplus
}
#endif

#endif  // AUDIO_DEV_H
