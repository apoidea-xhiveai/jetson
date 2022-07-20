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
* FILE NAME: jpeg_encoder.c
*
* PURPOSE: the example code of hardware encoding the yuv frame into jpeg
*
* DEVELOPMENT HISTORY:
* Date        Name       Description
* ---------   ---------- -----------------------------------------------
* 2022-07-19  Apoidea   Initial creating
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

#include "jpegenc.h"

static void usage(char *programname)
{
    printf("%s (compiled %s)\n", programname, __DATE__);
    printf(("Usage %s [OPTION]\n"
        " -i <yuv420p file> \n"
        " -o <jpeg file> \n"
        " -w <the width of picture> \n"
        " -h <the height of picture> \n"),
        programname);
}

int main(int argc, char *argv[])
{
    int option;
    int rc;
    int ret;
    int i;
    int fjpeg = -1;
    int fyuv = -1;
    int w = 0, h = 0;

    JpegEnSetting_t jpegSetting;
    JPEGEN_HANDLE_t *pic_cap_jpeg_h;
    unsigned char *yuv_buf;
    int  yuv_size;
    unsigned char *jpeg_buf;
    int jpeg_size;

    /* Process options with getopt */
    while ((option = getopt(argc, argv,"i:o:w:h:")) != -1) {
        switch (option) {
            case 'i':
                fyuv = open(optarg, O_RDONLY, 0777);
                if (fyuv < 0) {
                    printf("failed to create input yuv file: %s(error: %s)\n",
                            optarg, strerror(errno));
                    return -1;
                }

                break;

            case 'o':
                fjpeg = open(optarg, O_CREAT | O_TRUNC | O_WRONLY, 0777);
                if (fjpeg < 0) {
                    printf("failed to create output jpeg file: %s(error: %s)\n",
                            optarg, strerror(errno));
                    return -1;
                }
                break;

            case 'w':
                w = atoi(optarg);
                break;

            case 'h':
                h = atoi(optarg);
                break;

            default:
                usage(argv[0]);
                exit(0);
                break;
        }
    }

    if (fjpeg < 0 || fyuv < 0 || !w || !h) {
        usage(argv[0]);
        exit(0);
    }

    memset(&jpegSetting, 0, sizeof(JpegEnSetting_t));

    jpegSetting.width   = w;
    jpegSetting.height  = h;

    pic_cap_jpeg_h = JpegEncoderInit(&jpegSetting);
    if (pic_cap_jpeg_h == NULL) {
        printf("failed to do JpegEncoderInit(w: %d, h: %d)\n", w, h);
        return -1;
    }

    yuv_size = w * h * 3 / 2;
    yuv_buf = (unsigned char *)malloc(yuv_size);

    ret = read(fyuv, yuv_buf, yuv_size);
    close(fyuv);
    if (ret != yuv_size) {
        printf("read %d bytes but expect %d bytes\n", ret, yuv_size);
        return -1;
    }

    ret = JpegEncoderProc(pic_cap_jpeg_h, (void *)yuv_buf, (void **)&jpeg_buf, &jpeg_size);
    if (ret) {
        printf("failed to encode the jpeg picture(ret: %d)", ret);
        return -1;
    }

    write(fjpeg, jpeg_buf, jpeg_size);
    close(fjpeg);
    printf("write out %d bytes jpeg file\n", jpeg_size);
}