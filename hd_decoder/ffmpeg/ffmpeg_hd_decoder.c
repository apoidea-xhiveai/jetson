/*
 * Copyright (c) 2022 Apoidea Technology
 *
 * This file is part of Jeson Example Codes.
 *
 * It is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * It is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
/***********************************************************************
* FILE NAME: ffmpeg_hd_decoder.c
*
* PURPOSE: the example code of hardware decoding the video stream with ffmpeg
*
* DEVELOPMENT HISTORY:
* Date        Name       Description
* ---------   ---------- -----------------------------------------------
* 2022-07-18  Apoidea   Initial creating
************************************************************************/
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/time.h>

#include <pthread.h>
#include <errno.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavutil/pixdesc.h>
#include <libavutil/parseutils.h>

typedef struct runtime_t {
    AVFormatContext *ff_input_ctx;
    AVStream        *ff_vst; /*video stream*/
    AVCodec         *ff_vcodec;
    AVCodecContext  *ff_vdec_ctx;
    int             vstrm_index;

    char            *url;
    int             fyuv;
    int             count;

    pthread_t       dec_thread;
    int             dec_quit;

    int             stream_data_size;
    int             stream_nb_packets;
} runtime_t;


static void usage(char *programname)
{
    printf("%s (compiled %s)\n", programname, __DATE__);
    printf(("Usage %s [OPTION]\n"
        " -i <video streaem url>                 : video stream in rtsp/http/file ... \n"
        " -o <decoded yuv file>                  : output file path\n"
        " -c <number of yuv frames>(default: 1)  : the number of video frames\n"
        " -h, --help                             : print this help and exit\n"),
        programname);
}

static void ff_print_error(const char *filename, int err)
{
    char errbuf[128];
    const char *errbuf_ptr = errbuf;

    if (av_strerror(err, errbuf, sizeof(errbuf)) < 0) {
        errbuf_ptr = strerror(AVUNERROR(err));
    }

    printf("%s: %s\n", filename, errbuf_ptr);
}

static const char *ff_error_string(int err)
{
    static char errbuf[128];
    const char *errbuf_ptr = errbuf;

    if (av_strerror(err, errbuf, sizeof(errbuf)) < 0) {
        errbuf_ptr = strerror(AVUNERROR(err));
    }

    return errbuf_ptr;
}

static AVCodec *find_codec_by_name(const char *name, enum AVMediaType type, int encoder)
{
    const AVCodecDescriptor *desc;
    const char *codec_string = encoder ? "encoder" : "decoder";
    AVCodec *codec;

    codec = encoder ?
        avcodec_find_encoder_by_name(name) :
        avcodec_find_decoder_by_name(name);

    if (!codec && (desc = avcodec_descriptor_get_by_name(name))) {
        codec = encoder ? avcodec_find_encoder(desc->id) :
                          avcodec_find_decoder(desc->id);
        if (codec) {
            printf("Matched %s '%s' for codec '%s'.\n",
                    codec_string, codec->name, desc->name);
        }
    }

    if (!codec) {
        printf("Unknown %s '%s'\n", codec_string, name);
        return NULL;
    }

    if (codec->type != type) {
        printf("Invalid %s type '%s'\n", codec_string, name);
        return NULL;
    }

    return codec;
}

/*
The interrupt_callback is not an error callback; it is called periodically
during length operations. You need to return 0 from your decode_interrupt()
if you want to continue processing. Otherwise, you get the "immediate exit
requested" error.
*/
static int input_interrupt_cb(void *ctx) {
    runtime_t *rt = (runtime_t *)ctx;

    return 0;
}

static int open_stream(runtime_t *rt) {
    AVDictionary *avfmt_dict = NULL;
    char data[32];
    int err;

    rt->ff_input_ctx = avformat_alloc_context();
    if (!rt->ff_input_ctx) {
        printf("Failed to allocate avformat context\n");
        return -1;
    }

    rt->ff_input_ctx->interrupt_callback.callback = input_interrupt_cb;
    rt->ff_input_ctx->interrupt_callback.opaque   = rt;

    if (!strncmp(rt->url, "rtsp:", 5)) {
        /* To avoid waiting for a long time if the rtsp stream is not be reachable
          set avformat_open_input() timeout in us*/
        av_dict_set_int(&avfmt_dict, "stimeout", 10 * 1000 * 1000/*seconds*/, 0);
        printf("set 'stimeout' to 10s\n");
    }

    err = avformat_open_input(&rt->ff_input_ctx, rt->url, NULL, &avfmt_dict);
    if ( err < 0) {
        ff_print_error(rt->url, err);

        printf("avformat_open_input(%s) -- Failure!\n", rt->url);

        return -1;
    } else {
        printf("avformat_open_input(%s) -- OK!\n", rt->url);
    }

    return 0;
}

static void close_stream(runtime_t *rt) {
    if (rt->ff_input_ctx) {
        avformat_close_input(&rt->ff_input_ctx);
        rt->ff_input_ctx = NULL;
    }

    if (rt->ff_vdec_ctx) {
        avcodec_close(rt->ff_vdec_ctx);
        avcodec_free_context(&rt->ff_vdec_ctx);
    }
}

// This does not quite work like avcodec_decode_audio4/avcodec_decode_video2.
// There is the following difference: if you got a frame, you must call
// it again with pkt=NULL. pkt==NULL is treated differently from pkt->size==0
// (pkt==NULL means get more output, pkt->size==0 is a flush/drain packet)
static int decode(AVCodecContext *avctx, AVFrame *frame, int *got_frame, AVPacket *pkt)
{
    int ret;

    *got_frame = 0;

    if (pkt) {
        ret = avcodec_send_packet(avctx, pkt);
        // In particular, we don't expect AVERROR(EAGAIN), because we read all
        // decoded frames with avcodec_receive_frame() until done.
        if (ret < 0 && ret != AVERROR_EOF) {
            printf("avcodec_send_packet() - [error: %s]\n", av_err2str(ret));
            return ret;
        }
    }

    ret = avcodec_receive_frame(avctx, frame);
    if (ret < 0 && ret != AVERROR(EAGAIN)) {
        /*
            got AVERROR_EOF at the end of stream
        */
        printf("avcodec_receive_frame() - [error: %s]\n", av_err2str(ret));
        return ret;
    }
    if (ret >= 0)
        *got_frame = 1;

    return 0;
}

static int get_input_packet(runtime_t *rt, AVPacket *pkt) {
    if (rt->ff_input_ctx) {
        return av_read_frame(rt->ff_input_ctx, pkt);
    } else {
        printf("ff_input_ctx is NULL!\n");
        return -1;
    }
}

static void process_input_packet(runtime_t *rt, AVPacket *pkt) {
    static int w_loop = 0;

    int ret;
    int i;
    int got_output = 0;
    AVPacket *pkt_tmp = pkt;

    // With fate-indeo3-2, we're getting 0-sized packets before EOF for some
    // reason. This seems like a semi-critical bug. Don't trigger EOF, and
    // skip the packet.
    if (pkt && pkt->data != NULL && pkt->size == 0)
        return;

    do {
        AVFrame *frm = av_frame_alloc();

        ret = decode(rt->ff_vdec_ctx, frm, &got_output, pkt_tmp);
        if (ret < 0) {
            printf("failed to decode the video frame\n");
            got_output = 0;
        }

        if (ret != AVERROR_EOF) {
            if (frm->flags & AV_FRAME_FLAG_CORRUPT) {
                printf("corrupt decoded video frame\n");
                got_output = 0;
            }
        } else {
            printf("reach the end of stream");
            av_frame_free(&frm);
            break;
        }

        if (got_output) {
            if (rt->fyuv) {
                if (w_loop < rt->count) {
                    /*
                    video: only yuv420p or nv12 are available
                    */

                    if (frm->format == AV_PIX_FMT_YUV420P) {
                        /*copy Y*/
                        for (i = 0; i < frm->height; i++) {
                            write(rt->fyuv,
                                  frm->data[0] + i * frm->linesize[0],
                                  frm->width);
                        }

                        /*copy U*/
                        for (i = 0; i < frm->height/2; i++) {
                            write(rt->fyuv,
                                  frm->data[1] + i * frm->linesize[1],
                                  frm->width/2);
                        }

                        /*copy V*/
                        for (i = 0; i < frm->height/2; i++) {
                            write(rt->fyuv,
                                  frm->data[2] + i * frm->linesize[2],
                                  frm->width/2);
                        }

                        printf("write out %d YUV420P frame(decoded %d bytes, %d packets)\n",
                                w_loop + 1, rt->stream_data_size, rt->stream_nb_packets);
                    } else if (frm->format == AV_PIX_FMT_NV12) {
                        /*copy Y*/
                        for (i = 0; i < frm->height; i++) {
                            write(rt->fyuv,
                                  frm->data[0] + i * frm->linesize[0],
                                  frm->width);
                        }

                        /*copy UV*/
                        for (i = 0; i < frm->height/2; i++) {
                            write(rt->fyuv,
                                  frm->data[1] + i * frm->linesize[1],
                                  frm->width);
                        }

                        printf("write out %d NV12 frame(decoded %d bytes, %d packets)\n",
                                w_loop + 1, rt->stream_data_size, rt->stream_nb_packets);
                    } else {
                        printf("invalid avframe format: %d", frm->format);
                    }
                } else if (w_loop == rt->count) {
                    printf("close the file after %d yuv frames were written\n", w_loop);
                    close(rt->fyuv);
                    rt->fyuv = -1;
                    rt->dec_quit = 1;
                }
                w_loop++;
            }
        }
        av_frame_free(&frm);
    } while (got_output);
}

static void *decThreadEntry(void *priv) {
    runtime_t *rt = (runtime_t *)priv;
    int ret;

    while (!rt->dec_quit) {
        AVPacket pkt;   /*parsed input stream packet*/
        AVPacket *pPkt = NULL;

        ret = get_input_packet(rt, &pkt);

        if (ret < 0) {
            if (ret == AVERROR(EAGAIN)) {
                printf("no packets were available!\n");
                continue;
            } else if (ret == AVERROR_EOF) {
                printf("reach the end of stream, quit!\n");
                rt->dec_quit = 1;
                break;
            } else {
                printf("error(%s) happens in av_read_frame()\n",
                        ff_error_string(ret));
                continue;
            }
        } else if (ret == 0) {
            if (pkt.stream_index == rt->vstrm_index) {
                rt->stream_data_size += pkt.size;
                rt->stream_nb_packets++;
            } else {
                /*the packet is not belong to the stream*/
                av_packet_unref(&pkt);
                continue;
            }

            if (pkt.flags & AV_PKT_FLAG_CORRUPT) {
                printf("corrupt input packet, discard it\n");
                av_packet_unref(&pkt);
                continue;
            }

            pPkt = &pkt;
        }

        if (!pPkt) {
            /* EOF handling */
            av_init_packet(&pkt);
            pkt.data = NULL;
            pkt.size = 0;

            pPkt = &pkt;
        }

        process_input_packet(rt, pPkt);

        av_packet_unref(pPkt);
    }

    printf("Exit the decThreadEntry() thread!\n");
}

int main(int argc, char *argv[])
{
    int option;
    int rc;
    int ret;
    int i;
    int valid_vcodec = 0;
    AVDictionary *codec_dict = NULL;

    runtime_t *rt = NULL;

    rt = (runtime_t *)malloc(sizeof(runtime_t));
    if (rt == NULL) {
        printf("failed to  malloc runtime_t\n");
        return -1;
    }

    memset(rt, 0, sizeof(runtime_t));
    rt->count = 1;

    /* Process options with getopt */
    while ((option = getopt(argc, argv,"i:o:c:h")) != -1) {
        switch (option) {
            case 'i':
                rt->url = strdup(optarg);
                break;

            case 'o':
                rt->fyuv = open(optarg, O_CREAT | O_TRUNC | O_WRONLY, 0777);
                if (!rt->fyuv) {
                    printf("failed to create output file: %s(error: %s)\n",
                            optarg, strerror(errno));
                    return -1;
                }
                break;

            case 'c':
                rt->count = atoi(optarg);
                break;

            case 'h':
            default:
                usage(argv[0]);
                exit(0);
                break;
        }
    }

    if (!rt->url || !rt->fyuv) {
        usage(argv[0]);
        exit(0);
    }

    avformat_network_init();
    rc = open_stream(rt);
    if (rc) {
        return -1;
    }

    ret = avformat_find_stream_info(rt->ff_input_ctx, NULL);

    if (ret < 0) {
        printf("%s: could not find codec parameters\n", rt->url);
        if (rt->ff_input_ctx->nb_streams == 0) {
            printf("%s: nb_streams is 0, avformat_close_input()!\n",
                        rt->url);
        }

        return -1;
    }

    printf("%s: found %d streams\n", rt->url, rt->ff_input_ctx->nb_streams);

    for (i = 0; i < rt->ff_input_ctx->nb_streams; i++) {
        AVStream *st = rt->ff_input_ctx->streams[i];
        AVCodecParameters *par = st->codecpar;

        if (par->codec_type == AVMEDIA_TYPE_VIDEO) {
            rt->ff_vst = st;

            switch (par->codec_id) {
                case AV_CODEC_ID_H264:
                    /*h.264*/
                    rt->ff_vcodec = find_codec_by_name("h264_nvv4l2dec",
                                                       par->codec_type,
                                                       0 /*decoder*/);
                    valid_vcodec = 1;
                    break;

                case AV_CODEC_ID_HEVC:
                    /*h.265*/
                    rt->ff_vcodec = find_codec_by_name("hevc_nvv4l2dec",
                                                       par->codec_type,
                                                       0 /*decoder*/);
                    valid_vcodec = 1;
                    break;

                case AV_CODEC_ID_MPEG2VIDEO:
                    /*mpeg2*/
                    rt->ff_vcodec = find_codec_by_name("mpeg2_nvv4l2dec",
                                                       par->codec_type,
                                                       0 /*decoder*/);
                    valid_vcodec = 1;
                    break;

                case AV_CODEC_ID_MPEG4:
                    /*mpeg4*/
                    rt->ff_vcodec = find_codec_by_name("mpeg4_nvv4l2dec",
                                                       par->codec_type,
                                                       0 /*decoder*/);
                    valid_vcodec = 1;
                    break;

                case AV_CODEC_ID_VP8:
                    /*vp8*/
                    rt->ff_vcodec = find_codec_by_name("vp8_nvv4l2dec",
                                                       par->codec_type,
                                                       0 /*decoder*/);
                    valid_vcodec = 1;
                    break;

                case AV_CODEC_ID_VP9:
                    /*vp9*/
                    rt->ff_vcodec = find_codec_by_name("vp9_nvv4l2dec",
                                                       par->codec_type,
                                                       0 /*decoder*/);
                    valid_vcodec = 1;
                    break;

                default:
                    printf("[Vcodec: %s]: Unsupported for Jetson HW decoder, "
                            "use SW decoder", avcodec_get_name(par->codec_id));
                    rt->ff_vcodec = avcodec_find_decoder(par->codec_id);
            }

            if (rt->ff_vcodec != NULL) {
                if (valid_vcodec) {
                    rt->ff_vst->codecpar->codec_id = rt->ff_vcodec->id;
                }

                printf("[%s - %s] Decoded vstream: w(%d), h(%d), bitrate(%ld)\n",
                        rt->ff_vcodec->name,
                        rt->ff_vcodec->long_name,
                        par->width,
                        par->height,
                        par->bit_rate);

                if (rt->ff_vcodec->pix_fmts) {
                    /*it is an array of pixel formats, the codec chooses the
                      first element of the array which isn't a hardware-accel-only
                      pixel format*/
                    const enum AVPixelFormat *p = rt->ff_vcodec->pix_fmts;
                    const char *name = av_get_pix_fmt_name(*p);

                    printf("Decoded pixel format: %s\n", name);
                }

                rt->vstrm_index = i;
            } else {
                printf("Failed to get AVCodec for %s\n", avcodec_get_name(par->codec_id));
                return -1;
            }
        }
    }

    if (rt->ff_vcodec != NULL) {
        rt->ff_vdec_ctx = avcodec_alloc_context3(rt->ff_vcodec);
        if (! rt->ff_vdec_ctx) {
            printf("Error(%s) allocating the video decoder context\n", av_err2str(ret));
            return -1;
        }

        ret = avcodec_parameters_to_context(rt->ff_vdec_ctx, rt->ff_vst->codecpar);
        if (ret < 0) {
            printf("Error(%s) initializing the video decoder context.", av_err2str(ret));
            return -1;
        }

        ret = avcodec_open2(rt->ff_vdec_ctx, rt->ff_vcodec, &codec_dict);
        if (ret < 0) {
            printf("Error(%s) opening the video codec\n", av_err2str(ret));
            return -1;
        }
    }

    ret = pthread_create(&rt->dec_thread, NULL,
                         decThreadEntry, (void *)rt);

    if (ret != 0) {
        errno = ret;
        printf("Failed to create decoder thread(res=%d, error=%s)\n",
                ret, strerror(errno));
        return -1;
    }

    pthread_join(rt->dec_thread, NULL);
    printf("End of video decoding!\n");

    close_stream(rt);
    free(rt);

    return 0;
}