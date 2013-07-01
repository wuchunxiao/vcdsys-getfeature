/*
 * Libavformat API example: Output a media file in any supported
 * libavformat format. The default codecs are used.
 *
 * Copyright (c) 2003 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "libavformat/avformat.h"
#include "libswscale/swscale.h"

/* 5 seconds stream duration */
#define STREAM_DURATION   5.0
#define STREAM_FRAME_RATE 25 /* 25 images/s */
#define STREAM_NB_FRAMES  ((int)(STREAM_DURATION * STREAM_FRAME_RATE))
#define STREAM_PIX_FMT PIX_FMT_YUV420P /* default pix_fmt */

/*
 * add an audio output stream
 */
extern AVStream *add_audio_stream(AVFormatContext *oc, int codec_id);

extern void open_audio(AVFormatContext *oc, AVStream *st);
extern void get_audio_frame(int16_t *samples, int frame_size, int nb_channels);
extern void write_audio_frame(AVFormatContext *oc, AVStream *st);
extern void close_audio(AVFormatContext *oc, AVStream *st);

/**************************************************************/
/* video output */

/* add a video output stream */
extern AVStream *add_video_stream(AVFormatContext *oc, int codec_id, int width, int height);

extern AVFrame *alloc_picture(int pix_fmt, int width, int height);

extern void open_video(AVFormatContext *oc, AVStream *st);
/* prepare a dummy image */
extern void fill_yuv_image(AVFrame *pict, uint8_t *yuvdata, int width, int height);

extern void write_video_frame(AVFormatContext *oc, AVStream *st, uint8_t *yuvdata);

extern void close_video(AVFormatContext *oc, AVStream *st);
