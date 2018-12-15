// by rk wangh
#ifndef H264_ENCODER_H
#define H264_ENCODER_H

#include "base_encoder.h"

extern "C" {
#include <cvr_ffmpeg_shared.h>
}

#include <mpp/rk_mpi.h>

class H264Encoder : public BaseEncoder
{
private:
    AVFrame* video_frame;
    bool force_idr;

protected:
    H264Encoder();

public:
    virtual ~H264Encoder();
    int encode_init_config(video_encode_config* config);
    // pack the encoded h264 data to avpacket
    int encode_encoded_data(pdata_handle h264_data, AVPacket* out_pkt,
                            bool no_copy = false);
    // encode the raw srcdata to dstbuf, and pack to avpacket
    int encode_video_frame(pdata_handle srcdata, pdata_handle dstbuf,
                           pdata_handle mv_buf, AVPacket* out_pkt,
                           bool no_copy = false);
    // control before encoding width cmd and param
    int encode_control(int cmd, void* param);
    void set_force_encode_idr(bool val) {
        force_idr = val;
    }
    bool get_force_encode_idr() {
        return force_idr;
    }
    CREATE_FUNC(H264Encoder)
};

#endif // H264_ENCODER_H
