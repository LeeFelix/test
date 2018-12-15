extern "C" {
//#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
//#include <libavutil/mathematics.h>
#include <libswresample/swresample.h>
}
#include "aac_encoder.h"

AACEncoder::AACEncoder()
{
    codec_id = AV_CODEC_ID_AAC;
    audio_frame = NULL;
    audio_tmp_frame = NULL;
    audio_index = 0;
    samples_count = 0;
    swr_ctx = NULL;
}

AACEncoder::~AACEncoder()
{
    av_frame_free(&audio_frame);
    av_frame_free(&audio_tmp_frame);
    swr_free(&swr_ctx);
}

static AVFrame *alloc_audio_frame(enum AVSampleFormat sample_fmt,
                                  uint64_t channel_layout, int sample_rate,
                                  int nb_samples)
{
    AVFrame *frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Error allocating an audio frame\n");
        return NULL;
    }
    frame->format = sample_fmt;
    frame->channel_layout = channel_layout;
    frame->sample_rate = sample_rate;
    frame->nb_samples = nb_samples;
    if (nb_samples) {
        int ret = av_frame_get_buffer(frame, 0);
        if (ret < 0) {
            fprintf(stderr, "Error allocating an audio buffer\n");
            av_frame_free(&frame);
            return NULL;
        }
    }
    return frame;
}

int AACEncoder::encode_init_config(video_encode_config *config)
{
    int nb_samples, out_nb_samples;
    enum AVSampleFormat in_sample_fmt;
    AVStream *stream = get_avstream();
    AVCodec *codec = get_avcodec();
    AVCodecContext *c = stream->codec;

    c->sample_fmt =
        codec->sample_fmts ? codec->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
    c->bit_rate = config->audio_bit_rate ? config->audio_bit_rate : 64000;
    encode_config.audio_bit_rate = c->bit_rate;
    c->sample_rate =
        config->audio_sample_rate ? config->audio_sample_rate : 44100;
    encode_config.audio_sample_rate = c->sample_rate;
    if (codec->supported_samplerates) {
        uint8_t match = 0;
        for (int i = 0; codec->supported_samplerates[i]; i++) {
            if (codec->supported_samplerates[i] == c->sample_rate)
                match = 1;
        }
        if (!match) {
            // it's better to set the same sample_rate
            fprintf(stderr, "uncorrect config for audio sample_rate: %d\n",
                    c->sample_rate);
            return -1;
        }
    }
    // c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
    c->channel_layout = config->channel_layout;

    if (codec->channel_layouts) {
        uint8_t match = 0;
        for (int i = 0; codec->channel_layouts[i]; i++) {
            if (codec->channel_layouts[i] == c->channel_layout)
                match = 1;
        }
        if (!match) {
            // it's better to set the same channel_layout
            fprintf(stderr, "uncorrect config for audio channel layout\n");
            return -1;
        }
    }
    encode_config.channel_layout = config->channel_layout;
    c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
    encode_config.nb_channels = c->channels;
    stream->time_base = (AVRational) {
        1, c->sample_rate
    };
    c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    int av_ret = avcodec_open2(c, codec, NULL);
    if (av_ret < 0) {
        char str[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_strerror(av_ret, str, AV_ERROR_MAX_STRING_SIZE);
        fprintf(stderr, "Could not open audio codec: %s\n", str);
        return -1;
    }

    nb_samples = config->input_nb_samples ? config->input_nb_samples : 10000;
    if (c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
        out_nb_samples = nb_samples;
    else {
        out_nb_samples = c->frame_size;
    }
    encode_config.input_nb_samples = nb_samples;

    in_sample_fmt =
        config->input_sample_fmt ? config->input_sample_fmt : AV_SAMPLE_FMT_S16;
    encode_config.input_sample_fmt = in_sample_fmt;
    audio_tmp_frame = alloc_audio_frame(in_sample_fmt, c->channel_layout,
                                        c->sample_rate, nb_samples);
    if (!audio_tmp_frame) {
        return -1;
    }
    if ((c->sample_fmt != in_sample_fmt) || (out_nb_samples != nb_samples)) {
        PRINTF("audio sample_fmt: %d, nb_samples: %d, channels: %d\n",
               c->sample_fmt, nb_samples, c->channels);
        audio_frame = alloc_audio_frame(c->sample_fmt, c->channel_layout,
                                        c->sample_rate, out_nb_samples);
        if (!audio_frame) {
            return -1;
        }
        /* create resampler context */
        swr_ctx = swr_alloc();
        if (!swr_ctx) {
            fprintf(stderr, "Could not allocate resampler context\n");
            return -1;
        }
        /* set options */
        av_opt_set_int(swr_ctx, "in_channel_count", c->channels, 0);
        av_opt_set_int(swr_ctx, "in_sample_rate", c->sample_rate, 0);
        av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", in_sample_fmt, 0);
        av_opt_set_int(swr_ctx, "out_channel_count", c->channels, 0);
        av_opt_set_int(swr_ctx, "out_sample_rate", c->sample_rate, 0);
        av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", c->sample_fmt, 0);

        /* initialize the resampling context */
        if ((av_ret = swr_init(swr_ctx)) < 0) {
            fprintf(stderr, "Failed to initialize the resampling context\n");
            return -1;
        }
    }
    return 0;
}

// just return the audio_tmp_frame
void AACEncoder::encode_get_audio_buffer(void **buf, int *nb_samples,
        int *channels)
{
    AVFrame *frame = audio_tmp_frame;
    *buf = frame->data[0];
    *nb_samples = frame->nb_samples;
    *channels = frame->channels;
}

int AACEncoder::encode_audio_frame(AVPacket *out_pkt,
                                   uint64_t &audio_pkt_index)
{
    // data in audio_tmp_frame
    AVCodecContext *c = get_avstream()->codec;
    AVPacket pkt = {0};
    AVFrame *frame = audio_tmp_frame;
    int ret;
    int got_packet;
    int dst_nb_samples;
    av_init_packet(&pkt);

    audio_index++;
    audio_pkt_index = audio_index;
    if (swr_ctx) {
        /* convert samples from native format to destination codec format, using the
         * resampler compute destination number of samples
         */
        dst_nb_samples = av_rescale_rnd(
                             swr_get_delay(swr_ctx, c->sample_rate) + frame->nb_samples,
                             c->sample_rate, c->sample_rate, AV_ROUND_UP);
        assert(dst_nb_samples == audio_frame->nb_samples);

        /* when we pass a frame to the encoder, it may keep a reference to it
         * internally;
         * make sure we do not overwrite it here
         */
        ret = av_frame_make_writable(audio_frame);
        if (ret < 0) {
            fprintf(stderr, "Error av_frame_make_writable\n");
            return -1;
        }
        /* convert to destination format */
        ret = swr_convert(swr_ctx, audio_frame->data, dst_nb_samples,
                          (const uint8_t **)frame->data, frame->nb_samples);
        if (ret < 0) {
            fprintf(stderr, "Error while converting\n");
            return -1;
        }
        frame = audio_frame;

        frame->pts = av_rescale_q(samples_count, (AVRational) {
            1, c->sample_rate
        },
        c->time_base);
        samples_count += dst_nb_samples;
    }

    ret = avcodec_encode_audio2(c, &pkt, frame, &got_packet);
    if (ret < 0) {
        char str[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_strerror(ret, str, AV_ERROR_MAX_STRING_SIZE);
        fprintf(stderr, "Error encoding audio frame: %s\n", str);
        goto a_out;
    }

    if (got_packet) {
        if (out_pkt) {
            *out_pkt = pkt;
            return 0;
        } else {
            assert(0);
        }
    }
a_out:
    av_packet_unref(&pkt);
    return 0;
}
// return if there is no remaining audio data
void AACEncoder::encode_flush_audio_data(AVPacket *out_pkt, uint64_t &audio_idx,
        bool &finish)
{
    int got_packet = 0;
    AVPacket pkt = {0}; // data and size must be 0;
    av_init_packet(&pkt);
    int av_ret =
        avcodec_encode_audio2(get_avstream()->codec, &pkt, NULL, &got_packet);
    if (av_ret < 0) {
        char str[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_strerror(av_ret, str, AV_ERROR_MAX_STRING_SIZE);
        fprintf(stderr, "Error encoding audio frame: %s\n", str);
        got_packet = 0;
    }

    if (got_packet) {
        assert(pkt.pts == pkt.dts);
        finish = false;
        *out_pkt = pkt;
        audio_idx = ++audio_index;
        return;
    }
    av_packet_unref(&pkt);
    if (!got_packet) {
        finish = true;
    }
}