#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/time.h>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
}

#include "FeatureExtraction.h"
#include "define.h"
//#include "FeatureDetect.h"
//#include "FeatureDB.h"
#define MAX_FEATURE_FRAME 100000

void SaveFrame(AVFrame *pFrame, int width, int height, int video_id, char* videofile)
{
	FILE *pFile;
	char szFilename[32];
	int y,linesize;
	unsigned char* ptr;
	sprintf(szFilename, "%s_%dx%d_%d.yuv", videofile,width,height,video_id);
	pFile=fopen(szFilename, "w+b");
	if(pFile==NULL)
	{
		fprintf(stderr, "open target file error\n");
		return;
	}
	for(int mi=0;mi<3;mi++) {		
		ptr = pFrame->data[mi];
		linesize = pFrame->linesize[mi];
		if (mi == 1) {
			width >>= 1;
			height >>= 1;						 
		}
		for(int j=0;j<height;j++) {
			fwrite(ptr,1,width*sizeof(unsigned char),pFile);
			ptr += linesize;
		}
	}
	fclose(pFile);
}

typedef struct _yuvFrame
{
	unsigned char * yuvdata;
	struct _yuvFrame * next;
}yuvFrame;

typedef struct _yuvList
{
	yuvFrame * head;
	yuvFrame * tail;
}yuvList;

struct ThreadPara
{
	int width;
	int height;
	char * filename;
	yuvList * yuvlist;
};

void SaveIFrame(unsigned char *yuvdata, yuvList * yuvlist)
{
	yuvFrame * newnode = (yuvFrame *)malloc(sizeof(yuvFrame));
	newnode->yuvdata = yuvdata;
	newnode->next = NULL;
	if(yuvlist->head == NULL)
	{
		yuvlist->head = newnode;
		yuvlist->tail = newnode;
		//printf("Insert a Frame !\n");
		return;
	}
	yuvlist->tail->next = newnode;
	//printf("Insert a Frame\n");
}

static void video_encode_example(const char *filename)
{
	AVCodec *codec;
	AVCodecContext *c= NULL;
	int i, out_size, size, x, y, outbuf_size;
	FILE *f;
	AVFrame *picture;
	uint8_t *outbuf, *picture_buf;

	printf("Video encoding\n");

	/* find the mpeg1 video encoder */
	codec = avcodec_find_encoder(CODEC_ID_MPEG1VIDEO);
	if (!codec) {
		fprintf(stderr, "codec not found\n");
		exit(1);
	}

	c= avcodec_alloc_context();
	picture= avcodec_alloc_frame();

	/* put sample parameters */
	c->bit_rate = 400000;
	/* resolution must be a multiple of two */
	c->width = 352;
	c->height = 288;
	/* frames per second */
	c->time_base= (AVRational){1,25};
	c->gop_size = 10; /* emit one intra frame every ten frames */
	c->max_b_frames=1;
	c->pix_fmt = PIX_FMT_YUV420P;

	/* open it */
	if (avcodec_open(c, codec) < 0) {
		fprintf(stderr, "could not open codec\n");
		exit(1);
	}

	f = fopen(filename, "wb");
	if (!f) {
		fprintf(stderr, "could not open %s\n", filename);
		exit(1);
	}

	/* alloc image and output buffer */
	outbuf_size = 100000;
	outbuf = (uint8_t *)malloc(outbuf_size);
	size = c->width * c->height;
	picture_buf = (uint8_t *)malloc((size * 3) / 2); /* size for YUV 420 */

	picture->data[0] = picture_buf;
	picture->data[1] = picture->data[0] + size;
	picture->data[2] = picture->data[1] + size / 4;
	picture->linesize[0] = c->width;
	picture->linesize[1] = c->width / 2;
	picture->linesize[2] = c->width / 2;

	/* encode 1 second of video */
	for(i=0;i<25;i++) {
		fflush(stdout);
		/* prepare a dummy image */
		/* Y */
		for(y=0;y<c->height;y++) {
			for(x=0;x<c->width;x++) {
				picture->data[0][y * picture->linesize[0] + x] = x + y + i * 3;
			}
		}

		/* Cb and Cr */
		for(y=0;y<c->height/2;y++) {
			for(x=0;x<c->width/2;x++) {
				picture->data[1][y * picture->linesize[1] + x] = 128 + y + i * 2;
				picture->data[2][y * picture->linesize[2] + x] = 64 + x + i * 5;
			}
		}

		/* encode the image */
		out_size = avcodec_encode_video(c, outbuf, outbuf_size, picture);
		printf("encoding frame %3d (size=%5d)\n", i, out_size);
		fwrite(outbuf, 1, out_size, f);
	}

	/* get the delayed frames */
	for(; out_size; i++) {
		fflush(stdout);

		out_size = avcodec_encode_video(c, outbuf, outbuf_size, NULL);
		printf("write frame %3d (size=%5d)\n", i, out_size);
		fwrite(outbuf, 1, out_size, f);
	}

	/* add sequence end code to have a real mpeg file */
	outbuf[0] = 0x00;
	outbuf[1] = 0x00;
	outbuf[2] = 0x01;
	outbuf[3] = 0xb7;
	fwrite(outbuf, 1, 4, f);
	fclose(f);
	free(picture_buf);
	free(outbuf);

	avcodec_close(c);
	av_free(c);
	av_free(picture);
	printf("\n");
}

void *CreateAbstract(void * param)
{
	ThreadPara * tpara = (ThreadPara *)param;
	char *filename;
	filename = strdup(tpara->filename);
	int width = tpara->width;
	int height = tpara->height;
	yuvList * yuvlist = tpara->yuvlist;		 

	AVCodec *codec;
	AVCodecContext *c= NULL;
	int i, out_size, size, x, y, outbuf_size;
	FILE *f;
	AVFrame *picture;
	uint8_t *outbuf, *picture_buf;

	printf("Video encoding\n");

	/* find the mpeg1 video encoder */
	codec = avcodec_find_encoder(CODEC_ID_MPEG1VIDEO);
	//codec = avcodec_find_encoder(CODEC_ID_FLV1);
	if (!codec) {
		fprintf(stderr, "codec not found\n");
		exit(1);
	}

	c= avcodec_alloc_context();
	picture= avcodec_alloc_frame();

	/* put sample parameters */
	c->bit_rate = 400000;
	/* resolution must be a multiple of two */
	c->width = width;
	c->height = height;
	/* frames per second */
	c->time_base= (AVRational){1,25};
	c->gop_size = 10; /* emit one intra frame every ten frames */
	c->max_b_frames=0;
	c->pix_fmt = PIX_FMT_YUV420P;

	/* open it */
	if (avcodec_open(c, codec) < 0) {
		fprintf(stderr, "could not open codec\n");
		exit(1);
	}

	f = fopen(filename, "wb");
	if (!f) {
		fprintf(stderr, "could not open %s\n", filename);
		exit(1);
	}

	/* alloc image and output buffer */
	outbuf_size = 100000;
	outbuf = (uint8_t *)malloc(outbuf_size);

	size = c->width * c->height;

	picture->linesize[0] = c->width;
	picture->linesize[1] = c->width / 2;
	picture->linesize[2] = c->width / 2;

	i = 0;
	while(1) 
	{
		fflush(stdout);

		yuvFrame * yuvframe = NULL;

		if(yuvlist != NULL)
		{
			if(yuvlist->head != NULL && yuvlist->head != yuvlist->tail)
			{
				yuvframe = yuvlist->head;
				yuvlist->head = yuvlist->head->next;	
			}
			else if(yuvlist->head != NULL && yuvlist->head == yuvlist->tail)
			{
				yuvframe = yuvlist->head;
				yuvlist->head = yuvlist->tail = NULL;
			}
		}
		
		if(yuvframe == NULL)
		{
			//printf("continue\n");
			continue;
		}	

		if(yuvframe->yuvdata == NULL)
		{
			printf("break\n");
			break;
		}

		picture->data[0] = yuvframe->yuvdata;
		picture->data[1] = picture->data[0] + size;
		picture->data[2] = picture->data[1] + size / 4;

		/* encode the image */
		out_size = avcodec_encode_video(c, outbuf, outbuf_size, picture);
		printf("encoding frame %3d (size=%5d)\n", i++, out_size);
		fwrite(outbuf, 1, out_size, f);
		
		free(yuvframe->yuvdata);
		free(yuvframe);
	}
	
	/* get the delayed frames */
	for(; out_size; i++) {
		fflush(stdout);

		out_size = avcodec_encode_video(c, outbuf, outbuf_size, NULL);
		printf("write frame %3d (size=%5d)\n", i, out_size);
		fwrite(outbuf, 1, out_size, f);
	}

	/* add sequence end code to have a real mpeg file */
	outbuf[0] = 0x00;
	outbuf[1] = 0x00;
	outbuf[2] = 0x01;
	outbuf[3] = 0xb7;
	fwrite(outbuf, 1, 4, f);
	fclose(f);
	//free(picture_buf);
	free(outbuf);

	avcodec_close(c);
	av_free(c);
	av_free(picture);
	free(tpara);
	printf("\n");

}

yuvList * init_abs_create(int type, char * absfile, int width, int height)
{
	if(type == 0)
	{
		// Don't create video abstract thread
		return NULL;
	}
	else if(type == 1)
	{
		// Create video abstract thread
		pthread_t id;
		yuvList * yuvlist = NULL;
		yuvlist = (yuvList *)malloc(sizeof(yuvList));
		yuvlist->head = NULL;
		yuvlist->tail = NULL;
		printf("Create a empty yuvlist !\n");

		struct ThreadPara * param = (struct ThreadPara *)malloc(sizeof(struct ThreadPara));
		param->yuvlist = yuvlist; 
		param->width = width; 
		param->height = height; 
		param->filename = strdup(absfile); 

		int ret  = pthread_create(&id,NULL,CreateAbstract,(void*)param);
		if(ret !=0 )
		{
			printf("Create thread error!\n");
			exit(1);
		}

		return yuvlist;
	}
}

int finish_abs_create(yuvList * yuvlist)
{
	if(yuvlist != NULL)
	{
	}	
}

int getfeature(char* videofile,char* fealibpath, char * abspath, int id)
{
	// init decode
	static AVPacket packet;
	AVFormatContext *pFormatCtx;
	int i, videoStream;
	AVCodecContext *pCodecCtx;
	AVCodec *pCodec;
	AVFrame *pFrame;
	AVFrame *pFrameYUV;
	int numBytes;
	uint8_t *buffer;
	int frameFinished;
	av_register_all();
	pFrame=avcodec_alloc_frame();
	pFrameYUV=avcodec_alloc_frame();
	if(pFrameYUV==NULL){
		fprintf(stderr, "pFrameYUV==NULL\n");
		return -1;
	}

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

	//dump_format(pFormatCtx, 0, videofile, false);

	videoStream=-1;
	for(i = 0; i < pFormatCtx->nb_streams; i ++)
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
		fprintf(stderr, "pFrameYUV==NULL\n");
		return -1; // Could not open codec
	}

	
	int frame_no = 0;
	int video_id = -1;
	//pFrame->key_frame = 1;
	int width = pCodecCtx->width;
	int height = pCodecCtx->height;
		
	int I_Frame = 0;
	int B_Frame = 0;
	//gettimeofday(&tpstart,NULL);

	int ifabs = 0;
	if(abspath != NULL)
	{
		ifabs = 1;
	}

	char absfile[1024];
	memset(absfile,0,1024);
	sprintf(absfile,"%s/%d.flv",abspath,id);    

	yuvList * yuvlist = init_abs_create(ifabs, absfile, width, height);

		
	int matchFrameNum = 0;
	float* features = (float*)calloc(INDEX_FEATURE_DIM*MAX_FEATURE_FRAME,sizeof(float));
	while(av_read_frame(pFormatCtx,&packet)>=0)
	{
		if(packet.stream_index==videoStream)
		{
			/*
			if(packet.flags)
			{
				I_Frame ++;
			}
			else
			{
				B_Frame ++;
			}
			
			*/
			avcodec_decode_video(pCodecCtx, pFrame, &frameFinished,packet.data, packet.size);
			//avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished,&packet);
			if(frameFinished)
			{
				unsigned char* yuvdata = NULL;
				if(NULL == yuvdata)
					yuvdata = (unsigned char*)calloc(sizeof(unsigned char), width*height*3/2);

				int yuvwidth = width;
				int yuvheight = height;
				unsigned char* ptr = 0;
				int linesize = 0;
				int offset = 0;

				for(int mi=0;mi<3;mi++) {		
					ptr = pFrame->data[mi];
					linesize = pFrame->linesize[mi];
					if (mi == 1) {
						yuvwidth >>= 1;
						yuvheight >>= 1;						 
					}
					for(int j=0;j<yuvheight;j++) {								
						memcpy(yuvdata+offset,ptr,yuvwidth*sizeof(uint8_t));
						ptr += linesize;
						offset += yuvwidth;
					}	
				}
				
				//== 1.filter none match frames,
				//if (!FilterFrame(yuvdata,width, height))
				//{							
				//	skipframe ++;
				//	continue;
				//}				

				float* currFeatures = features+matchFrameNum*INDEX_FEATURE_DIM; //ALL_FEATURES_DIM;
				//== 2. fetch and write current frame's features into file
				get_feature(yuvdata,width, height,currFeatures);
				//== 3. filter similar frame

				//== 4.show picture
				if(packet.flags)
				{
					SaveIFrame(yuvdata, yuvlist);
				}

				matchFrameNum ++;
			}
		}
		
		av_free_packet(&packet);
	}
	

	SaveIFrame(NULL, yuvlist);

	//delete [] buffer;
	//av_free(pFrameYUV);

	if(matchFrameNum)
	{
		char smpfile[1024];
		memset(smpfile,0,1024);
		sprintf(smpfile,"%s/%d.smp",fealibpath,id);    
		FILE * pfFeature = fopen(smpfile, "wb");
		fwrite(&matchFrameNum,sizeof(int),1,pfFeature);
		int i = 0;
		for (i=0; i<matchFrameNum; i++)
		{
			fwrite(features+i*INDEX_FEATURE_DIM,sizeof(float),INDEX_FEATURE_DIM,pfFeature);
		}
		fclose(pfFeature);
		printf("Process %d frame\n",matchFrameNum);
		printf("Save feafile to %s\n",smpfile);
		printf("=============================\n");
	}			
	else
	{

	}

	free(features);
	avcodec_close(pCodecCtx);
	av_close_input_file(pFormatCtx);
	av_free(pFrame);

	finish_abs_create(yuvlist);
	return 0;
}

