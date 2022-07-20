/*
* All Rights Reserved
*
* APOIDEA CONFIDENTIAL
* Copyright 2017 Apoidea Technology All Rights Reserved.
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
* FILE NAME: picconverter.h
*
* PURPOSE: header file of picture converter library for Jetson
*
* DEVELOPMENT HISTORY:
* Date        Name       Description
* ---------   ---------- -----------------------------------------------
* 2021-10-28  Jun Yu     Initial creating
************************************************************************/

#ifndef __PIC_CONVERTER_H_
#define __PIC_CONVERTER_H_

__BEGIN_DECLS

#ifdef MODULE_TAG
#undef MODULE_TAG
#endif
#define MODULE_TAG "pic_conv"

#ifndef PIC_CONV_IN
#define PIC_CONV_IN
#endif

#ifndef PIC_CONV_OUT
#define PIC_CONV_OUT
#endif

#ifndef PIC_CONV_INOUT
#define PIC_CONV_INOUT
#endif

typedef void* PIC_CONV_HANDLE_t;

/*
[Don't use for now]
Jetson support _ER: gamma corrected RGB input [0;255]
    Gamma correction or gamma is a nonlinear operation used to encode and decode
    luminance or tristimulus values in video or still image systems.
*/
typedef enum {
    PIC_FMT_YUV420 = 0,  // YYYY.....U....V...
    PIC_FMT_NV12,        // YYYY.....UV....
    PIC_FMT_NV21,        // YYYY.....VU....
    PIC_FMT_UYVY,        // UYVY.....
    PIC_FMT_YUYV,        // YUYV.....
    PIC_FMT_YVYU,        // YVYU.....
    PIC_FMT_YUV444,      // Y...U...V

    PIC_FMT_ABGR32,      // BGRA.....
    PIC_FMT_XRGB32,      // RGBX.....
    PIC_FMT_ARGB32,      // RGBA.....

    PIC_FMT_GRAY8,       // GRAY8.....

    PIC_FMT_JPEG,
    PIC_FMT_MAX
} PicFormat_t;

typedef struct _PicParam_t {
    PicFormat_t format;

    unsigned int width;
    unsigned int height;
} PicParam_t;

typedef struct _PicCropRect_t {
    int x; /*left*/
    int y; /*top*/
    int w; /*width*/
    int h; /*height*/
} PicCropRect_t;

typedef enum {
    PIC_FLIP_NONE         = 0, // Identity(no rotation)
    PIC_FLIP_90           = 1, // 90 degree counter-clockwise rotation
    PIC_FLIP_180          = 2, // 180 degree counter-clockwise rotation
    PIC_FLIP_270          = 3, // 270 degree counter-clockwise rotation
    PIC_FLIP_FlipX        = 4, // Horizontal flip
    PIC_FLIP_FlipY        = 5, // Vertical flip
    PIC_FLIP_Transpose    = 6, // Flip across upper left/lower right diagonal
    PIC_FLIP_InvTranspose = 7, // Flip across upper right/lower left diagonal

    PIC_FLIP_MAX
} PicFlip_t;

/*Interpolation method to use*/
typedef enum {
    PIC_INTERP_DEFAULT  = 0, // PIC_INTERP_BILINEAR
    PIC_INTERP_NEAREST  = 1, // nearest
    PIC_INTERP_BILINEAR = 2, // bilinear
    PIC_INTERP_5TAP     = 3, // 5-tap
    PIC_INTERP_10TAP    = 4, // 10-tap
    PIC_INTERP_SMART    = 5, // smart
    PIC_INTERP_NICEST   = 6, // nicest

    PIC_INTERP_MAX
} PicInterp_t;

typedef enum {
    PIC_DATA_TYPE_plain  = 0, // unsigned char *src_pic
    PIC_DATA_TYPE_ffmpeg = 1  // ffmpeg AVFrame*
} PicDataType_t;

typedef struct _PicSetting_t {
    PicParam_t    src;
    PicParam_t    dest;

    PicCropRect_t *cropping;  /*NULL: disabled*/
    PicFlip_t     flip;       /*default: NONE*/
    PicInterp_t   interp;     /*default: bilinear*/

    PicDataType_t pic_type;

    int           play_id;    /*1, 2, 3...*/
} PicSetting_t;

/*
* To generate the picture converter handler
*/
PIC_CONV_HANDLE_t PicConvInit(PIC_CONV_IN PicSetting_t *config);

/*
* To do the configured picture converter
*   void *src_pic:   the source picture data
*                      user space buffer: unsigned char *  OR  ffmpeg AVFrame*
*
*   void **dest_pic: the converted picture data
*                      Created in library and returned back to the caller
*
*   unsigned int  *dest_pic_sz: the size of the converted picture data
*/
int PicConvProc(PIC_CONV_IN  PIC_CONV_HANDLE_t handle,
                PIC_CONV_IN  void *src_pic,
                PIC_CONV_OUT void **dest_pic,
                PIC_CONV_OUT unsigned int *dest_pic_sz);

/*
    void *dest_pic: provided by outside app,
                    copy the converted picture to outside
*/
int PicConvProc_copy(PIC_CONV_IN    PIC_CONV_HANDLE_t handle,
                     PIC_CONV_IN    void *src_pic,
                     PIC_CONV_INOUT void *dest_pic,
                     PIC_CONV_OUT   unsigned int *dest_pic_sz);

/*
* To release the picture converter handler
*/
void PicConvRelease(PIC_CONV_IN PIC_CONV_HANDLE_t handle);

__END_DECLS

#endif /* __PIC_CONVERTER_H_ */