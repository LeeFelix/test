#ifndef __VPU_ENCODE_H__
#define __VPU_ENCODE_H__

#include <memory.h>
#include <stdint.h>

#include <stdio.h>

#include <mpp/vpu.h>
#include <mpp/mpp_log.h>
#include <mpp/vpu_api.h>
#include <mpp/rk_mpi.h>
#include <libavutil/pixfmt.h>

#include "common.h"

#define BSWAP32(x)                                      \
  ((((x)&0xff000000) >> 24) | (((x)&0x00ff0000) >> 8) | \
   (((x)&0x0000ff00) << 8) | (((x)&0x000000ff) << 24))
#define MPP_ALIGN(x, a)         (((x)+(a)-1)&~((a)-1))

typedef struct VpuApiEncInput {
    EncInputStream_t stream;
    RK_U32 capability;
} VpuApiEncInput;

struct vpu_encode {
	struct VpuCodecContext *ctx;
	RK_S32 nal;
	RK_S32 size;
	EncoderOut_t *enc_out;
	VpuApiEncInput *api_enc_in;
	EncInputStream_t *enc_in;
	EncParameter_t *enc_param;
	struct video_ion jpeg_enc_out;
};

struct vpu_decode {
	int in_width;
	int in_height;
	RK_S32 pkt_size;
	MppCtx mpp_ctx;
	MppApi *mpi;
	MppBufferGroup memGroup;
	MppFrameFormat fmt;
};

int vpu_nv12_encode_jpeg_init(struct vpu_encode* encode,
                              int width,
                              int height);

int vpu_nv12_encode_jpeg_doing(struct vpu_encode* encode,
                               void* dstbuf,
                               int src_fd,
                               size_t src_size,
                               int dst_fd);

void vpu_nv12_encode_jpeg_done(struct vpu_encode* encode);

int vpu_decode_jpeg_init(struct vpu_decode* decode, int width, int height);

int vpu_decode_jpeg_doing(struct vpu_decode* decode,
                          char* in_data,
                          RK_S32 in_size,
                          int out_fd,
                          char *out_data);

int vpu_decode_jpeg_done(struct vpu_decode* decode);

inline enum AVPixelFormat vpu_color_space2ffmpeg(MppFrameFormat mpp_fmt);

#endif
