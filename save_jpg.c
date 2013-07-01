#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jpeglib.h"

#define max(a,b)            (((a) > (b)) ? (a) : (b))
#define min(a,b)            (((a) < (b)) ? (a) : (b))

long mU[256], mV[256], mY1[256], mY2[256];

void prepareTran()
{
	long i;

	for (i=0; i<256; i++)
	{

		mV[i] = 15938*i-2221300;
		mU[i] = 20238*i-2771300;
		mY1[i] = 11644*i;
		mY2[i] = 19837*i-311710;
	}
}

int YUV4202RGB(int width,int height,unsigned char *pYUVBuf,unsigned char** pRGBBuf)
{
	//pRGBBuf = new unsigned char[width*height*3];
	*pRGBBuf = (unsigned char*)malloc(width*height*3);
	if(*pRGBBuf == NULL)
		return -1;

	unsigned char *pY, *pU, *pV, *pUbase, *pVbase;
	int i, j;
	unsigned char *pR, *pG, *pB;
	int RGB_SIZE=width*height*3;
	if (width % 2)
	{
		width -= 1;
	}

	pY = pYUVBuf;
	pU = pUbase = pYUVBuf + width*height;
	pV = pVbase = pUbase + width*height/4;

	for (i=0; i<height; i++)
	{
		pR = *pRGBBuf+3*width*i;
		pG = *pRGBBuf+3*width*i+1;
		pB = *pRGBBuf+3*width*i+2;

		for (j=0; j<width; j+=2)
		{

			*pR = max (0, min (255, (mV[*pV] + mY1[*pY])/10000) );   //R value
			*pB = max (0, min (255, (mU[*pU] + mY1[*pY])/10000) );   //B value
			*pG = max (0, min (255, (mY2[*pY] - 5094*(*pR) - 1942*(*pB))/10000) ); //G value

			pR += 3;
			pB += 3;
			pG += 3;
			pY++;

			*pR = max (0, min (255, (mV[*pV] + mY1[*pY])/10000) );   //R value
			*pB = max (0, min (255, (mU[*pU] + mY1[*pY])/10000) );   //B value
			*pG = max (0, min (255, (mY2[*pY] - 5094*(*pR) - 1942*(*pB))/10000) ); //G value

			pR += 3;
			pB += 3;
			pG += 3;
			pY++;

			pU++;
			pV++;
		}
		if ((i % 2 == 0))
		{
			pU = pU-(width >> 1);
			pV = pV-(width >> 1);
		}
	}
	return 0;
}


int YUV420Resize2RGB(unsigned char *pYUVBuf,int src_width,int src_height,unsigned char** pRGBBuf,int dst_width,int dst_height)
{
	//pRGBBuf = new unsigned char[dst_width*dst_height*3];
	(*pRGBBuf) = (unsigned char*)malloc(dst_width*dst_height*3);
	if((*pRGBBuf) == NULL)
		return -1;

	int sw = src_width - 1, sh = src_height- 1, dw = dst_width - 1, dh = dst_height - 1;   
	int BY, NY, xY, yY;
	int BU, NU, xU, yU;
	int BV, NV, xV, yV;
	unsigned char* pY = pYUVBuf;
	unsigned char* pU = pYUVBuf + src_width*src_height;
	unsigned char* pV = pU + src_width*src_height/4;
	unsigned char Y,U,V;
	unsigned char *pR,*pG,*pB;

	const unsigned char *pYA, *pYB, *pYC, *pYD;	//指向原图像Y左上、右上、左下、右下像素灰度值
	const unsigned char *pUA, *pUB, *pUC, *pUD;	//指向原图像U左上、右上、左下、右下像素灰度值
	const unsigned char *pVA, *pVB, *pVC, *pVD;	//指向原图像V左上、右上、左下、右下像素灰度值
	int i = 0;
	for ( i = 0; i <= dh; i++ )   
	{   
		pR = (*pRGBBuf)+3*dst_width*i;
		pG = (*pRGBBuf)+3*dst_width*i+1;
		pB = (*pRGBBuf)+3*dst_width*i+2;

		yY = i * sh / dh;		  //原图像y坐标,scale=dh/sh y=i/scale; 
		NY = dh - i * sh % dh;  

		yU = (i/2)*sh/dh;
		NU = dh/2 - (i/2)*(sh/2)%(dh/2);

		yV = (i/2)*sh/dh;
		NV = dh/2 - (i/2)*(sh/2)%(dh/2);
		int j=0;
		for (j = 0; j <= dw; j++ )   
		{   
			xY = j * sw / dw;
			BY = dw - j * sw % dw;

			xU = (j/2)*sw/dw;
			BU = (dw/2) - (j/2)*(sw/2)%(dw/2);
			xV = (j/2)*sw/dw;
			BV = (dw/2) - (j/2)*(sw/2)%(dw/2);

			pYA = pY+src_width*yY+xY;
			pYB = pY+src_width*yY+xY+1;
			pUA = pU+src_width*yU/2+xU;
			pUB = pU+src_width*yU/2+xU+1;
			pVA = pV+src_width*yV/2+xV;
			pVB = pV+src_width*yV/2+xV+1;

			if (NY!=dh)  //防止越界
			{
				pYC = pY+src_width*(yY+1)+xY;
				pYD = pY+src_width*(yY+1)+xY+1;
			}
			else
			{
				pYC = pYA;
				pYD = pYB;
			}
			if(NU !=(dh/2))
			{
				pUC = pU+src_width*(yU+1)/2+xU;
				pUD = pU+src_width*(yU+1)/2+xU+1;
			}
			else
			{
				pUC = pUA;
				pUD = pUB;
			}
			if(NV !=(dh/2))
			{
				pVC = pV+src_width*(yV+1)/2+xV;
				pVD = pV+src_width*(yV+1)/2+xV+1;
			} 
			else
			{
				pVC = pVA;
				pVD = pVB;
			}

			if ( BY == dw )   //防止越界
			{   
				pYB = pYA;   
				pYD = pYC;
			}
			if( BU == (dw/2))
			{
				pUB = pUA;
				pUD = pUC;
			}
			if(BV == (dw/2))
			{
				pVB = pVA;
				pVD = pVC;
			}   

			//重采样
			Y = (unsigned char)(int)((BY*NY*(*pYA-*pYB-*pYC+*pYD)+dw*NY*(*pYB)+dh*BY*(*pYC)+(dw*dh-dh*BY-dw*NY)*(*pYD)+dw*dh/2)/(dw*dh));
			if(j%2 == 0)
			{
				U = (unsigned char)(int)((BU*NU*(*pUA-*pUB-*pUC+*pUD)+(dw/2)*NU*(*pUB)+(dh/2)*BU*(*pUC)+(dw*dh/4-dh*BU/2-dw*NU/2)*(*pUD)+dw*dh/8)/(dw*dh/4));
				V = (unsigned char)(int)((BV*NV*(*pVA-*pVB-*pVC+*pVD)+dw*NV*(*pVB)/2+dh*BV*(*pVC)/2+(dw*dh/4-dh*BV/2-dw*NV/2)*(*pVD)+dw*dh/8)/(dw*dh/4));
			}

			*pR = max(0,min(255,(mV[V]+mY1[Y])/10000));   //R value
			*pB = max(0,min(255,(mU[U]+mY1[Y])/10000));   //B value
			*pG = max(0,min(255,(mY2[Y]-5094*(*pR)-1942*(*pB))/10000)); //G value

			pR += 3;
			pB += 3;
			pG += 3;
		}   
	}   
	return 0;
}



int RGB2JPG(char* filename,unsigned char* rgbData,int quality,int image_width,int image_height)
{
	if (rgbData==NULL)
		return -1;
	if (image_width==0)
		return -1;
	if (image_height==0)
		return -1;

	struct jpeg_compress_struct cinfo;
	
	FILE * outfile=NULL;
	int row_stride;

	struct jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	if ((outfile = fopen(filename, "wb+")) == NULL) 
	{
		printf("Can not open file:%s",filename);
		return -1;
	}

	jpeg_stdio_dest(&cinfo, outfile);

	cinfo.image_width = image_width; 
	cinfo.image_height = image_height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB; 

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, TRUE);

	jpeg_start_compress(&cinfo, TRUE);

	row_stride = image_width * 3;

	while (cinfo.next_scanline < cinfo.image_height) 
	{
		unsigned char* outRow;
		outRow = rgbData + (cinfo.next_scanline * image_width * 3);
		(void) jpeg_write_scanlines(&cinfo, &outRow, 1);
	}

	jpeg_finish_compress(&cinfo);

	fclose(outfile);

	jpeg_destroy_compress(&cinfo);

	return 0;
}

int YUV4202JPG(char* filename, unsigned char* yuvData,int quality,int image_width,int image_height)
{	
	if(yuvData == NULL)
		return -1;
	unsigned char* rgbData = NULL;
	YUV4202RGB(image_width,image_height,yuvData,&rgbData);
	RGB2JPG(filename,rgbData,quality,image_width,image_height);
	//delete [] rgbData;
	free(rgbData);
	rgbData = NULL;
	return 0;
}

int YUV420Resize2JPG(char* filename, unsigned char* yuvData,int src_width,int src_height,int quality,int dst_width,int dst_height)
{
	if(yuvData == NULL)
		return -1;
	unsigned char* rgbData = NULL;
	YUV420Resize2RGB(yuvData,src_width,src_height,&rgbData,dst_width,dst_height);
	RGB2JPG(filename,rgbData,quality,dst_width,dst_height);
	//delete [] rgbData;
	free(rgbData);
	rgbData = NULL;
	return 0;
}

