
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "libvl/generic.h"
#include "libvl/sift.h"
#include "define.h"
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
// #define INFINITE 1000000

int get_vlad_feature(unsigned char* data, int nw, int nh, float *centroids, float* features)
{
	// printf("here 0\n");
	float *vlad_features = (float *)malloc(SIFT_DESCRIPTOR_DIM * VLAD_CENTROIDS_NUM *sizeof(float));
	if(vlad_features == NULL)
	{
		printf("Memory overflow error:can't apply features memory!\n");
		return -1;
	}
	memset(vlad_features,0,SIFT_DESCRIPTOR_DIM * VLAD_CENTROIDS_NUM *sizeof(float));
	// printf("here 1\n");
	ExtractVladFeatures(data,nw,nh,centroids,vlad_features);
	// printf("here 2\n");
	memcpy(features,vlad_features,INDEX_FEATURE_DIM * sizeof(float));	
	free(vlad_features);
	return 0;
}

//*****centroids and pca_proj_matrix files shuold be readed only once!!!***************8
// input:
//	pca_dim, 降到多少维
int get_pca_vlad_feature(unsigned char *data,int nw,int nh,float *centroids,float *mean,float *pca_proj,int pca_dim,float *features)
{
	float *vlad_features = (float *)malloc(SIFT_DESCRIPTOR_DIM * VLAD_CENTROIDS_NUM *sizeof(float));
	if(vlad_features == NULL)
	{
		printf("Memory overflow error:can't apply vlad features memory!\n");
		return -1;
	}
	memset(vlad_features,0,SIFT_DESCRIPTOR_DIM * VLAD_CENTROIDS_NUM *sizeof(float));
	ExtractVladFeatures(data,nw,nh,centroids,vlad_features);
	//printf("here 1\n");

	int vlad_feature_dim = SIFT_DESCRIPTOR_DIM * VLAD_CENTROIDS_NUM;
	// char *f_pca_proj = "data/pca_proj_matrix_vladk64_flickr1Mstar.fvecs";
	// float *mean = (float *)malloc(vlad_feature_dim * sizeof(float));
	// float *pca_proj = (float *)malloc(vlad_feature_dim * 1024 *sizeof(float));	// only the 1024 eigenvectors are stored
	// ReadPCAProj(f_pca_proj,mean,pca_proj);
	// printf("here 2\n");
	
	float *pca_vlad_features = (float *)malloc(pca_dim * sizeof(float));
	int i = 0;
	for(i = 0;i < vlad_feature_dim;++i)
	{
		vlad_features[i] -= mean[i];
	}
	int j = 0;
	for(i = 0;i < pca_dim;++i)
	{
		float sum = 0;
		for(j = 0;j < vlad_feature_dim;++j)	
		{
			sum += pca_proj[i * vlad_feature_dim + j] * vlad_features[j];
		}
		pca_vlad_features[i] = sum;
	}
	//printf("here 3\n");
	Norm2(pca_dim,pca_vlad_features);
	memcpy(features,pca_vlad_features,pca_dim * sizeof(float));
	// free(mean);
	// free(pca_proj);
	free(vlad_features);
	free(pca_vlad_features);
	return 0;
}

int ExtractVladFeatures(unsigned char* data, int nw, int nh, float *centroids, float* vlad_features)
{
	//printf("here 0\n");
	FILE * f_fea_time = fopen("./result/fea_time.txt","w");
	struct timeval start,end;
	int time_used;
	float *sift_features = (float *)malloc(SIFT_DESCRIPTOR_DIM * MAX_SIFT_FEATURE_NUM * sizeof(float));	// 没初始化
	if(sift_features == NULL)
	{
		printf("Memory overflow error:can't apply sift features memory!\n");
		return -1;
	}
	gettimeofday(&start,NULL);
	int sift_features_num = 0;
	GetSiftFeatures(data, nw, nh, sift_features,&sift_features_num);
	printf("sift_num = %d \n",sift_features_num);
	gettimeofday(&end,NULL);
	time_used = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
	time_used /= 1000;
    printf("Get SIFT Feature Time = %d ms\n",time_used);
    fprintf(f_fea_time,"Get SIFT Feature Time = %d ms\n",time_used);
	//printf("here 1\n");

	// char *centroids_file = "data/clust_k64.fvecs";
	// int centroids_dim = 0;
	// float *centroids = (float *)malloc(SIFT_DESCRIPTOR_DIM * VLAD_CENTROIDS_NUM * sizeof(float));
	// ReadFvecs(centroids_file,&centroids_dim,centroids);
	// int cn = 0;
	// for(cn = 0;cn < SIFT_DESCRIPTOR_DIM * VLAD_CENTROIDS_NUM;cn += SIFT_DESCRIPTOR_DIM)
	// {
	// 	Norm2(SIFT_DESCRIPTOR_DIM,centroids + cn);
	// }
	// printf("here 2\n");
	gettimeofday(&start,NULL);
	int *idx = (int *)malloc(sift_features_num * sizeof(int));
	float *dis = (float *)malloc(sift_features_num * sizeof(float));
	memset(idx,0,sift_features_num * sizeof(int));
	memset(dis,0.0,sift_features_num * sizeof(float));
	FindNearestNeighbors(centroids,VLAD_CENTROIDS_NUM,sift_features,sift_features_num,SIFT_DESCRIPTOR_DIM,idx,dis);
	// printf("here 3\n");
	
	// float *vlad_features = (float *)malloc(SIFT_DESCRIPTOR_DIM * VLAD_CENTROIDS_NUM *sizeof(float));
	// memset(vlad_features,0,SIFT_DESCRIPTOR_DIM * VLAD_CENTROIDS_NUM *sizeof(float));
	int i = 0,j = 0;
	for(i = 0;i < sift_features_num;++i)
	{
		for(j = 0;j < SIFT_DESCRIPTOR_DIM;++j)
		{
			vlad_features[idx[i] *  SIFT_DESCRIPTOR_DIM + j] += sift_features[i * SIFT_DESCRIPTOR_DIM + j] -
																centroids[idx[i] * SIFT_DESCRIPTOR_DIM + j];
		}
	}
	// printf("here 4.1\n");
	//exit(1);
	// L2-normalized
	Norm2(SIFT_DESCRIPTOR_DIM * VLAD_CENTROIDS_NUM,vlad_features);	
	gettimeofday(&end,NULL);
	time_used = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
	time_used /= 1000;
    printf("SIFT Feature Convert to VLAD Time = %d ms\n",time_used);
    fprintf(f_fea_time,"SIFT Feature Convert to VLAD Time = %d ms\n\n",time_used);
	fclose(f_fea_time);
	// printf("here 4.2\n");
	
	//memcpy(features,vlad_features,INDEX_FEATURE_DIM * sizeof(float));	
	//printf("here 5");
	// fcolse(centroids_file);
	free(idx);
	free(dis);
	// free(centroids);
	free(sift_features);
	// free(vlad_features);
	return 0;
}

int GetSiftFeatures(unsigned char *data, int nw, int nh, float *features, int *features_num)
{
	vl_sift_pix *img_data = (vl_sift_pix *)malloc(sizeof(vl_sift_pix) * nw * nh);
	if(img_data == NULL)
	{
		printf("Memory overflow error:can't apply image data memory!\n");
		return -1;
	}
	unsigned char *pixel;
	int i = 0, j = 0;
	for(i = 0;i < nh;++i)
	{
		for(j = 0;j < nw;++j)
		{
			pixel = (unsigned char *)(data + i * nw + j);
			img_data[i * nw + j] = *pixel;
		}
	}
	int noctaves = 4, nlevels = 3, o_min = -1;
	VlSiftFilt *sift_filt = NULL;
	sift_filt =  vl_sift_new(nw, nh, noctaves, nlevels, o_min);
	int num_descriptors = 0;
	// int save_flag = -1;
	if(vl_sift_process_first_octave(sift_filt,img_data) != VL_ERR_EOF)
	{
		while(1)
		{
			vl_sift_detect(sift_filt);
			VlSiftKeypoint *sift_keypoints = sift_filt->keys; 
			for(i = 0; i < sift_filt->nkeys;++i)
			{
				VlSiftKeypoint temp_sift_keypoint = *sift_keypoints;
				double angles[4];
				int angle_count = vl_sift_calc_keypoint_orientations(sift_filt,angles,&temp_sift_keypoint);
				for(j = 0; j < angle_count;++j)
				{
					double temp_angle = angles[j];
					float *descriptor = (float *)malloc(sizeof(float) * SIFT_DESCRIPTOR_DIM);
					vl_sift_calc_keypoint_descriptor(sift_filt, descriptor, &temp_sift_keypoint, temp_angle);
					float *curr_features = features + num_descriptors * SIFT_DESCRIPTOR_DIM;
					// if(save_flag == -1)
					// {
						memcpy(curr_features,descriptor,sizeof(float) * SIFT_DESCRIPTOR_DIM);
						//save_flag == 1;
					// }
					++num_descriptors;
					free(descriptor);
				}
				++sift_keypoints;
			}
			if(vl_sift_process_next_octave(sift_filt) == VL_ERR_EOF)
			{
				break;
			}
		}
	}
	*features_num = num_descriptors;
	vl_sift_delete(sift_filt);
	free(img_data);
	return 0;
}

// 此处采用线性查找的方法,因为VLAD聚类中心一般个数不多,此处为64个.
// 如果想要提高速度,可用VLFEAT里的kd-tree方法,根据FLANN编写的,或者融yael库
// input
// 	v	the dataset to be searched
//	nv	the num of dataset
// 	q	the set of queries
//	nq	...
//	dim	the feature dim
// output
// 	idx	the vector index of the nearest neighbor
// 	dis	the corresponding distance	
void FindNearestNeighbors(float *v,int nv,float *q,int nq,int dim,int idx[],float dis[])
{
	// int nq = sizeof(q) / dim;
	// int nv = sizeof(v) / dim;
	int i = 0, j = 0,k = 0;
	for(i = 0;i < nq;++i)	// query
	{
		// dis[i] = INFINITE;
		for(j = 0;j < nv;++j)	// dataset
		{
			float distance = 0.0;
			for(k = 0;k < dim;++k)
			{
				distance += q[i*dim+k] * q[i*dim+k] + v[j*dim+k] * v[j*dim+k] - 2 * q[i*dim+k] * v[j*dim+k];
				// distance += (q[i*dim+k] - v[j*dim+k]) * (q[i*dim+k] - v[j*dim+k]);	// 跟上面的一样
			}
			// printf("dis = %f\n",distance);
			if(j == 0 || distance < dis[i])
			{
				dis[i] = distance;
				idx[i] = j;
			}
		}
	}
}

// Read a set of vectors stored in the fvec format(int + n * float)
// The function return a set of output vector
int ReadFvecs(char *filename,int *vector_dim,float *vectors)
{
	FILE *file = fopen(filename,"rb");
	if(file == NULL)
	{
		printf("I/O error: Unable to open the file %s\n", filename);
		return -1;
	}
	// read the dim of vector
	int d = 0;
	fread(&d, sizeof(int), 1 ,file);
	*vector_dim = d;
	
	int vector_num = 0;
	fseek(file,0,SEEK_SET);	
	while(!feof(file))
	{
		fseek(file,4,SEEK_CUR);	
		float *cur_vector = vectors + vector_num * d;
		fread(cur_vector,sizeof(float),d,file);
		++vector_num;
	}
	fclose(file);
	return 0;
	/*
	int vecsizeof =  1 * 4 + d * 4;
	
	// get the number of vectors
	fseek(file,0,SEEK_END);
	int bmax = ftell(file);
	if(bmax == 0)
	{
		vectors = NULL;
		return 0;
	}
	bmax = floor(bmax / vecsizeof);
	if(bmax == 0)
	{
		vectors = NULL;
		return 0;
	}
	int b = bmax;
	
	fseek(file,0,SEEK_SET);
	int i = 0;
	for(i = 0;i < b;++i)
	{
		fseek(file,4,SEEK_CUR);
		float *curr_vector = vectors + i * d;
		fread(curr_vectors,sizeof(float),d);		
	}
	// fread(vectors,sizeof(float),(d + 1) * b);
	fclose(file);*/
	
}

// This function reads pca project matrix and mean of the vectors
// input
//	filename ...
// output
// 	mean, the mean of the vectors,Note that VLAD are already almost centered
//	pca_proj, the project matrix
int ReadPCAProj(char *filename,float *mean,float *pca_proj)
{
	FILE *file = fopen(filename,"rb");
	if(file == NULL)
	{
		printf("I/O error: Unable to open the file %s\n", filename);
		return -1;
	}
	int d = 0;
	fread(&d, sizeof(int), 1 ,file);
	fread(mean,sizeof(float),d,file);
	fread(&d,sizeof(int), 1,file);
	fread(pca_proj,sizeof(float),d,file);
	fclose(file);
	return 0;
}

void Norm2(int nv,float *v)
{
	int i = 0;
	float sum = 0.0;
	for(i = 0;i < nv;++i)
	{
		sum += v[i] * v[i];
	}
	sum = sqrt(sum);
	if(sum == 0)
	{
		for(i = 0;i < nv;++i)
			v[i] = 1.0;
	}
	else
	{
		for(i = 0;i < nv;++i)
			v[i] /= sum;
	}
}

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
