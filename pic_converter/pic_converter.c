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
* FILE NAME: pic_converter.c
*
* PURPOSE: the example code of hardware picture resizing or pixel format converting
*
* DEVELOPMENT HISTORY:
* Date        Name       Description
* ---------   ---------- -----------------------------------------------
* 2022-07-20  Apoidea   Initial creating
************************************************************************/
/*
example:
./pic_converter -i ../ffmpeg/out.yuv -o conv.rgb -s 1920,1080,yuv420 -c 640,480,rgba
*/
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

#include "picconverter.h"

static void usage(char *programname)
{
    printf("%s (compiled %s)\n", programname, __DATE__);
    printf(("Usage %s [OPTION]\n"
        " -i <input raw picture file> \n"
        " -o <output converted picture file> \n"
        " -s <input picture format: (width),(height),(pixel format: nv12/yuv420/bgra/rgba)> \n"
        " -c <output picture format: (width),(height),(pixel format: nv12/yuv420/bgra/rgba)> \n"),
        programname);
}

int main(int argc, char *argv[])
{
    int option;
    int rc;
    int ret;
    int i;
    int fconv = -1;
    int fin = -1;
    int in_w = 0, in_h = 0;
    PicFormat_t in_fmt = PIC_FMT_MAX;
    int conv_w = 0, conv_h = 0;
    PicFormat_t conv_fmt = PIC_FMT_MAX;
    char *p;
    int id;
    char fmt[8];

    PicSetting_t pic_conf;
    PIC_CONV_HANDLE_t *pic_conv_h;
    unsigned char *in_buf;
    int  in_size;
    unsigned char *conv_buf;
    int conv_size;

    /* Process options with getopt */
    while ((option = getopt(argc, argv,"i:o:s:c:")) != -1) {
        switch (option) {
            case 'i':
                fin = open(optarg, O_RDONLY, 0777);
                if (fin < 0) {
                    printf("failed to open input picture file: %s(error: %s)\n",
                            optarg, strerror(errno));
                    return -1;
                }

                break;

            case 'o':
                fconv = open(optarg, O_CREAT | O_TRUNC | O_WRONLY, 0777);
                if (fconv < 0) {
                    printf("failed to create output converted file: %s(error: %s)\n",
                            optarg, strerror(errno));
                    return -1;
                }
                break;

            case 's':
                id = 0;
                p = strtok(optarg, ",");
                do {
                    if (id == 0) {
                        in_w = atoi(p);
                    } else if (id == 1) {
                        in_h = atoi(p);
                    } else if (id == 2) {
                        if (!strcmp(p, "nv12")) {
                            in_fmt = PIC_FMT_NV12;
                        } else if (!strcmp(p, "yuv420")) {
                            in_fmt = PIC_FMT_YUV420;
                        } else if (!strcmp(p, "bgra")) {
                            in_fmt = PIC_FMT_ABGR32;
                        } else if (!strcmp(p, "rgba")) {
                            in_fmt = PIC_FMT_ARGB32;
                        }
                        strcpy(fmt, p);
                    }

                    id++;
                } while((p = strtok(NULL, ",")) != NULL);
                printf("input picture: w-%d, h-%d, fmt-%s\n", in_w, in_h, fmt);

                break;

            case 'c':
                id = 0;
                p = strtok(optarg, ",");
                do {
                    if (id == 0) {
                        conv_w = atoi(p);
                    } else if (id == 1) {
                        conv_h = atoi(p);
                    } else if (id == 2) {
                        if (!strcmp(p, "nv12")) {
                            conv_fmt = PIC_FMT_NV12;
                        } else if (!strcmp(p, "yuv420")) {
                            conv_fmt = PIC_FMT_YUV420;
                        } else if (!strcmp(p, "bgra")) {
                            conv_fmt = PIC_FMT_ABGR32;
                        } else if (!strcmp(p, "rgba")) {
                            conv_fmt = PIC_FMT_ARGB32;
                        }
                        strcpy(fmt, p);
                    }
                    id++;
                } while((p = strtok(NULL, ",")) != NULL);
                printf("output picture: w-%d, h-%d, fmt-%s\n", conv_w, conv_h, fmt);

                break;

            default:
                usage(argv[0]);
                exit(0);
                break;
        }
    }

    if (fconv < 0 || fin < 0 ||
        (!in_w || !in_h || in_fmt == PIC_FMT_MAX) ||
        (!conv_w || !conv_h || conv_fmt == PIC_FMT_MAX)) {
        usage(argv[0]);
        exit(0);
    }

    memset(&pic_conf, 0, sizeof(PicSetting_t));

    pic_conf.src.width  = in_w;
    pic_conf.src.height = in_h;
    pic_conf.src.format = in_fmt;

    pic_conf.dest.width  = conv_w;
    pic_conf.dest.height = conv_h;
    pic_conf.dest.format = conv_fmt;

    pic_conv_h = PicConvInit(&pic_conf);
    if (pic_conv_h == NULL) {
        printf("failed to do PicConvInit()\n");
        return -1;
    }

    if (in_fmt == PIC_FMT_YUV420 || in_fmt == PIC_FMT_NV12) {
        in_size = in_w * in_h * 3 / 2;
    } else if (in_fmt == PIC_FMT_ABGR32 || in_fmt == PIC_FMT_ARGB32) {
        in_size = in_w * in_h * 4;
    }

    in_buf = (unsigned char *)malloc(in_size);

    ret = read(fin, in_buf, in_size);
    close(fin);
    if (ret != in_size) {
        printf("read %d bytes but expect %d bytes\n", ret, in_size);
        return -1;
    }

    ret = PicConvProc(pic_conv_h, (void *)in_buf, (void **)&conv_buf, &conv_size);
    if (ret) {
        printf("failed to convert the picture(ret: %d)", ret);
        return -1;
    }

    write(fconv, conv_buf, conv_size);
    close(fconv);
    printf("write out %d bytes converted picture file\n", conv_size);

    PicConvRelease(pic_conv_h);
}