#include "audio_dev.h"
#include <pthread.h>

#define AUDIO_MIN_BUFFER_SIZE 512
#define AUDIO_MAX_CALLBACKS_PER_SEC 30

static int ALSA_finalize_hardware(snd_pcm_t* pcm_handle,
                                  uint32_t* samples,
                                  int sample_size,
                                  snd_pcm_hw_params_t* hwparams) {
  int status;
  snd_pcm_uframes_t bufsize;
  int ret = 0;
  /* "set" the hardware with the desired parameters */
  status = snd_pcm_hw_params(pcm_handle, hwparams);
  if (status < 0) {
    av_log(NULL, AV_LOG_ERROR, "Couldn't set pcm_hw_params: %s\n",
           snd_strerror(status));
    ret = -1;
    goto err;
  }

  /* Get samples for the actual buffer size */
  status = snd_pcm_hw_params_get_buffer_size(hwparams, &bufsize);
  if (status < 0) {
    av_log(NULL, AV_LOG_ERROR, "Couldn't get buffer size: %s\n",
           snd_strerror(status));
    ret = -1;
    goto err;
  }
  if (bufsize != (*samples) * sample_size) {
    fprintf(stderr, "error, bufsize != (*samples) * %d; %lu != %d*%d\n",
            sample_size, bufsize, *samples, sample_size);
    ret = -1;
    goto err;
  }

/* FIXME: Is this safe to do? */
//*samples = bufsize / 2;
err:
  /* This is useful for debugging */
  if (1) {
    snd_pcm_uframes_t persize = 0;
    unsigned int periods = 0;
    snd_pcm_hw_params_get_period_size(hwparams, &persize, NULL);
    snd_pcm_hw_params_get_periods(hwparams, &periods, NULL);
    fprintf(stderr,
            "ALSA: period size = %ld, periods = %u, buffer size = %lu\n",
            persize, periods, bufsize);
  }
  return ret;
}

static int ALSA_set_period_size(snd_pcm_t* pcm_handle,
                                uint32_t* samples,
                                int sample_size,
                                snd_pcm_hw_params_t* params) {
  int status;
  snd_pcm_hw_params_t* hwparams;
  snd_pcm_uframes_t frames;
  unsigned int periods;

  /* Copy the hardware parameters for this setup */
  snd_pcm_hw_params_alloca(&hwparams);
  snd_pcm_hw_params_copy(hwparams, params);

  frames = 1024;  //*samples;
  status = snd_pcm_hw_params_set_period_size_near(pcm_handle, hwparams, &frames,
                                                  NULL);
  if (status < 0) {
    av_log(NULL, AV_LOG_ERROR, "Couldn't set period size : %s\n",
           snd_strerror(status));
    return (-1);
  }

  periods = ((*samples) >> 10) * sample_size;
  status =
      snd_pcm_hw_params_set_periods_near(pcm_handle, hwparams, &periods, NULL);
  if (status < 0) {
    av_log(NULL, AV_LOG_ERROR, "Couldn't set periods : %s\n",
           snd_strerror(status));
    return (-1);
  }

  return ALSA_finalize_hardware(pcm_handle, samples, sample_size, hwparams);
}

static int ALSA_set_buffer_size(snd_pcm_t* pcm_handle,
                                uint32_t* samples,
                                int sample_size,
                                snd_pcm_hw_params_t* params) {
  int status;
  snd_pcm_hw_params_t* hwparams;
  snd_pcm_uframes_t frames;
  /* Copy the hardware parameters for this setup */
  snd_pcm_hw_params_alloca(&hwparams);
  snd_pcm_hw_params_copy(hwparams, params);

  frames = (*samples) * sample_size;
  status =
      snd_pcm_hw_params_set_buffer_size_near(pcm_handle, hwparams, &frames);
  if (status < 0) {
    av_log(NULL, AV_LOG_ERROR, "Couldn't set buffer size : %s\n",
           snd_strerror(status));
    return (-1);
  }

  return ALSA_finalize_hardware(pcm_handle, samples, sample_size, hwparams);
}

static void show_available_sample_formats(snd_pcm_t* handle,
                                          snd_pcm_hw_params_t* params) {
  snd_pcm_format_t format;

  fprintf(stderr, "Available formats:\n");
  for (format = 0; format <= SND_PCM_FORMAT_LAST; format++) {
    if (snd_pcm_hw_params_test_format(handle, params, format) == 0)
      fprintf(stderr, "- %s\n", snd_pcm_format_name(format));
  }
}

static AudioParams g_audio_hw_params = {0};
static pthread_mutex_t g_audio_hw_mutex;

int audio_dev_init() {
  // g_audio_hw_params.wanted_params.fmt = AV_SAMPLE_FMT_S16;
  // g_audio_hw_params.wanted_params.wanted_channel_layout =
  // AV_CH_LAYOUT_STEREO;
  // g_audio_hw_params.wanted_params.wanted_sample_rate = 44100;
  memset(&g_audio_hw_mutex, 0, sizeof(pthread_mutex_t));
  return pthread_mutex_init(&g_audio_hw_mutex, NULL);
}

void audio_dev_deinit() {
  audio_dev_close();
  pthread_mutex_destroy(&g_audio_hw_mutex);
}

#define AUDIO_DEV_LOCK pthread_mutex_lock(&g_audio_hw_mutex)
#define AUDIO_DEV_UNLOCK pthread_mutex_unlock(&g_audio_hw_mutex)

int audio_params_changed(enum AVSampleFormat fmt,
                         int64_t wanted_channel_layout,
                         int wanted_sample_rate) {
  WantedAudioParams* param = &g_audio_hw_params.wanted_params;
  return (param->fmt != fmt ||
          param->wanted_channel_layout != wanted_channel_layout ||
          param->wanted_sample_rate != wanted_sample_rate);
}

int audio_dev_open(enum AVSampleFormat fmt,
                   int64_t wanted_channel_layout,
                   int wanted_sample_rate,
                   AudioParams** paudio_hw_params) {
  int status;
  snd_pcm_t* pcm_handle = NULL;
  snd_pcm_hw_params_t* hwparams = NULL;
  snd_pcm_sw_params_t* swparams = NULL;
  snd_pcm_format_t pcm_fmt;
  int wanted_nb_channels;
  uint32_t rate = 0;
  int sample_size = 0;
  AudioParams* audio_hw_params = &g_audio_hw_params;
  AUDIO_DEV_LOCK;
  pcm_handle = g_audio_hw_params.pcm_handle;
  if (pcm_handle) {
    if (audio_params_changed(fmt, wanted_channel_layout, wanted_sample_rate)) {
      AUDIO_DEV_UNLOCK;
      audio_dev_close();
      AUDIO_DEV_LOCK;
    } else {
      *paudio_hw_params = &g_audio_hw_params;
      AUDIO_DEV_UNLOCK;
      return 0;
    }
  }
  printf("~~fmt: %d, wanted_channel_layout: %lld, wanted_sample_rate: %d\n",
         fmt, wanted_channel_layout, wanted_sample_rate);
  audio_hw_params->wanted_params.fmt = fmt;
  audio_hw_params->wanted_params.wanted_channel_layout = wanted_channel_layout;
  audio_hw_params->wanted_params.wanted_sample_rate = wanted_sample_rate;
  // putenv("ALSA_CONFIG_PATH=/usr/local/share/alsa/alsa.conf");
  switch (fmt) {
    case AV_SAMPLE_FMT_S16:
      pcm_fmt = SND_PCM_FORMAT_S16_LE;
      break;
    default:
      av_log(NULL, AV_LOG_ERROR, "TODO support other sample format, %d\n", fmt);
      goto err;
  }
  // TODO, add audio out switch function
  status = snd_pcm_open(&pcm_handle, "hw:0,0", SND_PCM_STREAM_PLAYBACK, 0);
  if (status < 0 || !pcm_handle) {
    av_log(NULL, AV_LOG_ERROR, ("audio open error: %s\n"),
           snd_strerror(status));
    goto err;
  }
  /* Figure out what the hardware is capable of */
  snd_pcm_hw_params_alloca(&hwparams);
  if (!hwparams) {
    fprintf(stderr, "hwparams alloca failed, no memory!\n");
    goto err;
  }
  status = snd_pcm_hw_params_any(pcm_handle, hwparams);
  if (status < 0) {
    av_log(NULL, AV_LOG_ERROR, "Couldn't get hardware config: %s\n",
           snd_strerror(status));
    goto err;
  }
#if 0  // def DEBUG
    {
        snd_output_t *log;
        snd_output_stdio_attach(&log, stderr, 0);
        // fprintf(stderr, "HW Params of device \"%s\":\n",
        //        snd_pcm_name(pcm_handle));
        fprintf(stderr, "--------------------\n");
        snd_pcm_hw_params_dump(hwparams, log);
        fprintf(stderr, "--------------------\n");
        snd_output_close(log);
    }
#endif
  status = snd_pcm_hw_params_set_access(pcm_handle, hwparams,
                                        SND_PCM_ACCESS_RW_INTERLEAVED);
  if (status < 0) {
    av_log(NULL, AV_LOG_ERROR, "Couldn't set interleaved access: %s\n",
           snd_strerror(status));
    goto err;
  }
  status = snd_pcm_hw_params_set_format(pcm_handle, hwparams, pcm_fmt);
  if (status < 0) {
    av_log(NULL, AV_LOG_ERROR, "Couldn't find any hardware audio formats\n");
    show_available_sample_formats(pcm_handle, hwparams);
    goto err;
  }
  audio_hw_params->pcm_format = pcm_fmt;
  wanted_nb_channels = av_get_channel_layout_nb_channels(wanted_channel_layout);
  status =
      snd_pcm_hw_params_set_channels(pcm_handle, hwparams, wanted_nb_channels);
  if (status < 0) {
    uint32_t channels;
    av_log(NULL, AV_LOG_WARNING, "Couldn't set audio channels1: %s\n",
           snd_strerror(status));
    status = snd_pcm_hw_params_get_channels(hwparams, &channels);
    if (status < 0) {
      av_log(NULL, AV_LOG_ERROR, "Couldn't set audio channels2: %s\n",
             snd_strerror(status));
      status = snd_pcm_hw_params_get_channels_min(hwparams, &channels);
      if (status < 0) {
        av_log(NULL, AV_LOG_ERROR, "Couldn't set audio channels3: %s\n",
               snd_strerror(status));
        goto err;
      }
    }
    wanted_nb_channels = channels;
    wanted_channel_layout = av_get_default_channel_layout(channels);
  }
  rate = wanted_sample_rate;
  status = snd_pcm_hw_params_set_rate_near(pcm_handle, hwparams, &rate, NULL);
  if (status < 0) {
    av_log(NULL, AV_LOG_ERROR, "Couldn't set audio frequency: %s\n",
           snd_strerror(status));
    goto err;
  }
  wanted_sample_rate = rate;
  audio_hw_params->pcm_samples =
      FFMAX(AUDIO_MIN_BUFFER_SIZE,
            2 << av_log2(wanted_sample_rate / AUDIO_MAX_CALLBACKS_PER_SEC));
#if 1
  sample_size =
      /* wanted_nb_channels* */ snd_pcm_format_physical_width(pcm_fmt) >> 3;
/* Set the buffer size, in samples */
#if 0
    if (ALSA_set_buffer_size(pcm_handle, &audio_hw_params->pcm_samples, sample_size, hwparams) < 0) {
        if (ALSA_set_period_size(pcm_handle, &audio_hw_params->pcm_samples, sample_size, hwparams) < 0) {
            goto err;
        }
    }
#else
  if (ALSA_set_period_size(pcm_handle, &audio_hw_params->pcm_samples,
                           sample_size, hwparams) < 0) {
    if (ALSA_set_buffer_size(pcm_handle, &audio_hw_params->pcm_samples,
                             sample_size, hwparams) < 0) {
      goto err;
    }
  }
#endif
#else
  static unsigned int buffer_time = 500000; /* ring buffer length in us */
  static unsigned int period_time = 100000; /* period time in us */
  int dir;
  snd_pcm_sframes_t buffer_size;
  snd_pcm_sframes_t period_size;
  /* set the buffer time */
  status = snd_pcm_hw_params_set_buffer_time_near(pcm_handle, hwparams,
                                                  &buffer_time, &dir);
  if (status < 0) {
    av_log(NULL, AV_LOG_ERROR,
           "Unable to set buffer time %i for playback: %s\n", buffer_time,
           snd_strerror(status));
    goto err;
  }
  status = snd_pcm_hw_params_get_buffer_size(hwparams, &buffer_size);
  if (status < 0) {
    av_log(NULL, AV_LOG_ERROR, "Unable to get buffer size for playback: %s\n",
           snd_strerror(status));
    goto err;
  }
  status = snd_pcm_hw_params_set_period_time_near(pcm_handle, hwparams,
                                                  &period_time, &dir);
  if (status < 0) {
    av_log(NULL, AV_LOG_ERROR,
           "Unable to set period time %i for playback: %s\n", period_time,
           snd_strerror(status));
    goto err;
  }
  status = snd_pcm_hw_params_get_period_size(hwparams, &period_size, &dir);
  if (status < 0) {
    av_log(NULL, AV_LOG_ERROR, "Unable to get period size for playback: %s\n",
           snd_strerror(status));
    goto err;
  }
  av_assert0(buffer_size != period_size);
  /* write the parameters to device */
  status = snd_pcm_hw_params(pcm_handle, hwparams);
  if (status < 0) {
    av_log(NULL, AV_LOG_ERROR, "Unable to set hw params for playback: %s\n",
           snd_strerror(status));
    goto err;
  }
  if (1) {
    snd_pcm_uframes_t persize = 0;
    unsigned int periods = 0;
    snd_pcm_uframes_t bufsize = 0;
    snd_pcm_hw_params_get_period_size(hwparams, &persize, NULL);
    snd_pcm_hw_params_get_periods(hwparams, &periods, NULL);
    snd_pcm_hw_params_get_buffer_size(hwparams, &bufsize);
    fprintf(stderr,
            "ALSA: period size = %ld, periods = %u, buffer size = %lu\n",
            persize, periods, bufsize);
  }
#endif
  snd_pcm_sw_params_alloca(&swparams);
  if (!swparams) {
    fprintf(stderr, "swparams alloca failed, no memory!\n");
    goto err;
  }
  status = snd_pcm_sw_params_current(pcm_handle, swparams);
  if (status < 0) {
    av_log(NULL, AV_LOG_ERROR, "Couldn't get software config: %s\n",
           snd_strerror(status));
    goto err;
  }

  status = snd_pcm_sw_params_set_avail_min(pcm_handle, swparams,
                                           /*period_size*/
                                           audio_hw_params->pcm_samples);
  if (status < 0) {
    av_log(NULL, AV_LOG_ERROR, "Couldn't set minimum available samples: %s\n",
           snd_strerror(status));
    goto err;
  }
  /* start the transfer when the buffer is almost full: */
  /* (buffer_size / avail_min) * avail_min */
  status = snd_pcm_sw_params_set_start_threshold(
      pcm_handle, swparams, 1 /*(buffer_size / period_size) * period_size*/);
  if (status < 0) {
    av_log(NULL, AV_LOG_ERROR,
           "Unable to set start threshold mode for playback: %s\n",
           snd_strerror(status));
    goto err;
  }
  status = snd_pcm_sw_params(pcm_handle, swparams);
  if (status < 0) {
    av_log(NULL, AV_LOG_ERROR, "Couldn't set software audio parameters: %s\n",
           snd_strerror(status));
    goto err;
  }
  /* Switch to blocking mode for playback */
  // snd_pcm_nonblock(pcm_handle, 0);

  audio_hw_params->fmt = fmt;
  audio_hw_params->freq = wanted_sample_rate;
  audio_hw_params->channel_layout = wanted_channel_layout;
  audio_hw_params->channels = wanted_nb_channels;
  audio_hw_params->frame_size = av_samples_get_buffer_size(
      NULL, audio_hw_params->channels, 1, audio_hw_params->fmt, 1);
  audio_hw_params->bytes_per_sec = av_samples_get_buffer_size(
      NULL, audio_hw_params->channels, audio_hw_params->freq,
      audio_hw_params->fmt, 1);
  if (audio_hw_params->bytes_per_sec <= 0 || audio_hw_params->frame_size <= 0) {
    av_log(NULL, AV_LOG_ERROR, "av_samples_get_buffer_size failed\n");
    goto err;
  }
#ifdef DEBUG
  printf(
      "fmt: %d, freq: %d, channel_layout: %lld, channels: %d, "
      "bytes_per_sec: %d, frame_size: %d,"
      "pcm_samples: %d, snd_pcm_format_physical_width(pcm_fmt): %d\n",
      audio_hw_params->fmt, audio_hw_params->freq,
      audio_hw_params->channel_layout, audio_hw_params->channels,
      audio_hw_params->bytes_per_sec, audio_hw_params->frame_size,
      audio_hw_params->pcm_samples, snd_pcm_format_physical_width(pcm_fmt));
#endif
  audio_hw_params->pcm_handle = pcm_handle;
  audio_hw_params->audio_hw_buf_size =
      (wanted_nb_channels * audio_hw_params->pcm_samples *
       snd_pcm_format_physical_width(pcm_fmt) / 8);
  *paudio_hw_params = &g_audio_hw_params;
  AUDIO_DEV_UNLOCK;
  return 0;
err:
  if (pcm_handle) {
    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);
  }
  AUDIO_DEV_UNLOCK;
  return -1;
}

void audio_dev_close() {
  snd_pcm_t* pcm_handle = NULL;
  AUDIO_DEV_LOCK;
  pcm_handle = g_audio_hw_params.pcm_handle;
  if (pcm_handle) {
    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);
    memset(&g_audio_hw_params, 0, sizeof(AudioParams));
    // printf("audio_dev_close ----------------\n");
  }
  AUDIO_DEV_UNLOCK;
}

int audio_dev_write(uint8_t* buf, snd_pcm_sframes_t* frames_left) {
  snd_pcm_t* pcm_handle = NULL;
  int status = -1;
  AUDIO_DEV_LOCK;
  pcm_handle = g_audio_hw_params.pcm_handle;
  if (pcm_handle) {
    size_t frames = *frames_left;
    assert(frames > 0);
    // printf("snd_pcm_writei, buf: %p, size: %d\n", buf, frames);
    status = snd_pcm_writei(pcm_handle, buf, frames);
    // status = frames;
    // av_usleep(1000000.0*is->audio_hw_buf_size/audio_tgt_params.bytes_per_sec);
    // printf("snd_pcm_writei: %d, frame_size: %d\n",status,
    // g_audio_hw_params.frame_size);
    if (status < 0) {
      if (status == -EAGAIN) {
        /* Apparently snd_pcm_recover() doesn't handle this case - does it
         * assume snd_pcm_wait() above? */
        AUDIO_DEV_UNLOCK;
        av_usleep(100);
        return 0;
      }
      status = snd_pcm_recover(pcm_handle, status, 0);
      if (status < 0) {
        /* Hmm, not much we can do - abort */
        fprintf(stderr, "ALSA write failed (unrecoverable): %s\n",
                snd_strerror(status));
        goto err;
      }
      goto err;
    }
    *frames_left -= status;
    status *= g_audio_hw_params.frame_size;
  }
err:
  AUDIO_DEV_UNLOCK;
  // printf("ssss status : %d\n", status);
  return status;
}
