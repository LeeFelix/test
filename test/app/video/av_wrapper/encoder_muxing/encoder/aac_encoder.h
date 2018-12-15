// by rk wangh
#ifndef AAC_ENCODER_H
#define AAC_ENCODER_H

#include "base_encoder.h"

class AACEncoder : public BaseEncoder
{
private:
    AVFrame *audio_frame;
    AVFrame *audio_tmp_frame;
    uint64_t audio_index;
    int samples_count;
    struct SwrContext *swr_ctx;
    AACEncoder();

public:
    ~AACEncoder();
    int encode_init_config(video_encode_config *config);
    void encode_get_audio_buffer(void **buf, int *nb_samples, int *channels);
    int encode_audio_frame(AVPacket *out_pkt, uint64_t &audio_pkt_index);
    // return if there is no remaining audio data
    void encode_flush_audio_data(AVPacket *out_pkt, uint64_t &audio_idx,
                                 bool &finish);
    CREATE_FUNC(AACEncoder)
};

#endif // AAC_ENCODER_H
