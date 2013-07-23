#ifndef		FEATURE_EXTRACTION_H
#define		FEATURE_EXTRACTION_H

#include <stdlib.h>
//#include <string>
#include <math.h>
//#include <limits>
#include "define.h"

int get_feature(unsigned char* data,int nw,int nh,float* features);
int get_pic_feature(unsigned char *data, int nw, int nh, float *features, float *feature);

int get_vlad_feature(unsigned char* data, int nw, int nh, float *centroids, float* features);
int ExtractVladFeatures(unsigned char* data, int nw, int nh, float *centroids, float* vlad_features);
int ReadFvecs(char *filename,int *vector_dim,float *vectors);
void FindNearestNeighbors(float *v,int nv,float *q,int nq,int dim,int idx[],float dis[]);
int GetSiftFeatures(unsigned char *data, int nw, int nh, float *features, int *features_num);
void Norm2(int nv,float *v);

int get_pca_vlad_feature(unsigned char *data,int nw,int nh,float *centroids,float *mean,float *pca_proj,int pca_dim,float *features);
int ReadPCAProj(char *filename,float *mean,float *pca_proj);


#endif


