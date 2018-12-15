//by rk wangh
#ifndef CAMERA_ENCODER_H
#define CAMERA_ENCODER_H

#include "../../video_common.h"
extern "C" {
#include <libavformat/avformat.h>
}


//not thread_safe
class BaseEncoder : public video_object
{
private:
    // as avformat_new_stream must need un-null fmtctx, we need alloc a fmtctx
    AVFormatContext *avfmtctx;

    AVCodec *avcodec;
    AVStream *avstream;
    char *get_codec_name();

protected:
    char codec_name[16];
    AVCodecID codec_id;
    BaseEncoder();
    virtual int init();

public:
    video_encode_config encode_config;
    virtual ~BaseEncoder();
    virtual int encoder_init_config(video_encode_config *config);
    AVFormatContext *get_avfmtctx();
    AVCodec *get_avcodec();
    AVStream *get_avstream();
    CREATE_FUNC(BaseEncoder)
};

#endif // CAMERA_ENCODER_H
