
//#include <vector>
//#include <string>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
//#include <windows.h>

#include "FeatureExtraction.h"
#define VIDEOSTEP 10000

float avg_y_min = 25;
float avg_y_max = 230;
double entropy_min = 1.0;//minum entropy

void InitialFilter(char * cfgfile)
{
	FILE* pfCfg = fopen(cfgfile,"r");
	if (NULL!=pfCfg)
	{
		fscanf(pfCfg,"%d\n%d\n%d",&avg_y_min,&avg_y_max,&entropy_min);
		fclose(pfCfg);
	}
	return;
}

#define nBins 16
#define MYLOG(x) (((x)>1e-4)? log(x) : 0 )

float getEntropy(float *pdf){
	float H=0;
	int i;
	for(i=0;i<nBins;i++){
		if(pdf[i]>1e-4)
		H+=-1*pdf[i]*log(pdf[i]);
	}
	return H;
}
void hist2pdf(float *hist,float *pdf){
	float sum=0;
	int i;
	for(i=0;i<nBins;i++){
		sum+=hist[i];
	}
	if(sum<1){
		for(i=0;i<nBins;i++){
			pdf[i]=0;
		}
	}else{
		for(i=0;i<nBins;i++){
			pdf[i]=hist[i]/sum;
		}
	}
}
//统计圆心在（x，y）的半径从radios1到radios2的圆环中的元素构成的直方图。
double calEntropy(unsigned char *pY,int width,int height){
	float hist[nBins];
	float pdf[nBins];
	int div=(256/nBins);	//quantisation of image.
	int i;
	for(i=0;i<nBins;i++){
		hist[i]=0;
	}
	for(i=0;i<width;i++){
		int j;
		for(j=0;j<height;j++){
			hist[pY[i+j*width]/div]++;
			
		}
	}
	hist2pdf(hist,pdf);
	double entropy;
	entropy=getEntropy(pdf);
	return entropy;
}
//== return 1 when frame can be set as match frame,otherwise return 0
int FilterFrame(unsigned char * yuvdata, int width, int height)
{
	//== average of Y
	float avg = 0;
	int j=0;
	for(j=0;j<width*height; j++)
	{
		avg += yuvdata[j];
	}
	avg = avg/(height*width);
	//== variance of Y
	float std = 0;
	for(j=0; j<width*height; j++)
	{
		std += (yuvdata[j]-avg)*(yuvdata[j]-avg);
	}
	std = sqrt(std/(width*height));
	//printf("avgY = %d ,stdY = %d\n",avg, std);
	//== entropy
	double entropy = calEntropy(yuvdata,width,height);
	//if(avg>60 && avg<225 && std<=65)
	//if(avg>60 && avg<225)
	//{
	//	return 1;
	//}
	if(avg>avg_y_min && avg<avg_y_max&&entropy>entropy_min)
	{
		return 1;
	}
	return 0;
}
