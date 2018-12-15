#include <alsa/asoundlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "alsa_capture.h"

int alsa_capture_open(struct alsa_capture* capture, int devid) {
  int err;
  // int channel;
  char device_name[10];

  sprintf(device_name, "hw:%d,0", devid);

  if ((err = snd_pcm_open(&capture->handle, device_name, SND_PCM_STREAM_CAPTURE,
                          0)) < 0) {
    fprintf(stderr, "cannot open audio device %s (%s)\n", device_name,
            snd_strerror(err));
    return -1;
  }

  if ((err = snd_pcm_hw_params_malloc(&capture->hw_params)) < 0) {
    fprintf(stderr, "cannot allocate hardware parameter structure (%s)\n",
            snd_strerror(err));
    return -1;
  }

  memset(capture->hw_params, 0, snd_pcm_hw_params_sizeof());

  if ((err = snd_pcm_hw_params_any(capture->handle, capture->hw_params)) < 0) {
    fprintf(stderr, "cannot initialize hardware parameter structure (%s)\n",
            snd_strerror(err));
    return -1;
  }
  snd_pcm_hw_params_get_channels_min(capture->hw_params, &capture->channel);

  if ((err = snd_pcm_hw_params_set_access(capture->handle, capture->hw_params,
                                          SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
    fprintf(stderr, "cannot set access type (%s)\n", snd_strerror(err));
    return -1;
  }

  if ((err = snd_pcm_hw_params_set_format(capture->handle, capture->hw_params,
                                          capture->format)) < 0) {
    fprintf(stderr, "cannot set sample format (%s)\n", snd_strerror(err));
    return -1;
  }

  if ((err = snd_pcm_hw_params_set_rate_near(
           capture->handle, capture->hw_params, &capture->sample_rate, 0)) <
      0) {
    fprintf(stderr, "cannot set sample rate (%s)\n", snd_strerror(err));
    return -1;
  }

  if ((err = snd_pcm_hw_params_set_channels(capture->handle, capture->hw_params,
                                            capture->channel)) < 0) {
    fprintf(stderr, "cannot set channel count (%s)\n", snd_strerror(err));
    return -1;
  }

  if ((err = snd_pcm_hw_params(capture->handle, capture->hw_params)) < 0) {
    fprintf(stderr, "cannot set parameters (%s)\n", snd_strerror(err));
    return -1;
  }

  snd_pcm_hw_params_get_period_size(capture->hw_params, &capture->period_size,
                                    0);
  snd_pcm_hw_params_get_buffer_size(capture->hw_params, &capture->buffer_size);
  if (capture->period_size == capture->buffer_size) {
    fprintf(stderr, "Can't use period equal to buffer size(%lu == %lu)\n",
            capture->period_size, capture->buffer_size);
    return -1;
  }
  printf("capture period_size: %lu, buffer_size: %lu\n", capture->period_size,
         capture->buffer_size);

  snd_pcm_hw_params_free(capture->hw_params);

  if ((err = snd_pcm_prepare(capture->handle)) < 0) {
    fprintf(stderr, "cannot prepare audio interface for use (%s)\n",
            snd_strerror(err));
    return -1;
  }
  capture->dev_id = devid;
  printf("alsa capture open.\n");
  return 0;
}

int alsa_capture_doing(struct alsa_capture* capture, void* buff) {
  int err = 0;
  int remain_len = capture->buffer_frames;
  do {
    err = snd_pcm_readi(capture->handle, buff, remain_len);
    if (err != remain_len) {
      if (err < 0) {
        if (err == -EAGAIN) {
          /* Apparently snd_pcm_recover() doesn't handle this case - does it
           * assume snd_pcm_wait() above? */
          return 0;
        }
        err = snd_pcm_recover(capture->handle, err, 0);
        if (err < 0) {
          /* Hmm, not much we can do - abort */
          fprintf(stderr, "ALSA read failed (unrecoverable): %s\n",
                  snd_strerror(err));
          return err;
        }
        err = snd_pcm_readi(capture->handle, buff, remain_len);
        if (err < 0) {
          fprintf(stderr, "read from audio interface failed(%s)\n",
                  snd_strerror(err));
          return err;
        }
      }
    }
    remain_len -= err;
  } while (remain_len > 0);

  // printf("alsa_capture_doing: %d\n", capture->buffer_frames - remain_len);
  return capture->buffer_frames - remain_len;
}

void alsa_capture_done(struct alsa_capture* capture) {
  if (capture->handle) {
    snd_pcm_drain(capture->handle);
    snd_pcm_close(capture->handle);
    capture->handle = NULL;
    printf("alsa capture close.\n");
  }
}
