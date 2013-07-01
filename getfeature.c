#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/time.h>
#include <pthread.h>
//#include <windows.h>
#define __STDC_CONSTANT_MACROS
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "encoder.h"

#include "FeatureExtraction.h"
#include "FilterFrame.h"
#include "Reduce.h"
#include "define.h"
#include "getfeature.h"
#include "save_jpg.h"
#include <sys/stat.h>
#include <sys/types.h>

#define MAX_FEATURE_FRAME 1000000
#define SEND_FEATURE_FRAME 4096

pthread_mutex_t yuvlist_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t avcodec_mutex = PTHREAD_MUTEX_INITIALIZER;

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
	int mi;
	for(mi=0;mi<3;mi++) {		
		ptr = pFrame->data[mi];
		linesize = pFrame->linesize[mi];
		if (mi == 1) {
			width >>= 1;
			height >>= 1;						 
		}
		int j;
		for(j=0;j<height;j++) {
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
	int count;
}yuvList;

typedef struct _ThreadPara
{
	int width;
	int height;
	char * filename;
	yuvList * yuvlist;
}ThreadPara;

void SaveIFrame(unsigned char *yuvdata, yuvList * yuvlist)
{
	yuvFrame * newnode = (yuvFrame *)malloc(sizeof(yuvFrame));
	newnode->yuvdata = yuvdata;
	newnode->next = NULL;
	pthread_mutex_lock(&yuvlist_mutex);
	if(yuvlist->head == NULL)
	{
		yuvlist->head = newnode;
		yuvlist->tail = newnode;
		yuvlist->count ++;
		//printf("Insert a Frame !\n");
		pthread_mutex_unlock(&yuvlist_mutex);
		return;
	}
	yuvlist->tail->next = newnode;
	yuvlist->tail = newnode;
	yuvlist->count++;
	pthread_mutex_unlock(&yuvlist_mutex);
	//printf("Insert a Frame\n");
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
		printf("codec not found\n");
		return NULL;
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
		printf("could not open codec\n");
		return NULL;
	}

	f = fopen(filename, "wb");
	if (!f) {
		printf("could not open %s\n", filename);
		return NULL;
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

		pthread_mutex_lock(&yuvlist_mutex);
		if(yuvlist != NULL)
		{
			//if(yuvlist->head != NULL && yuvlist->head != yuvlist->tail)
			if(yuvlist->head != NULL)
			{
				yuvframe = yuvlist->head;
				yuvlist->count--;
				printf("The remain frame is %d\n",yuvlist->count);
				yuvlist->head = yuvlist->head->next;	
			}
			//else if(yuvlist->head != NULL && yuvlist->head == yuvlist->tail)
			//{
			//	yuvframe = yuvlist->head;
			//	yuvlist->head = yuvlist->tail = NULL;
			//}
		}
		pthread_mutex_unlock(&yuvlist_mutex);

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

void *CreateAbstract_ALL(void * param)
{
	ThreadPara * tpara = (ThreadPara *)param;
	char *filename;
	filename = strdup(tpara->filename);
	int width = tpara->width;
	int height = tpara->height;
	yuvList * yuvlist = tpara->yuvlist;		 

	AVOutputFormat *fmt;
	AVFormatContext *oc;
	AVStream *audio_st, *video_st;
	double audio_pts, video_pts;
	int i;

	/* initialize libavcodec, and register all codecs and formats */
	av_register_all();

	/* auto detect the output format from the name. default is
	   mpeg. */
	fmt = guess_format(NULL, filename, NULL);
	
	if (!fmt) {
		printf("Could not deduce output format from file extension: using MPEG.\n");
		fmt = guess_format("mpeg", NULL, NULL);
	}
	if (!fmt) {
		printf("Could not find suitable output format\n");
		return NULL;
	}

	/* allocate the output media context */
	oc = avformat_alloc_context();
	if (!oc) {
		printf("Memory error\n");
		return NULL;
	}
	oc->oformat = fmt;
	snprintf(oc->filename, sizeof(oc->filename), "%s", filename);

	/* add the audio and video streams using the default format codecs
	   and initialize the codecs */
	video_st = NULL;
	audio_st = NULL;
	if (fmt->video_codec != CODEC_ID_NONE) {
		video_st = add_video_stream(oc, fmt->video_codec, width, height);
	}
	//if (fmt->audio_codec != CODEC_ID_NONE) {
	//	audio_st = add_audio_stream(oc, fmt->audio_codec);
	//}

	/* set the output parameters (must be done even if no
	   parameters). */
	if (av_set_parameters(oc, NULL) < 0) {
		printf("Invalid output format parameters\n");
		return NULL;
	}

	dump_format(oc, 0, filename, 1);

	/* now that all the parameters are set, we can open the audio and
	   video codecs and allocate the necessary encode buffers */
	if (video_st)
		open_video(oc, video_st);
	//if (audio_st)
	//	open_audio(oc, audio_st);

	/* open the output file, if needed */
	if (!(fmt->flags & AVFMT_NOFILE)) {
		if (url_fopen(&oc->pb, filename, URL_WRONLY) < 0) {
			printf("Could not open '%s'\n", filename);
			return NULL;
		}
	}

	/* write the stream header, if any */
	av_write_header(oc);

	while(1) {
		/* compute current audio and video time */
		//if (audio_st)
		//	audio_pts = (double)audio_st->pts.val * audio_st->time_base.num / audio_st->time_base.den;
		//else
		//	audio_pts = 0.0;

		yuvFrame * yuvframe = NULL;

		pthread_mutex_lock(&yuvlist_mutex);
		if(yuvlist != NULL)
		{
			if(yuvlist->head != NULL && yuvlist->head != yuvlist->tail)
			{
				yuvframe = yuvlist->head;
				yuvlist->head = yuvlist->head->next;	
				yuvlist->count--;
				//printf("The remain frame is %d\n",yuvlist->count);
			}
			else if(yuvlist->head != NULL && yuvlist->head == yuvlist->tail)
			{
				yuvframe = yuvlist->head;
				yuvlist->head = yuvlist->tail = NULL;
				yuvlist->count--;
				//printf("The remain frame is %d\n",yuvlist->count);

			}
		}
		pthread_mutex_unlock(&yuvlist_mutex);

		if(yuvframe == NULL)
		{
			//printf("continue\n");
			sleep(1);
			continue;
		}	

		if(yuvframe->yuvdata == NULL)
		{
			printf("break\n");
			free(yuvframe);
			break;
		}

		if (video_st)
			video_pts = (double)video_st->pts.val * video_st->time_base.num / video_st->time_base.den;
		else
			video_pts = 0.0;

		//if ((!audio_st || audio_pts >= STREAM_DURATION) &&
		//		(!video_st || video_pts >= STREAM_DURATION))
		
		//if(!video_st || video_pts >= STREAM_DURATION)
		//	break;

		/* write interleaved audio and video frames */
		//if (!video_st || (video_st && audio_st && audio_pts < video_pts)) {
		//	write_audio_frame(oc, audio_st);
		//} else {
		//printf("Before write video frame oc = %x, video_st = %x, yuvframe->yuvdata = %x\n",oc,video_st,yuvframe->yuvdata);
		write_video_frame(oc, video_st, yuvframe->yuvdata);
		//printf("After write video frame\n");
		//}
		free(yuvframe->yuvdata);
		free(yuvframe);

	}

	/* write the trailer, if any.  the trailer must be written
	 * before you close the CodecContexts open when you wrote the
	 * header; otherwise write_trailer may try to use memory that
	 * was freed on av_codec_close() */
	av_write_trailer(oc);

	/* close each codec */
	if (video_st)
		close_video(oc, video_st);
	//if (audio_st)
	//	close_audio(oc, audio_st);

	/* free the streams */
	for(i = 0; i < oc->nb_streams; i++) {
		av_freep(&oc->streams[i]->codec);
		av_freep(&oc->streams[i]);
	}

	if (!(fmt->flags & AVFMT_NOFILE)) {
		/* close the output file */
		url_fclose(oc->pb);
	}

	/* free the stream */
	av_free(oc);
	free(filename);
	free(tpara->filename);
	free(tpara);
}

yuvList * init_abs_create(int type, char * absfile, int width, int height, pthread_t *id)
{
	if(type == 0)
	{
		// Don't create video abstract thread
		return NULL;
	}
	else if(type == 1)
	{
		// Create video abstract thread
		//pthread_t id;
		//printf("init_abs_create begin !\n");
		yuvList * yuvlist = NULL;
		yuvlist = (yuvList *)malloc(sizeof(yuvList));
		yuvlist->head = NULL;
		yuvlist->tail = NULL;
		yuvlist->count = 0;
		//printf("Create a empty yuvlist !\n");

		ThreadPara * param = (ThreadPara *)malloc(sizeof(ThreadPara));
		param->yuvlist = yuvlist; 
		param->width = width; 
		param->height = height; 
		param->filename = strdup(absfile); 

		int ret  = pthread_create(id,NULL,CreateAbstract_ALL,(void*)param);
		//int ret  = pthread_create(id,NULL,CreateAbstract,(void*)param);
		if(ret !=0 )
		{
			printf("Create thread error!\n");
			free(yuvlist);
			return NULL;
		}

		return yuvlist;
	}
}

int finish_abs_create(yuvList * yuvlist, pthread_t id)
{
	pthread_join(id,NULL);
	printf("Finish abs create\n");
	if(yuvlist != NULL)
	{
		free(yuvlist);
		yuvlist = NULL;
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

	//printf("here 0\n");
	dump_format(pFormatCtx, 0, videofile, 0);
	//printf("here 1\n");
	videoStream=-1;
	for(i = 0; i < pFormatCtx->nb_streams; i++)
	{
		if(pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_VIDEO)
		{
			videoStream=i;
			break;
		}
	}

	//printf("here 2\n");
	if(videoStream==-1)
	{
		fprintf(stderr, "Didn't find a video stream and pFormatCtx->nb_streams is %d\n",pFormatCtx->nb_streams);
		return -1; // Didn't find a video stream
	}

	//printf("here 3\n");
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

	//printf("here 4\n");
	
	int frame_no = 0;
	int video_id = -1;
	//pFrame->key_frame = 1;
	int width = pCodecCtx->width;
	int height = pCodecCtx->height;
		
	int I_Frame = 0;
	int B_Frame = 0;
	//gettimeofday(&tpstart,NULL);

	int ifabs = 0;
	pthread_t pid;
	yuvList * yuvlist = NULL; 
	if(strcmp(abspath,"NULL"))
	{
		ifabs = 1;
	}

	if(ifabs)
	{

		char absfile[1024];
		memset(absfile,0,1024);

		if(id != -1)
		{
			sprintf(absfile,"%s\\%d.flv",abspath,id);    
		}
		else
		{
			//printf("here 4.1 %s\n",videofile);
			char* p = strrchr(videofile,'\\');
			//printf("here 4.2 %s\n",p);
			char* sub = strrchr(p+1,'.');
			//printf("here 4.3\n");
			char filename[1024];
			//printf("here 4.4\n");
			memset(filename,0,1024);
			//printf("here 4.5\n");
			strncpy(filename,p+1,strlen(p)-1-strlen(sub));
			//printf("here 4.6\n");
			strcat(filename,".flv");
			//printf("here 4.7\n");
			sprintf(absfile,"%s\%s",abspath,filename);
			//printf("here 4.8\n");
			//printf("absfile is %s\n",absfile);
		}    

		yuvlist = init_abs_create(ifabs, absfile, width, height, &pid);
	}
		
	//printf("here 5\n");

	int matchFrameNum = 0;
	float* features = (float*)malloc(INDEX_FEATURE_DIM*MAX_FEATURE_FRAME*sizeof(float));
	if(features == NULL)
	{
		return -1;
	}
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
			//printf("Before decode\n");
			avcodec_decode_video(pCodecCtx, pFrame, &frameFinished,packet.data, packet.size);
			//printf("After decode\n");
			//avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished,&packet);
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
				
				//== 1.filter none match frames,
				if (!FilterFrame(yuvdata,width, height))
				{							
					//skipframe ++;
					free(yuvdata);
					continue;
				}				

				float* currFeatures = features+matchFrameNum*INDEX_FEATURE_DIM; //ALL_FEATURES_DIM;
				//== 2. fetch and write current frame's features into file
				get_feature(yuvdata,width,height,currFeatures);
				//== 3. filter similar frame

				//== 4.show picture
				if(packet.flags)
				{
					if(ifabs)
					{
						SaveIFrame(yuvdata, yuvlist);
					}
					else
					{
						free(yuvdata);
					}
				}
				else
				{
					free(yuvdata);
				}

				matchFrameNum ++;
				if(matchFrameNum > MAX_FEATURE_FRAME)
				{
					printf("Reach Max Frames!\n");
					break;
				}
			}
		}
		
		av_free_packet(&packet);
	}
	
	if(ifabs)
	{
		SaveIFrame(NULL, yuvlist);
	}

	//delete [] buffer;
	av_free(pFrameYUV);

	// Reduce the features
	matchFrameNum = FeatureRefine(matchFrameNum,features);
	//printf("after reduce!\n");

	if(matchFrameNum)
	{
		char smpfile[1024];
		memset(smpfile,0,1024);
		if(id != -1)
		{
			sprintf(smpfile,"%s\%d.smp",fealibpath,id);
		}
		else
		{
			char* p = strrchr(videofile,'\\');
			char* sub = strrchr(p+1,'.');
			char filename[1024];
			memset(filename,0,1024);
			strncpy(filename,p+1,strlen(p)-1-strlen(sub));
			strcat(filename,".smp");
			sprintf(smpfile,"%s\%s",fealibpath,filename);
		}    

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
	if(ifabs)
	{
		finish_abs_create(yuvlist,pid);
	}
	return 0;
}

int getfeature_jpg(char* videofile,char* fealibpath, char * abspath, int id, long long *duration, int jpgnum)
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
	*duration = 0;
	*duration = pFormatCtx->duration/1000000;
	
	//printf("here 0\n");
	dump_format(pFormatCtx, 0, videofile, 0);
	//printf("here 1\n");
	videoStream=-1;
	int fps = 0;
	int nb_frames = 0;
	for(i = 0; i < pFormatCtx->nb_streams; i++)
	{
		if(pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_VIDEO)
		{
			videoStream=i;
			fps = pFormatCtx->streams[i]->r_frame_rate.num/ (double)pFormatCtx->streams[i]->r_frame_rate.den;
			//if(fps == 0)
			//	fps =  pFormatCtx->streams[i]->avg_frame_rate;
			//if(*duration == 0)
			//	*duration = pFormatCtx->streams[i]->duration/1000000;
			break;
		}
	}

	

	//printf("here 2\n");
	if(videoStream==-1)
	{
		fprintf(stderr, "Didn't find a video stream and pFormatCtx->nb_streams is %d\n",pFormatCtx->nb_streams);
		return -1; // Didn't find a video stream
	}

	//printf("here 3\n");
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

	int step = 0;
	if(fps == 0)
		fps = pCodecCtx->time_base.num/pCodecCtx->time_base.den;
	if(fps == 0 || fps > 30)
		fps = 25;
	if(*duration == 0 || *duration == AV_NOPTS_VALUE)
	{
		step = 100;
		*duration = 0;
	}
	else
		step = ((*duration) * fps) / jpgnum;
	nb_frames = pCodecCtx->frame_number;
	printf("================================fps is %d\n", fps);
	printf("================================nb_frames is %d\n", nb_frames);
	printf("================================step is %d\n", step);
	//printf("here 4\n");
	
	int frame_no = 0;
	int video_id = -1;
	//pFrame->key_frame = 1;
	int width = pCodecCtx->width;
	int height = pCodecCtx->height;
		
	int I_Frame = 0;
	int B_Frame = 0;
	//gettimeofday(&tpstart,NULL);

	int ifabs = 0;
	pthread_t pid;
	yuvList * yuvlist = NULL; 
	if(strcmp(abspath,"NULL"))
	{
		ifabs = 1;
	}

	char saveViewPath[1024];

	if(ifabs)   // abspath不为NULL,存储摘要视频
	{

		char absfile[1024];
		memset(absfile,0,1024);
    	
		if(id != -1)    // start_id != -1时, 对视频名称按照id(1,2,...)重新命名,进行存储
		{
			sprintf(absfile,"%s/%d.flv",abspath,id);  
		      	sprintf(saveViewPath , "%s/%d/" , abspath, id);
			mkdir(saveViewPath,S_IRWXU);	
		}
		else            // start_id == -1时,按照视频的原名进行存储
		{
			//printf("here 4.1 %s\n",videofile);
			char* p = strrchr(videofile,'/');
			//printf("here 4.2 %s\n",p);
			char* sub = strrchr(p+1,'.');
			//if(strcmp(sub,".M2TS")!=0)
			//{
			//	exit(0);
			//}		
			//printf("here 4.3\n");
			char filename[1024];
			//printf("here 4.4\n");
			memset(filename,0,1024);
			//printf("here 4.5\n");
			strncpy(filename,p+1,strlen(p)-1-strlen(sub));
			//sprintf(saveViewPath , "%s/%s/" , abspath, filename);
			sprintf(saveViewPath , "%s/%s" , abspath, filename);
			mkdir(saveViewPath,S_IRWXU);
			//printf("here 4.6\n");
			strcat(filename,".flv");
			//printf("here 4.7\n");
			sprintf(absfile,"%s/%s",abspath,filename);
			//printf("here 4.8\n");
			//printf("absfile is %s\n",absfile);
			
		}    

	}
		
	//printf("here 5\n");

	int matchFrameNum = 0;
	int framecount = 0;
	int vkfcount = 0;
	float* features = (float*)malloc(INDEX_FEATURE_DIM*MAX_FEATURE_FRAME*sizeof(float));
	if(features == NULL)
	{
		return -1;
	}

	prepareTran();
	
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
			//printf("Before decode\n");
			avcodec_decode_video(pCodecCtx, pFrame, &frameFinished,packet.data, packet.size);
			//printf("After decode\n");
			//avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished,&packet);
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
				
				//== 1.filter none match frames,
				if (!FilterFrame(yuvdata,width, height))
				{							
					//skipframe ++;
					free(yuvdata);
					continue;
				}				

				float* currFeatures = features+matchFrameNum*INDEX_FEATURE_DIM; //ALL_FEATURES_DIM;
				//== 2. fetch and write current frame's features into file
				get_feature(yuvdata,width,height,currFeatures);
				//== 3. filter similar frame
				//if(vkfcount == 0 || ++framecount == step && ifabs )
				if(++framecount == 5)   // 每隔5帧存一张图片
				{
					vkfcount ++;
					char jpgname[1024];
					if(id != -1)
					{
						sprintf(jpgname,"%s_%d.jpg",saveViewPath,vkfcount);
					}
					else
					{
						char* p = strrchr(videofile,'/');
						char* sub = strrchr(p+1,'.');
						char filename[1024];
						memset(filename,0,1024);
						strncpy(filename,p+1,strlen(p)-1-strlen(sub));
						sprintf(jpgname,"%s_%d.jpg",saveViewPath,vkfcount);
					}   
					printf("Create %d jpg\n", vkfcount);
					YUV420Resize2JPG(jpgname, yuvdata, width, height, 100, width, height);
					framecount = 0;
				}
				free(yuvdata);

				matchFrameNum ++;
				if(matchFrameNum > MAX_FEATURE_FRAME)
				{
					printf("Reach Max Frames!\n");
					break;
				}
			}
		}
		
		av_free_packet(&packet);
	}
	
	//delete [] buffer;
	av_free(pFrameYUV);

	// Reduce the features
	matchFrameNum = FeatureRefine(matchFrameNum,features);
	//printf("after reduce!\n");

	if(matchFrameNum)
	{
		char smpfile[1024];
		memset(smpfile,0,1024);
		if(id != -1)
		{
			sprintf(smpfile,"%s\%d.smp",fealibpath,id);
		}
		else
		{
			char* p = strrchr(videofile,'/');
			char* sub = strrchr(p+1,'.');
			char filename[1024];
			memset(filename,0,1024);
			strncpy(filename,p+1,strlen(p)-1-strlen(sub));
			strcat(filename,".smp");
			sprintf(smpfile,"%s/%s",fealibpath,filename);
		}    

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

	return 0;
}


int getfeature_for_display(char* videofile,char* fealibpath, char * abspath, CallBackF * callbackf, int id, int useid)
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

	//printf("here 0\n");
	dump_format(pFormatCtx, 0, videofile, 0);
	//printf("here 1\n");
	videoStream=-1;
	for(i = 0; i < pFormatCtx->nb_streams; i++)
	{
		if(pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_VIDEO)
		{
			videoStream=i;
			break;
		}
	}

	//printf("here 2\n");
	if(videoStream==-1)
	{
		fprintf(stderr, "Didn't find a video stream and pFormatCtx->nb_streams is %d\n",pFormatCtx->nb_streams);
		return -1; // Didn't find a video stream
	}

	//printf("here 3\n");
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

	//printf("here 4\n");
	
	int frame_no = 0;
	int video_id = -1;
	//pFrame->key_frame = 1;
	int width = pCodecCtx->width;
	int height = pCodecCtx->height;
		
	int I_Frame = 0;
	int B_Frame = 0;
	//gettimeofday(&tpstart,NULL);

	int ifabs = 0;
	pthread_t pid;
	yuvList * yuvlist = NULL; 
	if(strcmp(abspath,"NULL"))
	{
		ifabs = 1;
	}

	if(ifabs)
	{

		char absfile[1024];
		memset(absfile,0,1024);

		if(id != -1)
		{
			sprintf(absfile,"%s\\%d.flv",abspath,id);    
		}
		else
		{
			//printf("here 4.1 %s\n",videofile);
			char* p = strrchr(videofile,'\\');
			//printf("here 4.2 %s\n",p);
			char* sub = strrchr(p+1,'.');
			//printf("here 4.3\n");
			char filename[1024];
			//printf("here 4.4\n");
			memset(filename,0,1024);
			//printf("here 4.5\n");
			strncpy(filename,p+1,strlen(p)-1-strlen(sub));
			//printf("here 4.6\n");
			strcat(filename,".flv");
			//printf("here 4.7\n");
			sprintf(absfile,"%s\%s",abspath,filename);
			//printf("here 4.8\n");
			//printf("absfile is %s\n",absfile);
		}    

		yuvlist = init_abs_create(ifabs, absfile, width, height, &pid);
	}
		
	//printf("here 5\n");

	int matchFrameNum = 0;
	float* features = (float*)malloc(INDEX_FEATURE_DIM*MAX_FEATURE_FRAME*sizeof(float));
	if(features == NULL)
	{
		return -1;
	}
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
			//printf("Before decode\n");
			avcodec_decode_video(pCodecCtx, pFrame, &frameFinished,packet.data, packet.size);
			//printf("After decode\n");
			//avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished,&packet);
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
				//== 1.filter none match frames,
				if (!FilterFrame(yuvdata,width, height))
				{							
					//skipframe ++;
					free(yuvdata);
					continue;
				}				

				float* currFeatures = features+matchFrameNum*INDEX_FEATURE_DIM; //ALL_FEATURES_DIM;
				//== 2. fetch and write current frame's features into file
				get_feature(yuvdata,width,height,currFeatures);
				//== 3. filter similar frame

				//== 4.show picture
				if(packet.flags)
				{
					if(ifabs)
					{
						SaveIFrame(yuvdata, yuvlist);
					}
					else
					{
						free(yuvdata);
					}
				}
				else
				{
					free(yuvdata);
				}

				matchFrameNum ++;
				if(matchFrameNum > MAX_FEATURE_FRAME)
				{
					printf("Reach Max Frames!\n");
					break;
				}
			}
		}
		
		av_free_packet(&packet);
	}
	
	if(ifabs)
	{
		SaveIFrame(NULL, yuvlist);
	}

	//delete [] buffer;
	av_free(pFrameYUV);

	// Reduce the features
	matchFrameNum = FeatureRefine(matchFrameNum,features);
	//printf("after reduce!\n");

	if(matchFrameNum)
	{
		char smpfile[1024];
		memset(smpfile,0,1024);
		if(id != -1)
		{
			sprintf(smpfile,"%s\%d.smp",fealibpath,id);
		}
		else
		{
			char* p = strrchr(videofile,'\\');
			char* sub = strrchr(p+1,'.');
			char filename[1024];
			memset(filename,0,1024);
			strncpy(filename,p+1,strlen(p)-1-strlen(sub));
			strcat(filename,".smp");
			sprintf(smpfile,"%s\%s",fealibpath,filename);
		}    

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
	if(ifabs)
	{
		finish_abs_create(yuvlist,pid);
	}
	return 0;
}

int getfeature_callback(char* videofile, CallBackF * callbackf, int useid)
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
	
	pthread_mutex_lock(&avcodec_mutex);
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

	//printf("here 0\n");
	dump_format(pFormatCtx, 0, videofile, 0);
	//printf("here 1\n");
	videoStream=-1;
	for(i = 0; i < pFormatCtx->nb_streams; i++)
	{
		if(pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_VIDEO)
		{
			videoStream=i;
			break;
		}
	}

	//printf("here 2\n");
	if(videoStream==-1)
	{
		fprintf(stderr, "Didn't find a video stream and pFormatCtx->nb_streams is %d\n",pFormatCtx->nb_streams);
		return -1; // Didn't find a video stream
	}

	//printf("here 3\n");
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
	pthread_mutex_unlock(&avcodec_mutex);
	//printf("here 4\n");
	
	int frame_no = 0;
	int video_id = -1;
	//pFrame->key_frame = 1;
	int width = pCodecCtx->width;
	int height = pCodecCtx->height;
		
	int I_Frame = 0;
	int B_Frame = 0;
	//gettimeofday(&tpstart,NULL);

	int matchFrameNum = 0;
	float* features = (float*)malloc(INDEX_FEATURE_DIM*SEND_FEATURE_FRAME*sizeof(float));
	if(features == NULL)
	{
		return -1;
	}
	while(av_read_frame(pFormatCtx,&packet)>=0)
	{
		if(packet.stream_index==videoStream)
		{
			//printf("Before decode\n");
			avcodec_decode_video(pCodecCtx, pFrame, &frameFinished,packet.data, packet.size);
			//printf("After decode\n");
			//avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished,&packet);
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
				//== 1.filter none match frames,
				if (!FilterFrame(yuvdata,width, height))
				{							
					//skipframe ++;
					free(yuvdata);
					continue;
				}				
				
				float* currFeatures = features+matchFrameNum*INDEX_FEATURE_DIM; //ALL_FEATURES_DIM;
				//== 2. fetch and write current frame's features into file
				get_feature(yuvdata,width,height,currFeatures);
				//== 3. filter similar frame
				free(yuvdata);
				matchFrameNum ++;
				if(matchFrameNum == SEND_FEATURE_FRAME)
				{
					// Reduce the features
					matchFrameNum = FeatureRefine(matchFrameNum,features);
					if(callbackf->sendSmp != NULL)
					{
						int ret = callbackf->sendSmp((unsigned char*)features,matchFrameNum,0,useid);
						if(ret == 1)
						{
							return 1;
						}
					}
					matchFrameNum = 0;
				}
			}
		}
		
		av_free_packet(&packet);
	}

	if(matchFrameNum > 0)
	{
		// Reduce the features
		matchFrameNum = FeatureRefine(matchFrameNum,features);
		if(callbackf->sendSmp != NULL)
		{
			int ret = callbackf->sendSmp((unsigned char*)features,matchFrameNum,1,useid);
			if(ret == 1)
			{
				return 1;
			}
		}
		matchFrameNum = 0;
	}

	//delete [] buffer;
	av_free(pFrameYUV);

	free(features);
	avcodec_close(pCodecCtx);
	av_close_input_file(pFormatCtx);
	av_free(pFrame);
	return 0;
}

