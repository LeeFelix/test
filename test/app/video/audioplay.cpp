#include "audioplay.h"
#include "audio_dev.h"
#include "av_wrapper/decoder_demuxing/decoder_demuxing.h"
#include "av_wrapper/video_common.h"
extern "C" {
#include <libavutil/time.h>
#include <libswresample/swresample.h>
}
#include <vector>

class AudioPlayer {
 private:
  DecoderDemuxer dd;
  bool cache_all;  // cache all frame in memory
  AudioParams* audio_tgt_params;
  AudioParams audio_src_params;
  struct SwrContext* swr_ctx;
  std::vector<AVFrame*>* frame_vector;
  int vector_index;  // current reading frame index in vector
  std::mutex vector_mutex;
  std::condition_variable vector_cond;
  pthread_t play_tid;
  bool stop_play;
  bool fatal_error_occur;

 public:
  AudioPlayer();
  ~AudioPlayer();
  int init(char* audio_file_path);
  int set_cache_flag(int cache);
  bool get_cache_flag();
  AudioParams* get_audio_tgt_params();
  AVFrame* get_next_frame();
  int convert_frame(AVFrame* in_frame, AVFrame** out_frame);
  void set_fatal_error();
  int play();
};

static void* audio_refresh_thread(void* arg) {
  int status = 0;
  snd_pcm_sframes_t frames_left;
  AudioPlayer* player = (AudioPlayer*)arg;
  while (1) {
    // decode, and playback in the same thread, seperate if need TODO
    AVFrame* frame = player->get_next_frame();
    if (!frame)
      break;
    uint8_t* stream = frame->extended_data[0];
    int len = frame->linesize[0];
    frames_left = len;
    while (frames_left > 0) {
      status = audio_dev_write(stream, &frames_left);
      if (status < 0) {
        player->set_fatal_error();
        break;
      } else if (status == 0) {
        continue;
      }
      stream += status;
    }
    if (!player->get_cache_flag())
      av_frame_free(&frame);
  }

  if (!player->get_cache_flag()) {
    pthread_detach(pthread_self());
    // if non-cache, delete when completing playing
    delete player;
  }
  // printf("out of audio_refresh_thread\n");
  return NULL;
}

static void clear_frame_vector(std::vector<AVFrame*>* frame_vector) {
  if (!frame_vector)
    return;
  while (!frame_vector->empty()) {
    AVFrame* frame = frame_vector->back();
    av_frame_free(&frame);
    frame_vector->pop_back();
  }
}

static void free_frame_vector(std::vector<AVFrame*>** vector,
                              std::mutex* mutex) {
  std::unique_lock<std::mutex> lk(*mutex);
  if (vector && *vector) {
    std::vector<AVFrame*>* frame_vector = *vector;
    clear_frame_vector(frame_vector);
    delete frame_vector;
    *vector = NULL;
  }
}

AudioPlayer::AudioPlayer() {
  cache_all = false;
  audio_tgt_params = NULL;
  memset(&audio_src_params, 0, sizeof(audio_src_params));
  swr_ctx = NULL;
  frame_vector = NULL;
  vector_index = -1;
  play_tid = 0;
  stop_play = true;
  fatal_error_occur = false;
}

AudioPlayer::~AudioPlayer() {
  stop_play = true;
  free_frame_vector(&frame_vector, &vector_mutex);
  if (play_tid)
    pthread_join(play_tid, NULL);
  swr_free(&swr_ctx);
}

int AudioPlayer::init(char* audio_file_path) {
  int ret = dd.init(audio_file_path, VIDEO_DISABLE_FLAG);
  if (0 == ret) {
    media_info& info = dd.get_media_info();
    ret = audio_dev_open(info.out_sample_fmt, info.channel_layout,
                         info.audio_sample_rate, &audio_tgt_params);
    if (0 == ret) {
      audio_src_params = *audio_tgt_params;
      audio_src_params.pcm_handle = NULL;
    }
  }
  return ret;
}

int AudioPlayer::set_cache_flag(int cache) {
  cache_all = cache > 0 ? true : false;
  free_frame_vector(&frame_vector, &vector_mutex);
  if (cache_all) {
    frame_vector = new std::vector<AVFrame*>();
    if (!frame_vector) {
      fprintf(stderr, "set_cache failed, no memory!\n");
      return AVERROR(ENOMEM);
    }
  }
  return 0;
}

bool AudioPlayer::get_cache_flag() {
  return cache_all;
}

AudioParams* AudioPlayer::get_audio_tgt_params() {
  return audio_tgt_params;
}

int AudioPlayer::convert_frame(AVFrame* in_frame, AVFrame** out_frame) {
  int data_size, resampled_data_size;
  int64_t dec_channel_layout;
  int wanted_nb_samples;
  AVFrame* frame = NULL;
  data_size = av_samples_get_buffer_size(
      NULL, av_frame_get_channels(in_frame), in_frame->nb_samples,
      (enum AVSampleFormat)in_frame->format, 1);
  dec_channel_layout =
      (in_frame->channel_layout &&
       av_frame_get_channels(in_frame) ==
           av_get_channel_layout_nb_channels(in_frame->channel_layout))
          ? in_frame->channel_layout
          : av_get_default_channel_layout(av_frame_get_channels(in_frame));

  wanted_nb_samples = audio_tgt_params->pcm_samples;  // in_frame->nb_samples;
  if (in_frame->format != audio_src_params.fmt ||
      dec_channel_layout != audio_src_params.channel_layout ||
      in_frame->sample_rate != audio_src_params.freq ||
      (wanted_nb_samples != in_frame->nb_samples && !swr_ctx)) {
    swr_free(&swr_ctx);
    swr_ctx = swr_alloc_set_opts(NULL, audio_tgt_params->channel_layout,
                                 (enum AVSampleFormat)audio_tgt_params->fmt,
                                 audio_tgt_params->freq, dec_channel_layout,
                                 (enum AVSampleFormat)in_frame->format,
                                 in_frame->sample_rate, 0, NULL);
    if (!swr_ctx || swr_init(swr_ctx) < 0) {
      av_log(NULL, AV_LOG_ERROR,
             "Cannot create sample rate converter for "
             "conversion of %d Hz %s %d channels to %d Hz "
             "%s %d channels!\n",
             in_frame->sample_rate,
             av_get_sample_fmt_name((enum AVSampleFormat)in_frame->format),
             av_frame_get_channels(in_frame), audio_tgt_params->freq,
             av_get_sample_fmt_name((enum AVSampleFormat)audio_tgt_params->fmt),
             audio_tgt_params->channels);
      swr_free(&swr_ctx);
      return -1;
    }

    audio_src_params.channel_layout = dec_channel_layout;
    audio_src_params.channels = av_frame_get_channels(in_frame);
    audio_src_params.freq = in_frame->sample_rate;
    audio_src_params.fmt = (enum AVSampleFormat)in_frame->format;
  }
  if (swr_ctx) {
    // PRINTF("do audio swr convert\n");
    frame = av_frame_alloc();
    if (!frame) {
      av_log(NULL, AV_LOG_ERROR, "frame alloc failed, no memory!\n");
      return -1;
    }
    int out_count = (int64_t)wanted_nb_samples * audio_tgt_params->freq /
                        in_frame->sample_rate +
                    256;
    // PRINTF("out_count : %d, wanted_nb_samples: %d\n", out_count,
    // wanted_nb_samples);
    frame->channels = audio_tgt_params->channels;
    frame->nb_samples = wanted_nb_samples;  // out_count;
    frame->format = audio_tgt_params->fmt;

    const uint8_t** in = (const uint8_t**)in_frame->extended_data;
    uint8_t** out = frame->extended_data;
    assert(out);
    if (av_frame_get_buffer(frame, 0) < 0) {
      av_log(NULL, AV_LOG_ERROR, "av_frame_get_buffer() failed\n");
      goto err;
    }
    int len2 = 0;
    if (wanted_nb_samples != in_frame->nb_samples) {
      if (swr_set_compensation(
              swr_ctx, (wanted_nb_samples - in_frame->nb_samples) *
                           audio_tgt_params->freq / in_frame->sample_rate,
              wanted_nb_samples * audio_tgt_params->freq /
                  in_frame->sample_rate) < 0) {
        av_log(NULL, AV_LOG_ERROR, "swr_set_compensation() failed\n");
        goto err;
      }
    }

    frame->nb_samples = wanted_nb_samples;
    len2 = swr_convert(swr_ctx, out, out_count, in, in_frame->nb_samples);
    // printf("in_frame->nb_samples: %d, len2: %d\n", in_frame->nb_samples,
    // len2);
    if (len2 < 0) {
      av_log(NULL, AV_LOG_ERROR, "swr_convert() failed\n");
      goto err;
    }
    if (len2 == out_count) {
      av_log(NULL, AV_LOG_WARNING, "audio buffer is probably too small\n");
      if (swr_init(swr_ctx) < 0)
        swr_free(&swr_ctx);
    }
    // is->audio_buf = is->audio_buf1;
    av_frame_free(&in_frame);
    *out_frame = frame;
    resampled_data_size = len2 * audio_tgt_params->channels *
                          av_get_bytes_per_sample(audio_tgt_params->fmt);

    // PRINTF("frame linesize[0]: %d, len2: %d\n", frame->linesize[0], len2);
    (*out_frame)->linesize[0] = len2;
  } else {
    // PRINTF("do not audio swr convert\n");
    *out_frame = in_frame;
    resampled_data_size = data_size;
    (*out_frame)->linesize[0] = in_frame->nb_samples;
  }

  //    printf("22 out_frame linesize[0]: %d, linesize[1]: %d,
  //    resampled_data_size: "
  //           "%d\n", (*out_frame)->linesize[0], (*out_frame)->linesize[1],
  //           resampled_data_size);
  return resampled_data_size;
err:
  av_frame_free(&frame);
  return -1;
}

AVFrame* AudioPlayer::get_next_frame() {
  std::unique_lock<std::mutex> lk(vector_mutex);
  if (stop_play)
    return NULL;
  if (cache_all) {
    media_info& info = dd.get_media_info();
    if (audio_params_changed(info.out_sample_fmt, info.channel_layout,
                             info.audio_sample_rate)) {
      if (audio_dev_open(info.out_sample_fmt, info.channel_layout,
                         info.audio_sample_rate, &audio_tgt_params)) {
        return NULL;
      }
      audio_src_params = *audio_tgt_params;
      clear_frame_vector(frame_vector);
      vector_index = -1;
      if (0 > dd.rewind()) {
        fprintf(stderr, "audio reading rewind failed!\n");
        return NULL;
      }
    }

    if (dd.is_finished()) {
      if (!frame_vector || frame_vector->empty()) {
        fprintf(stderr,
                "!!file is end, however frame array is empty. Is "
                "it audio file?\n");
        return NULL;
      } else {
        int idx = vector_index;
        if (idx + 1 >= frame_vector->size()) {
          vector_cond.wait(lk);
          idx = 0;
        } else {
          idx++;
        }
        vector_index = idx;
        return frame_vector->at(idx);
      }
    }
  } else {
    if (dd.is_finished()) {
      stop_play = true;
      return NULL;
    }
  }
  AVFrame* frame = av_frame_alloc();
  if (!frame) {
    fprintf(stderr, "frame_alloc failed, no memory!\n");
  } else {
    int got_frame = 0;
    int is_video = 0;
    while (!got_frame) {
      int ret =
          dd.read_and_decode_one_frame(NULL, frame, &got_frame, &is_video);
      // printf("1111ret: %d, got_frame: %d, is_video: %d\n", ret,
      // got_frame, is_video);
      if (dd.is_finished()) {
        if (cache_all) {
          vector_cond.wait(lk);
          frame_vector->push_back(frame);  // zero length
          return frame;
        }
        break;
      }
    }
    if (got_frame) {
      AVFrame* out_frame = NULL;
      assert(frame->data == frame->extended_data);
      if (0 > convert_frame(frame, &out_frame)) {
        return NULL;
      }
      frame = out_frame;
      if (cache_all) {
        frame_vector->push_back(frame);
      }
    } else {
      av_frame_free(&frame);
    }
  }
  return frame;
}

void AudioPlayer::set_fatal_error() {
  std::unique_lock<std::mutex> _lk(vector_mutex);
  stop_play = true;
  fatal_error_occur = true;
  pthread_detach(play_tid);  // let it selfdestory
  play_tid = 0;
  printf("Warning: AudioPlayer::set_fatal_error\n");
}

int AudioPlayer::play() {
  int ret = 0;
  std::unique_lock<std::mutex> _lk(vector_mutex);
  if (fatal_error_occur) {
    fprintf(stderr,
            "audio fatal error occured, should recreate AudioObject?\n");
    return -1;
  }
  stop_play = false;
  if (cache_all) {
    vector_index = -1;
    vector_cond.notify_one();
  } else {
    if (0 > dd.rewind()) {
      fprintf(stderr, "audio reading rewind failed!\n");
      return -1;
    }
  }
  if (play_tid == 0) {
    pthread_attr_t attr;
    if (pthread_attr_init(&attr)) {
      fprintf(stderr, "pthread_attr_init failed!\n");
      return -1;
    }
    if (pthread_attr_setstacksize(&attr, 48 * 1024)) {
      pthread_attr_destroy(&attr);
      return -1;
    }
    if (pthread_create(&play_tid, &attr, audio_refresh_thread, this)) {
      fprintf(stderr, "audio_refresh_thread create failed!\n");
      ret = -1;
    }
    if (!cache_all)
      play_tid = 0;
    pthread_attr_destroy(&attr);
  }
  return ret;
}

int audio_play_init(void** handle, char* audio_file_path, int cache_flag) {
  AudioPlayer* player = new AudioPlayer();
  if (!player) {
    fprintf(stderr, "no memory\n");
    return -1;
  }
  if (0 > player->init(audio_file_path) ||
      0 > player->set_cache_flag(cache_flag)) {
    delete player;
    return -1;
  }
  *handle = player;
  return 0;
}

void audio_play_deinit(void* handle) {
  if (handle) {
    AudioPlayer* player = (AudioPlayer*)handle;
    delete player;
  }
}

int audio_play(void* handle) {
  AudioPlayer* player = (AudioPlayer*)handle;
  return player ? player->play() : -1;
}

int audio_play0(char* audio_file_path) {
  void* handle = NULL;
  int ret = audio_play_init(&handle, audio_file_path, 0);
  if (handle) {
    ret = audio_play(handle);
    // for non cache frame playing, handle will auto deinit when completing
    // playing
    if (ret < 0) {
      delete (AudioPlayer*)handle;
    }
  }
  return ret;
}
