#ifndef		FEATURE_EXTRACTION_H
#define		FEATURE_EXTRACTION_H

#include <stdlib.h>
//#include <string>
#include <math.h>
//#include <limits>
#include "define.h"

int get_feature(unsigned char* data,int nw,int nh,float* features);
int get_vlad_feature(unsigned char* data, int nw, int nh, float* features);
int get_pic_feature(unsigned char *data, int nw, int nh, float *features, float *feature);

#endif


