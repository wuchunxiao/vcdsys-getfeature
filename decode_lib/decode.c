#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/time.h>
#include <pthread.h>
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"

#include "decode.h"

int decode_callback(char* videofile, CallBackF * callbackf, int useid)
{
	// init decode
	static AVPacket packet;
	AVFormatContext *pFormatCtx;
	int i, videoStream;
	AVCodecContext *pCodecCtx;
	AVCodec *pCodec;
	AVFrame *pFrame;
	int numBytes;
	uint8_t *buffer;
	int frameFinished;
	
	av_register_all();
	pFrame=avcodec_alloc_frame();

	struct	timeval tpstart,tpend;
	double	timeuse;

	//open video file
	
	if(av_open_input_file(&pFormatCtx, videofile, NULL, 0, NULL) < 0){
		fprintf(stderr, "Couldn't Open video file\n");
		return -1; // Couldn't open file
	}
	if (av_find_stream_info(pFormatCtx) < 0){
		fprintf(stderr, "av_find_stream_info error\n");
	}
	if( (pFormatCtx)<0){
		fprintf(stderr, "Retrieve stream information error\n");
		return -1; // Couldn't find stream information
	}

	dump_format(pFormatCtx, 0, videofile, 0);
	videoStream=-1;
	for(i = 0; i < pFormatCtx->nb_streams; i++)
	{
		if(pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_VIDEO)
		{
			videoStream=i;
			break;
		}
	}

	if(videoStream==-1)
	{
		fprintf(stderr, "Didn't find a video stream and pFormatCtx->nb_streams is %d\n",pFormatCtx->nb_streams);
		return -1; // Didn't find a video stream
	}

	//open codec
	pCodecCtx=pFormatCtx->streams[videoStream]->codec;
	pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
	if(pCodec==NULL){
		fprintf(stderr, "Codec not found\n");
		return -1; // Codec not found
	}
	if(pCodec->capabilities & CODEC_CAP_TRUNCATED)
		pCodecCtx->flags|=CODEC_FLAG_TRUNCATED;
	if(avcodec_open(pCodecCtx, pCodec)<0){
		fprintf(stderr, "Could not open codec\n");
		return -1; // Could not open codec
	}
	
	int width = pCodecCtx->width;
	int height = pCodecCtx->height;
	
	while(av_read_frame(pFormatCtx,&packet)>=0)
	{
		if(packet.stream_index==videoStream)
		{
			avcodec_decode_video(pCodecCtx, pFrame, &frameFinished,packet.data, packet.size);
			if(frameFinished)
			{
				unsigned char* yuvdata = NULL;
				yuvdata = (unsigned char*)malloc(sizeof(unsigned char)*width*height*3/2);
				if(yuvdata == NULL)
				{
					break;
				}
				int yuvwidth = width;
				int yuvheight = height;
				unsigned char* ptr = 0;
				int linesize = 0;
				int offset = 0;
				int mi;
				for(mi=0;mi<3;mi++) {		
					ptr = pFrame->data[mi];
					linesize = pFrame->linesize[mi];
					if (mi == 1) {
						yuvwidth >>= 1;
						yuvheight >>= 1;						 
					}
					int j;
					for(j=0;j<yuvheight;j++) {								
						memcpy(yuvdata+offset,ptr,yuvwidth*sizeof(uint8_t));
						ptr += linesize;
						offset += yuvwidth;
					}	
				}
				if(callbackf->sendFrame != NULL)
				{
					int ret = callbackf->sendFrame(yuvdata, width, height, useid);
					if(ret == 1)
					{
						return 1;
					}
				}
				
				free(yuvdata);
				
			}
		}
		
		av_free_packet(&packet);
	}

	avcodec_close(pCodecCtx);
	av_close_input_file(pFormatCtx);
	av_free(pFrame);
	return 0;
}

