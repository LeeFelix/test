#include "vpu.h"
#include <dlfcn.h>
#include "common.h"

enum AVPixelFormat vpu_color_space2ffmpeg(MppFrameFormat mpp_fmt)
{
    switch (mpp_fmt) {
    case MPP_FMT_YUV420P:
                return AV_PIX_FMT_YUV420P;  /* YYYY... UUUU... VVVV */
        case MPP_FMT_YUV420SP:
            return AV_PIX_FMT_NV12;  /* YYYY... UVUVUV...    */
        case MPP_FMT_YUV422_YUYV:
            return AV_PIX_FMT_YVYU422;    /* YUYVYUYV...          */
        case MPP_FMT_YUV422_UYVY:
            return AV_PIX_FMT_UYVY422;    /* UYVYUYVY...          */
        case MPP_FMT_RGB565:
            return AV_PIX_FMT_RGB565LE; /* 16-bit RGB           */
        case MPP_FMT_BGR565:
            return AV_PIX_FMT_BGR565LE; /* 16-bit RGB           */
        case MPP_FMT_RGB555:
            return AV_PIX_FMT_RGB555LE; /* 15-bit RGB           */
        case MPP_FMT_BGR555:
            return AV_PIX_FMT_BGR555LE; /* 15-bit RGB           */
        case MPP_FMT_RGB444:
            return AV_PIX_FMT_RGB444LE; /* 12-bit RGB           */
        case MPP_FMT_BGR444:
            return AV_PIX_FMT_BGR444LE; /* 12-bit RGB           */
        case MPP_FMT_RGB888:
            return AV_PIX_FMT_RGB24;    /* 24-bit RGB           */
        case MPP_FMT_BGR888:
            return AV_PIX_FMT_BGR24;    /* 24-bit RGB           */
        default:
            fprintf(stderr, "unsupport mpp format to ffmpeg: %d", mpp_fmt);
            break;
        }
        return (enum AVPixelFormat)-1;
    }

    int vpu_nv12_encode_jpeg_init(struct vpu_encode* encode,
                                  int width,
                                  int height)
{
    int ret;
    long long size = 0;

    encode->nal = 0x00000001;

    encode->api_enc_in = (VpuApiEncInput*)calloc(1, sizeof(VpuApiEncInput));
    if (!encode->api_enc_in)
        return -1;

    encode->enc_in = &encode->api_enc_in->stream;
    encode->enc_in->buf = NULL;

    encode->enc_out = (EncoderOut_t*)calloc(1, sizeof(EncoderOut_t));
    if (!encode->enc_out)
        return -1;

    if (video_ion_alloc_rational(&encode->jpeg_enc_out, width, height, 2, 1)) {
        printf("%s video ion alloc fail!\n", __func__);
        return -1;
    }

    // encode->enc_out->data = (RK_U8*)calloc(1,width*height);
    // if(!encode->enc_out->data)
    //	return -1;

    size = encode->jpeg_enc_out.size;
    encode->enc_out->data = (RK_U8*)encode->jpeg_enc_out.buffer;
    encode->enc_out->timeUs = encode->jpeg_enc_out.fd + (size << 32);

    ret = vpu_open_context(&encode->ctx);
    if (ret || !encode->ctx)
        return -1;

    /*
     ** now init vpu api context. codecType, codingType, width ,height
     ** are all needed before init.
    */
    encode->ctx->codecType = CODEC_ENCODER;
    encode->ctx->videoCoding = OMX_RK_VIDEO_CodingMJPEG;
    encode->ctx->width = width;
    encode->ctx->height = height;
    encode->ctx->no_thread = 1;

    encode->ctx->private_data =
        (EncParameter_t*)calloc(1, sizeof(EncParameter_t));
    if (!encode->ctx->private_data)
        return -1;

    encode->enc_param = (EncParameter_t*)encode->ctx->private_data;
    memset(encode->enc_param, 0, sizeof(EncParameter_t));
    encode->enc_param->width = width;
    encode->enc_param->height = height;
    encode->enc_param->format = ENC_INPUT_YUV420_SEMIPLANAR;
    encode->enc_param->qp = 10; /* range 0 - 10, worst -> best */

    ret = encode->ctx->init(encode->ctx, NULL, 0);
    if (ret) {
        printf("init vpu api context fail, ret: %d\n", ret);
        return -1;
    }

    encode->size = ((width + 15) & (~15)) * ((height + 15) & (~15)) * 3 / 2;

    encode->api_enc_in->capability = encode->size;

    encode->nal = BSWAP32(encode->nal);
    // printf("%s out\n", __func__);
    return 0;
}

int vpu_nv12_encode_jpeg_doing(struct vpu_encode* encode,
                               void* dstbuf,
                               int src_fd,
                               size_t src_size,
                               int dst_fd)
{
    int ret;
    long long size = 0;

    size = encode->jpeg_enc_out.size;
    encode->enc_out->data = (RK_U8*)encode->jpeg_enc_out.buffer;
    encode->enc_out->timeUs = encode->jpeg_enc_out.fd + (size << 32);

    // printf("%s 1\n", __func__);
    encode->enc_in->buf = dstbuf;
    // printf("read one frame, size: %d, timeUs: %lld\n",
    //	encode->enc_in->size, encode->enc_in->timeUs);

    encode->enc_in->size = src_size;
    encode->enc_in->bufPhyAddr = src_fd;
    encode->enc_in->nFlags = 0;
    encode->enc_in->timeUs = 0;
    // printf("buf %p bufPhyAddr %d\n", encode->enc_in->buf,
    // encode->enc_in->bufPhyAddr);

    ret = encode->ctx->encode(encode->ctx, encode->enc_in, encode->enc_out);
    if (ret)
        return ret;

    // printf("vpu encode one frame, out len: %d, left size: %d",
    //	encode->enc_out->size, encode->enc_in->size);

    /*
     ** encoder output stream is raw bitstream, you need to add nal
     ** head by yourself.
    */
    if (encode->enc_out->size && encode->enc_out->data) {
        if (dst_fd >= 0) {
            // printf("dump %d bytes enc output stream to file",
            //	encode->enc_out->size);
            write(dst_fd, encode->enc_out->data, encode->enc_out->size);
        }
        // encode->enc_out->size = 0;
    }
    // printf("%s 2\n", __func__);
    return 0;
}

void vpu_nv12_encode_jpeg_done(struct vpu_encode* encode)
{
    // printf("%s in\n", __func__);

    encode->enc_in->buf = NULL;

    if (encode->api_enc_in) {
        free(encode->api_enc_in);
        encode->api_enc_in = NULL;
    }

    if (encode->enc_out) {
        // if(encode->enc_out->data) {
        //	free(encode->enc_out->data);
        //	encode->enc_out->data = NULL;
        //}
        free(encode->enc_out);
        encode->enc_out = NULL;
    }

    if (encode->ctx) {
        if (encode->ctx->private_data) {
            free(encode->ctx->private_data);
            encode->ctx->private_data = NULL;
        }

        vpu_close_context(&encode->ctx);
        encode->ctx = NULL;
    }

    video_ion_free(&encode->jpeg_enc_out);
    // printf("%s out\n", __func__);
}

int vpu_decode_jpeg_init(struct vpu_decode* decode, int width, int height)
{
    int ret;
    decode->in_width = width;
    decode->in_height = height;

    ret = mpp_buffer_group_get_internal(&decode->memGroup, MPP_BUFFER_TYPE_ION);
    if (MPP_OK != ret) {
        mpp_err("memGroup mpp_buffer_group_get failed\n");
        return ret;
    }

    ret = mpp_create(&decode->mpp_ctx, &decode->mpi);
    if (MPP_OK != ret) {
        mpp_err("mpp_create failed\n");
        return -1;
    }

    ret = mpp_init(decode->mpp_ctx, MPP_CTX_DEC, MPP_VIDEO_CodingMJPEG);
    if (MPP_OK != ret) {
        mpp_err("mpp_init failed\n");
        return -1;
    }

    MppApi *mpi = decode->mpi;
    MppCtx mpp_ctx = decode->mpp_ctx;
    MppFrame frame;
    ret = mpp_frame_init(&frame);
    if (!frame || (MPP_OK != ret)) {
        mpp_err("failed to init mpp frame!");
        return MPP_ERR_NOMEM;
    }

    mpp_frame_set_fmt(frame, MPP_FMT_YUV420SP);
    mpp_frame_set_width(frame, decode->in_width);
    mpp_frame_set_height(frame, decode->in_height);
    mpp_frame_set_hor_stride(frame, MPP_ALIGN(decode->in_width,16));
    mpp_frame_set_ver_stride(frame, MPP_ALIGN(decode->in_height,16));

    ret = mpi->control(mpp_ctx, MPP_DEC_SET_FRAME_INFO, (MppParam)frame);
    mpp_frame_deinit(&frame);

    return 0;
}

int vpu_decode_jpeg_doing(struct vpu_decode* decode,
                          char* in_data,
                          RK_S32 in_size,
                          int out_fd,
                          char *out_data)
{

    MPP_RET ret = MPP_OK;
    MppBufferGroup  memGroup;
    MppTask task = NULL;

    decode->pkt_size = in_size;
    if (decode->pkt_size <= 0) {
        mpp_err("invalid input size %d\n", decode->pkt_size);
        return VPU_API_ERR_UNKNOW;
    }

    if (NULL == in_data) {
        ret = MPP_ERR_NULL_PTR;
        goto DECODE_OUT;
    }

    /* try import input buffer and output buffer */
    RK_U32 width        = decode->in_width;
    RK_U32 height       = decode->in_height;
    RK_U32 hor_stride   = MPP_ALIGN(width,16);
    RK_U32 ver_stride   = MPP_ALIGN(height,16);
    MppFrame    frame   = NULL;
    MppPacket   packet  = NULL;
    MppBuffer   str_buf = NULL; /* input */
    MppBuffer   pic_buf = NULL; /* output */
    MppCtx mpp_ctx = decode->mpp_ctx;
    MppApi *mpi = decode->mpi;

    ret = mpp_frame_init(&frame);
    if (MPP_OK != ret) {
        mpp_err_f("mpp_frame_init failed\n");
        goto DECODE_OUT;
    }

    ret = mpp_buffer_get(decode->memGroup, &str_buf, decode->pkt_size);
    if (ret) {
        mpp_err_f("allocate input picture buffer failed\n");
        goto DECODE_OUT;
    }
    memcpy((RK_U8*) mpp_buffer_get_ptr(str_buf), in_data, decode->pkt_size);

    if (out_fd > 0) {
        MppBufferInfo outputCommit;

        memset(&outputCommit, 0, sizeof(outputCommit));
        /* in order to avoid interface change use space in output to transmit information */
        outputCommit.type = MPP_BUFFER_TYPE_ION;
        outputCommit.fd = out_fd;
        outputCommit.size = hor_stride * ver_stride * 2;
        outputCommit.ptr = (void*)out_data;

        ret = mpp_buffer_import(&pic_buf, &outputCommit);
        if (ret) {
            mpp_err_f("import output stream buffer failed\n");
            goto DECODE_OUT;
        }
    } else {
        ret = mpp_buffer_get(decode->memGroup, &pic_buf, hor_stride * ver_stride * 2);
        if (ret) {
            mpp_err_f("allocate output stream buffer failed\n");
            goto DECODE_OUT;
        }
    }

    mpp_packet_init_with_buffer(&packet, str_buf); /* input */
    mpp_frame_set_buffer(frame, pic_buf); /* output */

    //printf("mpp import input fd %d output fd %d\n",
    //       mpp_buffer_get_fd(str_buf), mpp_buffer_get_fd(pic_buf));

    do {
        ret = mpi->dequeue(mpp_ctx, MPP_PORT_INPUT, &task);
        if (ret) {
            mpp_err("mpp task input dequeue failed\n");
            goto DECODE_OUT;
        }

        if (task == NULL) {
            printf("mpi dequeue from MPP_PORT_INPUT fail, task equal with NULL!\n");
            usleep(3*1000);
        } else
            break;
    } while (1);

    mpp_task_meta_set_packet(task, KEY_INPUT_PACKET, packet);
    mpp_task_meta_set_frame (task, KEY_OUTPUT_FRAME, frame);

    if (mpi != NULL) {
        ret = mpi->enqueue(mpp_ctx, MPP_PORT_INPUT, task);
        if (ret) {
            mpp_err("mpp task input enqueue failed\n");
            goto DECODE_OUT;
        }
        task = NULL;

        do {
            ret = mpi->dequeue(mpp_ctx, MPP_PORT_OUTPUT, &task);
            if (ret) {
                mpp_err("ret %d mpp task output dequeue failed\n", ret);
                goto DECODE_OUT;
            }

            if (task) {
                MppFrame frame_out = NULL;
                decode->fmt = MPP_FMT_YUV420SP;

                mpp_task_meta_get_frame(task, KEY_OUTPUT_FRAME, &frame_out);
                mpp_assert(frame_out == frame);
                decode->fmt = mpp_frame_get_fmt(frame_out);
                if (MPP_FMT_YUV422SP != decode->fmt && MPP_FMT_YUV420SP != decode->fmt)
                    printf("No support USB JPEG decode format!\n");

                ret = mpi->enqueue(mpp_ctx, MPP_PORT_OUTPUT, task);
                if (ret) {
                    mpp_err("mpp task output enqueue failed\n");
                    goto DECODE_OUT;
                }
                task = NULL;
                break;
            }
            usleep(3*1000);
        } while (1);
    } else {
        mpp_err("mpi pointer is NULL, failed!");
    }

DECODE_OUT:
    if (str_buf) {
        mpp_buffer_put(str_buf);
        str_buf = NULL;
    }

    if (pic_buf) {
        mpp_buffer_put(pic_buf);
        pic_buf = NULL;
    }

    if (frame)
        mpp_frame_deinit(&frame);

    if (packet)
        mpp_packet_deinit(&packet);

    return 0;
}

int vpu_decode_jpeg_done(struct vpu_decode* decode)
{

    MPP_RET ret = MPP_OK;
    ret = mpp_destroy(decode->mpp_ctx);
    if(ret != MPP_OK) {
        mpp_err("something wrong with mpp_destroy! ret:%d\n", ret);
    }

    if (decode->memGroup) {
        mpp_buffer_group_put(decode->memGroup);
        decode->memGroup = NULL;
    }

    return ret;
}
