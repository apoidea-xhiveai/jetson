/*
* All Rights Reserved
*
* APOIDEA CONFIDENTIAL
* Copyright 2021 Apoidea Technology All Rights Reserved.
* The source code contained or described herein and all documents related to
* the source code ("Material") are owned by Apoidea International Ltd or its
* suppliers or licensors. Title to the Material remains with Apoidea International Ltd
* or its suppliers and licensors. The Material contains trade secrets and
* proprietary and confidential information of Apoidea or its suppliers and
* licensors. The Material is protected by worldwide copyright and trade secret
* laws and treaty provisions. No part of the Material may be used, copied,
* reproduced, modified, published, uploaded, posted, transmitted, distributed,
* or disclosed in any way without Apoidea's prior express written permission.
*
* No license under any patent, copyright, trade secret or other intellectual
* property right is granted to or conferred upon you by disclosure or delivery
* of the Materials, either expressly, by implication, inducement, estoppel or
* otherwise. Any license under such intellectual property rights must be
* express and approved by Apoidea in writing.
*/

/***********************************************************************
* FILE NAME: jpegenc.h
*
* PURPOSE: header file of Jetson jpeg encoder library
*
* DEVELOPMENT HISTORY:
* Date        Name       Description
* ---------   ---------- -----------------------------------------------
* 2021-10-28  Jun Yu     Initial creating
************************************************************************/

#ifndef __JPEG_ENCODER_H_
#define __JPEG_ENCODER_H_

__BEGIN_DECLS

#ifndef JPEGEN_IN
#define JPEGEN_IN
#endif

#ifndef JPEGEN_OUT
#define JPEGEN_OUT
#endif

#ifndef JPEGEN_INOUT
#define JPEGEN_INOUT
#endif

#define ENV_JPEG_ENCODER_DEBUG "JPEG_ENCODE_DEBUG"

typedef void* JPEGEN_HANDLE_t;

/*Must input YUV420 to Jpeg Encoder
  Other pixel formats need to be converted to YUV420
*/
typedef enum {
    JPEGEN_PIC_FMT_YUV420 = 0,  // YYYY.....U....V...
    JPEGEN_PIC_FMT_NV12,        // YYYY.....UV....
    JPEGEN_PIC_FMT_NV21,        // YYYY.....VU....
    JPEGEN_PIC_FMT_UYVY,        // UYVY.....
    JPEGEN_PIC_FMT_YUYV,        // YUYV.....
    JPEGEN_PIC_FMT_YVYU,        // YVYU.....
    JPEGEN_PIC_FMT_YUV444,      // Y...U...V

    JPEGEN_PIC_FMT_ABGR32,      // BGRA.....
    JPEGEN_PIC_FMT_XRGB32,      // RGBX.....
    JPEGEN_PIC_FMT_ARGB32,      // RGBA.....

    JPEGEN_PIC_FMT_GRAY8,       // GRAY8.....

    JPEGEN_PIC_FMT_MAX
} JpegEnPicFormat_t;

/*default disable: all 0*/
typedef struct _JpegEnPicCropRect_t {
    int x; /*left*/
    int y; /*top*/
    int w; /*width*/
    int h; /*height*/
} JpegEnPicCropRect_t;

/*Scale encoding with given scaled width and height encoder*/
/*default disable: all 0*/
typedef struct _JpegEnPicScale_t {
    int w; /*scale width*/
    int h; /*scale height*/
} JpegEnPicScale_t;

typedef struct _JpegEnSetting_t {
    int                 width;
    int                 height;
    JpegEnPicFormat_t   fmt;     /*default: JPEGEN_PIC_FMT_YUV420*/
    JpegEnPicScale_t    scale;
    JpegEnPicCropRect_t crop;

    int                 quality; /*0: [75(default)]*/
    int                 play_id; /*1, 2, 3...*/
} JpegEnSetting_t;


/*
* Initialize the jpeg encoder
*/
JPEGEN_HANDLE_t JpegEncoderInit(JPEGEN_IN JpegEnSetting_t *config);

/*
* Encode the YUV format picture into the jpeg format picture
*
* JPEGEN_HANDLE_t handle: the jpeg encoder handler
* void *src_pic:          YUV420/NV12 format picture data
* void **dest_jpeg:       return the encoded jpeg picture data
* int *dest_jpeg_sz:      the size of the jpeg picture
*/
int JpegEncoderProc(JPEGEN_IN JPEGEN_HANDLE_t handle,
                    JPEGEN_IN  void           *src_pic,
                    JPEGEN_OUT void           **dest_jpeg,
                    JPEGEN_OUT int            *dest_jpeg_sz);

/*
* Encode the YUV format picture into the jpeg format picture
*
* JPEGEN_HANDLE_t handle: the jpeg encoder handler
* void *avframe:          ffmpeg AVFrame*
* void **dest_jpeg:       return the encoded jpeg picture data
* int *dest_jpeg_sz:      the size of the jpeg picture
*/
int JpegEncoderProc_ffmpeg(JPEGEN_IN JPEGEN_HANDLE_t handle,
                           JPEGEN_IN  void           *avframe,
                           JPEGEN_OUT void           **dest_jpeg,
                           JPEGEN_OUT int            *dest_jpeg_sz);

/*
* Release the jpeg encoder
*/
void JpegEncoderRelease(JPEGEN_IN  JPEGEN_HANDLE_t handle);

__END_DECLS

#endif /* __JPEG_ENCODER_H_ */