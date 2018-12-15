// by wangh
#include "camera_mp4.h"

CameraMp4::CameraMp4()
{
#ifdef DEBUG
    snprintf(class_name, sizeof(class_name), "CameraMp4");
#endif
    snprintf(format, sizeof(format), "mp4");
    direct_piece = true;
    exit_id = MUXER_NORMAL_EXIT;
    no_async = true;  // no cache packet list and no thread handle
    memset(&lbr_pre_timeval, 0, sizeof(struct timeval));
    low_frame_rate_mode = false;
}

CameraMp4::~CameraMp4() {}

bool CameraMp4::is_low_frame_rate()
{
    return low_frame_rate_mode;
}
void CameraMp4::set_low_frame_rate(bool val, struct timeval start_timeval)
{
    low_frame_rate_mode = val;
    printf("set mp4 low bit rate: %d\n", low_frame_rate_mode);
    if (val) {
        lbr_pre_timeval = start_timeval;
    } else {
        memset(&lbr_pre_timeval, 0, sizeof(struct timeval));
    }
}

void CameraMp4::reset_lbr_time()
{
    if (low_frame_rate_mode) {
        memset(&lbr_pre_timeval, 0, sizeof(struct timeval));
    }
}

static void add_one_frame_time(struct timeval& val, int video_sample_rate)
{
    int64_t usec = (int64_t)val.tv_usec + 1000000LL / video_sample_rate;
    val.tv_sec += (usec / 1000000LL);
    val.tv_usec = (usec % 1000000LL);
}

void CameraMp4::push_packet(EncodedPacket* pkt)
{
    //av_log(NULL, AV_LOG_ERROR, "CameraMp4::push_packet, low_frame_rate_mode: %d\n", low_frame_rate_mode);
    if (low_frame_rate_mode) {
        if (pkt->type == AUDIO_PACKET) {
            return;
        }
        // only save the key frame and readjust timestamp
        AVPacket& avpkt = pkt->av_pkt;
        if (!(avpkt.flags & AV_PKT_FLAG_KEY)) {
            av_log(NULL, AV_LOG_ERROR, " non key frame\n");
            if (lbr_pre_timeval.tv_sec == 0 && lbr_pre_timeval.tv_usec == 0) {
                pkt->lbr_time_val = pkt->time_val;
            } else {
                pkt->lbr_time_val = lbr_pre_timeval;
            }
            return;
        }
        av_log(NULL, AV_LOG_ERROR, " key frame\n");
        struct timeval tval = pkt->time_val;

        if ((lbr_pre_timeval.tv_sec == 0 && lbr_pre_timeval.tv_usec == 0) ||
            (((tval.tv_sec - lbr_pre_timeval.tv_sec) * 1000000LL +
              (tval.tv_usec - lbr_pre_timeval.tv_usec)) /
             1000 * video_sample_rate <
             1000)) {
            PRINTF_FUNC_LINE;
            lbr_pre_timeval = tval;
        }
        av_log(NULL, AV_LOG_ERROR, "-lbr_pre_timeval[%d , %d]\n",
               lbr_pre_timeval.tv_sec, lbr_pre_timeval.tv_usec);

        add_one_frame_time(lbr_pre_timeval, video_sample_rate);
        pkt->lbr_time_val = lbr_pre_timeval;
        av_log(NULL, AV_LOG_ERROR, "+lbr_pre_timeval[%d , %d]\n",
               lbr_pre_timeval.tv_sec, lbr_pre_timeval.tv_usec);
    }
    CameraMuxer::push_packet(pkt);
}
