#include "base_encoder.h"

BaseEncoder::BaseEncoder()
{
    avcodec = NULL;
    avstream = NULL;
    codec_name[0] = 0;
    codec_id = AV_CODEC_ID_NONE;
}

BaseEncoder::~BaseEncoder()
{
    if (avstream) {
        avcodec_close(avstream->codec);
    }
    if (avfmtctx) {
        /* free the stream */
        avformat_free_context(avfmtctx);
    }
}

char *BaseEncoder::get_codec_name()
{
    return codec_id != AV_CODEC_ID_NONE ? (char *)avcodec_get_name(codec_id)
           : codec_name;
}

int BaseEncoder::init()
{
    avcodec = codec_id != AV_CODEC_ID_NONE
              ? avcodec_find_encoder(codec_id)
              : avcodec_find_encoder_by_name(codec_name);
    if (!avcodec) {
        fprintf(stderr, "Could not find encoder for '%s'\n", get_codec_name());
        return -1;
    }
    avfmtctx = avformat_alloc_context();
    if (!avfmtctx) {
        fprintf(stderr, "alloc avfmtctx failed for '%s'\n", get_codec_name());
        return -1;
    }
    avstream = avformat_new_stream(avfmtctx, avcodec);
    if (!avstream) {
        fprintf(stderr, "alloc stream failed for '%s'\n", get_codec_name());
        return -1;
    }
    return 0;
}

int BaseEncoder::encoder_init_config(video_encode_config *config)
{
    encode_config = *config;
    return 0;
}

AVFormatContext *BaseEncoder::get_avfmtctx()
{
    return avfmtctx;
}

AVCodec *BaseEncoder::get_avcodec()
{
    return avcodec;
}

AVStream *BaseEncoder::get_avstream()
{
    return avstream;
}

class av_register
{
public:
    av_register()
    {
        av_register_all();
    }
};

static av_register av_reg;
