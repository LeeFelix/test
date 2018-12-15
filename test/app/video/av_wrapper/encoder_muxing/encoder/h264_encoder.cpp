#include "h264_encoder.h"

extern "C" {
#include "../../../watermark.h"
#include "../../../common.h"
}

H264Encoder::H264Encoder()
{
    // definitely specify rk's encoder
    snprintf(codec_name, sizeof(codec_name), "h264_mpp");
    video_frame = NULL;
    force_idr = false;
}

H264Encoder::~H264Encoder()
{
    av_frame_free(&video_frame);
}

int H264Encoder::encode_init_config(video_encode_config* config)
{
    assert(config);
    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not alloc video frame\n");
        return -1;
    }
    // we donot alloc the data buffer in av framework, as we will use the buffer
    // passed from outside
    video_frame = frame;
    frame->format = config->pixel_fmt;
    frame->width = config->width;
    frame->height = config->height;

    AVStream* stream = get_avstream();
    AVCodec* codec = get_avcodec();
    AVCodecContext* c = stream->codec;
    // AVFormatContext *oc = get_avfmtctx();
    int av_ret = 0;
    int s_f_r = config->stream_frame_rate ? config->stream_frame_rate : STREAM_FRAME_RATE;
    encode_config.stream_frame_rate = s_f_r;
    c->codec_id = codec->id; // AV_CODEC_ID_H264
    c->bit_rate = config->video_bit_rate ? config->video_bit_rate : VIDEO_BIT_RATE;
    encode_config.video_bit_rate = c->bit_rate;
    /* Resolution must be a multiple of two. */
    c->width = encode_config.width = config->width;
    c->height = encode_config.height = config->height;
    stream->time_base = (AVRational) {
        1, s_f_r
    };
    /* timebase: This is the fundamental unit of time (in seconds) in terms
     * of which frame timestamps are represented. For fixed-fps content,
     * timebase should be 1/framerate and timestamp increments should be
     * identical to 1. */
    c->time_base = stream->time_base;
    c->level = config->video_encoder_level ? config->video_encoder_level
               : VIDEO_ENCODER_LEVEL;
    encode_config.video_encoder_level = c->level;
    c->gop_size = config->video_gop_size
                  ? config->video_gop_size
                  : VIDEO_GOP_SIZE; /* emit one intra frame every GOP_SIZE
                                           frames at most */
    encode_config.video_gop_size = c->gop_size;
    c->profile = encode_config.video_profile = config->video_profile;
    c->pix_fmt = config->pixel_fmt;
    /* Some formats want stream headers to be separate. */
    c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    /* open the video codec */
    av_ret = avcodec_open2(c, codec, NULL);
    if (av_ret < 0) {
        char str[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_strerror(av_ret, str, AV_ERROR_MAX_STRING_SIZE);
        fprintf(stderr, "Could not open video codec: %s\n", str);
        return -1;
    }
    video_frame->format = c->pix_fmt;

#ifdef USE_WATERMARK
    MppEncOSDPlt palette;
    memcpy((Uint8*)palette.buf, (Uint8*)YUV444_PALETTE_TABLE, PALETTE_TABLE_LEN*4);

    int ret = encode_control(MPP_ENC_SET_OSD_PLT_CFG, &palette);
    if(ret != 0) {
       printf("encode_control error, cmd 0x%08x", MPP_ENC_SET_OSD_PLT_CFG);
       return -1;
    }
#endif

    return 0;
}

// pack the encoded h264 data to avpacket
int H264Encoder::encode_encoded_data(pdata_handle h264_data, AVPacket* out_pkt,
                                     bool no_copy)
{
    int ret = 0;
    AVPacket pkt;
    assert(out_pkt);
    av_init_packet(&pkt);
    pkt.data = h264_data->vir_addr;
    pkt.size = h264_data->buf_size;

    if (!no_copy) {
        if (0 != av_copy_packet(out_pkt, &pkt)) {
            printf("in %s, av_copy_packet failed!\n", __func__);
            ret = -1;
        }
    } else {
        *out_pkt = pkt;
        return 0;
    }

    av_packet_unref(&pkt);
    return ret;
}
// encode the raw srcdata to dstbuf, and pack to avpacket
int H264Encoder::encode_video_frame(pdata_handle srcdata, pdata_handle dstbuf, pdata_handle mv_buf,
                                    AVPacket* out_pkt, bool no_copy)
{
    int ret = 0;
    AVStream* st = get_avstream();
    AVCodecContext* c = st->codec;
    AVFrame* frame = video_frame;
    int got_packet = 0;
    AVPacket pkt;
    int w_align, h_align;
    pdata_handle pdata = NULL;

    assert(out_pkt);
    w_align = (frame->width + 15) & (~15);
    h_align = (frame->height + 15) & (~15);
    frame->data[0] = srcdata->vir_addr;
    frame->linesize[0] = w_align;
    frame->data[1] = frame->data[0] + h_align * frame->linesize[0];
    frame->linesize[1] = (w_align >> 1);
    frame->data[2] = frame->data[1] + (h_align >> 1) * frame->linesize[1];
    frame->linesize[2] = (w_align >> 1);
    assert(sizeof(*srcdata) <= sizeof(frame->user_reserve_buf));
    pdata = (pdata_handle)frame->user_reserve_buf;
    *pdata = *srcdata;

    av_init_packet(&pkt);
    assert(2 * sizeof(*dstbuf) <= sizeof(frame->user_reserve_buf));
    pkt.data = dstbuf->vir_addr; // must set
    pkt.size = dstbuf->buf_size;
    memset(pkt.user_reserve_buf, 0, sizeof(pkt.user_reserve_buf));
    pdata = (pdata_handle)pkt.user_reserve_buf;
    *pdata = *dstbuf;
    if (mv_buf)
        *(pdata + 1) = *mv_buf;

#if 0
    FILE *f = fopen("/mnt/sdcard/sss.nv12", "ab");
    assert(f);
    fwrite(srcdata->vir_addr, w_align*h_align*3/2, 1, f);
    fclose(f);
#endif

    if (force_idr) {
        if(encode_control(MPP_ENC_SET_IDR_FRAME, NULL) != 0) {
           printf("encode_control MPP_ENC_SET_IDR_FRAME error!\n");
           return -1;
        }
        force_idr = false;
    }

    /* encode the image */
    ret = avcodec_encode_video2(c, &pkt, frame, &got_packet);
    if (ret < 0) {
        char str[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_strerror(ret, str, AV_ERROR_MAX_STRING_SIZE);
        fprintf(stderr, "Error encoding video frame: %s\n", str);
        ret = -1;
        goto out;
    }
    if (got_packet && pkt.data) {
        assert(pkt.data == dstbuf->vir_addr);
        // av_log(NULL, AV_LOG_ERROR, "Encoding one frame[%d x %d] video takes
        // <%lld> ms\n", w_align, h_align, (av_gettime_relative() - t1)/1000);
        if (!no_copy) {
            if (0 != av_copy_packet(out_pkt, &pkt)) {
                printf("in %s, av_copy_packet failed!\n", __func__);
                ret = -1;
            }
        } else {
            *out_pkt = pkt;
            return 0;
        }
    } else {
        ret = -1;
    }
out:
    frame->data[0] = NULL; // free by outsides
    av_packet_unref(&pkt);
    return ret;
}

int H264Encoder::encode_control(int cmd, void* param)
{
    int ret = 0;
    AVStream* st = get_avstream();
    AVCodecContext* avctx = st->codec;

    ret = avctx->codec->control(avctx, cmd, param);
    if (ret != 0) {
        printf("encode_control avctx %p cmd 0x%08x param %p failed\n", avctx, cmd, param);
        return ret;
    }

    return 0;
}
