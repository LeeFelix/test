#include "md_processor.h"

VPUMDProcessor::VPUMDProcessor()
{
    h264_encoder = NULL;
    width_align = -1;
    height_align = -1;
    mv_len_per_frame = 0;
    frame_rate = 0;
    frame_inverval = 0;
    times = 0;
    memset(&attr, 0, sizeof(attr));
    buffer = NULL;
    result_collecs = NULL;
    memset(&result, 0, sizeof(result));
    result_cb = NULL;
    cb_arg = NULL;
    low_frame_rate = -1;
}

VPUMDProcessor::~VPUMDProcessor()
{
    if (result_collecs)
        free(result_collecs);
    if (buffer)
        free(buffer);
}

void VPUMDProcessor::init(int image_width, int image_height, int rate)
{
    width_align = (image_width + 255) & (~255);
    height_align = (image_height + 15) & (~15);
    assert(sizeof(MppEncMDBlkInfo) == sizeof(RK_U32));
    mv_len_per_frame = width_align * height_align / (16 * 16) * sizeof(MppEncMDBlkInfo);
    frame_rate = rate;
    frame_inverval = rate / 2; //default 500ms
}

int VPUMDProcessor::set_attributes(MDAttributes* in_attr)
{
    uint8_t* buf = NULL;
    attr = *in_attr;
    frame_inverval = frame_rate * in_attr->time_interval / 1000;
    if (frame_inverval < 1)
        frame_inverval = 1;
    result_collecs = (uint32_t*)malloc(frame_inverval * sizeof(uint32_t));
    if (!result_collecs)
        return -1;
    buf = (uint8_t*)malloc(mv_len_per_frame * in_attr->buffered_frames);
    if (buf) {
        for (int i = 0; i < in_attr->buffered_frames; i++) {
            buf_queue.push(buf + (i * mv_len_per_frame));
        }
        buffer = buf;
        return 0;
    } else {
        free(result_collecs);
        result_collecs = NULL;
    }
    return -1;
}

#define EXTRACOEFF 4

uint32_t VPUMDProcessor::simple_process(void* mv_data)
{
    //very simple example
    //printf("\n");
    MppEncMDBlkInfo* data = (MppEncMDBlkInfo*)mv_data;
    int num = mv_len_per_frame / sizeof(MppEncMDBlkInfo);
    uint32_t change_pixels = 0;
    for (int i = 0; i < num; i++) {
        RK_U32 sad = data[i].sad;
        RK_S32 mvx = data[i].mvx;
        RK_S32 mvy = data[i].mvy;
        //printf("[sad mvx mvy]0x%x, 0x%x, 0x%x ", sad, mvx, mvy);
        //if (0 == (num + 1) % (width_align / 16)) {
        //    printf("\n");
        //}
        if (sad > 0xff*EXTRACOEFF) {
            //means change?
            change_pixels++;
        }
    }
    //printf("\n");
    //printf("change_pixels: %d, num : %d\n", change_pixels, num);
    return change_pixels;
}

int VPUMDProcessor::push_mv_data(void* mv_data, void* raw_yuv, size_t yuv_size)
{
    //very simple example
    UNUSED(raw_yuv); //unused right now, TODO
    UNUSED(yuv_size);
    uint32_t ret = 0;
    //if (times == 0) {
#if 0
    void* buf = buf_queue.front();
    buf_queue.pop();
    memcpy(buf, mv_data, mv_len_per_frame);
    buf_queue.push(buf);
#else
    void* buf = mv_data;
#endif
    //do in other thread??
    ret = simple_process(buf);
    //}
    result_collecs[times] = ret;
    if (++times >= frame_inverval) {
        times = 0;
        int num = mv_len_per_frame / sizeof(MppEncMDBlkInfo);
        uint64_t change_pixels = result_collecs[0];
        for (int i = 1; i < frame_inverval; i++) {
            change_pixels += result_collecs[i];
        }
        change_pixels /= frame_inverval;
        printf("change_pixels: %llu, num : %d\n", change_pixels, num);
        if ((uint32_t)change_pixels * EXTRACOEFF * 100 > attr.weight * num) {
            result.change = true;
        }
        if (result_cb) {
            result_cb(result, cb_arg);
        }
        result.change = false;
    }
    return 0;
}
