
#include "FeatureExtraction.h"

const float PI = 3.1415926F;

/**
  the function can generate the patchs of a yuv frame and the format patchs as follow...
  ____________________________
  |        |        |        |
  |        |        |        |
  |    5   |   3    |   6    |
  |      	 |        |        |
  |________|________|________|
  |        |        |        |
  |        |        |        |
  |    1   |   0    |   2    |
  |        |        |        |
  |________|________|________|
  |        |        |        |
  |        |        |        |
  |    7   |   4    |   8    |
  |        |        |        |
  |________|________|________|

 **/
#define STEP_SIZE		2
#define	CHECK_POINT	5
#define TH_LUMA			30

//== this function generates the features of patches in current frame
int get_feature(unsigned char* data, int nw, int nh, float* features)
{
	//== 1 remove static edgeframe
	int yValue=0;
	int lineNum=0;
	char bContinue = 1;
	int top = 0, bottom = nh-1,left = 0, right = nw-1;
	unsigned char * pPoint = NULL;
	int widthStep = (nw)/(CHECK_POINT-1)-1;
	int heightStep = (nh)/(CHECK_POINT-1)-1;
	//== 1.1 top	
	for(lineNum=0;bContinue && lineNum<nh/4;lineNum+=STEP_SIZE)
	{
		yValue = 0;
		int k;
		for (k=0;k<CHECK_POINT;k++)
		{
			pPoint = data + lineNum*nw + widthStep*k;
			yValue += pPoint[0] + pPoint[nw] + pPoint[2*nw]; 						

			if ((yValue/(3*(k+1)))>TH_LUMA)
			{
				bContinue = 0;				
				break;
			}
		}		
		top = lineNum;
	};
	//== 1.2 bottom	
	for(bContinue=1,lineNum=nh-1;bContinue && lineNum>3*nh/4; lineNum-=STEP_SIZE) 
	{
		yValue = 0;
		int k;
		for (k=0;k<CHECK_POINT;k++)
		{
			pPoint = data + lineNum*nw + widthStep*k;
			yValue += pPoint[0] + pPoint[-nw] + pPoint[-2*nw]; 						

			if (yValue/(3*(k+1))>TH_LUMA)
			{
				bContinue = 0;				
				break;
			}
		}		
		bottom = lineNum;
	} 
	//== 1.3 left	
	for(bContinue=1,lineNum=0;bContinue && lineNum<nw/4; lineNum+=STEP_SIZE)
	{
		yValue = 0;
		int k;
		for (k=0;k<CHECK_POINT;k++)
		{
			pPoint = data + lineNum + heightStep*k*nw;
			yValue += pPoint[0] + pPoint[1] + pPoint[2]; 						

			if (yValue/(3*(k+1))>TH_LUMA)
			{
				bContinue = 0;				
				break;
			}
		}
		left = lineNum;
	}
	//== 1.4 right	
	for(bContinue=1,lineNum=nw-1;bContinue && lineNum>3*nw/4;lineNum-=STEP_SIZE) 
	{
		yValue = 0;
		int k;
		for (k=0;k<CHECK_POINT;k++)
		{
			pPoint = data + lineNum + heightStep*k*nw;
			yValue += pPoint[0] + pPoint[-1] + pPoint[-2]; 						

			if (yValue/(3*(k+1))>TH_LUMA)
			{
				bContinue = 0;				
				break;
			}
		}
		right = lineNum;				
	}
	//== 2 calculate all subblocks' feature
	int block_offset[PATCH_NUM];
	int block_w = (right-left+1)/PATCH_NUM_W;
	int block_h = (bottom-top+1)/PATCH_NUM_H;
	int block_size = block_w * block_h;
	block_offset[0] = (block_w+left) + nw*(block_h+top);
	block_offset[1] = left + nw*(block_h+top);
	block_offset[2] = (block_w*2+left) + nw*(block_h+top);
	block_offset[3] = block_w+left + nw*top;
	block_offset[4] = block_w+left + nw*(block_h*2+top);
	block_offset[5] = left + nw*top;
	block_offset[6] = block_w*2+left + nw*top;
	block_offset[7] = left + nw*(block_h*2+top);	
	float avg = 0.; 
	float var = 0.; 
	//==2.1 for subblock from 0 to 5, index features
	int block_idx;
	for (block_idx=0; block_idx<5; block_idx++)
	{
		//== 2.1.1 Calculate avg
		unsigned char* blockdata = data + block_offset[block_idx]; 
		avg = 0.;
		var = 0.;
		int y;
		for (y = 0; y < block_h; y++)
		{
			int x;
			for (x = 0; x < block_w; x++)
			{
				avg += blockdata[x + y * nw];				
			}
		}
		avg = avg / block_size;				
		if (block_idx>2)
		{
			features[3 + block_idx] = avg;
			continue;
		}
		features[block_idx*2] = avg;
		//== 2.1.2 calculate var
		for (y = 0; y < block_h; y++)
		{
			int x;
			for (x = 0; x < block_w; x++)
			{
				var += (blockdata[x + y * nw]-avg)*(blockdata[x + y * nw]-avg);				
			}
		}		
		features[block_idx*2+1] = sqrt(var/ block_size);
	}
	/*
	float* matchFeatures = features + INDEX_FEATURE_DIM;
	memset(matchFeatures,0,PATCH_NUM*MATCH_FEATURE_DIM*sizeof(float));
	//== 2.2 for subblock from 0 to 7, match features
	for (int block_idx=0; block_idx<PATCH_NUM; block_idx++)
	{
		unsigned char* blockdata = data + block_offset[block_idx]; 		
		for (int y = 0; y < block_h; y++)
		{
			for (int x = 0; x < block_w; x++)
			{
				matchFeatures[blockdata[x + y * nw]/MATCH_FEATURE_STEP] += (1./block_size);				
			}
		}		
		matchFeatures += MATCH_FEATURE_DIM;
	}
	*/
	return 0;
}
//== this function generates the features of patches in current picture

//线性插值法对图像重采样，改变图像大小
void resizeFrame(unsigned char * pSrc, int srcWidth, int srcHeight, unsigned char *pDest, int destWidth, int destHeight)
{
	int sw = srcWidth - 1, sh = srcHeight - 1, dw = destWidth - 1, dh = destHeight - 1;   
	int B, N, x, y;   
	const unsigned char *pA, *pB, *pC, *pD;	//指向原图像左上、右上、左下、右下像素灰度值
	int i;
	for ( i = 0; i <= dh; i++ )   
	{   
		y = i * sh / dh;   //原图像y坐标,scale=dh/sh y=i/scale; 
		N = dh - i * sh % dh;   
		int j;
		for ( j = 0; j <= dw; j++ )   
		{   
			x = j * sw / dw;
			B = dw - j * sw % dw;   

			pA=pSrc+srcWidth*y+x;
			pB=pSrc+srcWidth*y+x+1;

			if (N!=dh)  //防止越界
			{
				pC=pSrc+srcWidth*(y+1)+x;
				pD=pSrc+srcWidth*(y+1)+x+1;
			} 
			else
			{
				pC=pA;
				pD=pB;
			}

			if ( B == dw )   //防止越界
			{   
				pB = pA;   
				pD = pC;   
			}   

			//重采样
			pDest[i*destWidth+j]=( unsigned char )( int )( ( B * N * ( *pA - *pB - *pC + *pD ) + dw * N * *pB   
						+ dh * B * *pC + ( dw * dh - dh * B - dw * N ) * *pD + dw * dh / 2 ) / ( dw * dh ) );
		}   
	}   
}
