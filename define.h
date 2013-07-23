#ifndef DEFINE_H
#define DEFINE_H

#define MAX_PATH_LEN	1024
#define PATCH_NUM_W		3
#define PATCH_NUM_H		3
#define PATCH_NUM		(PATCH_NUM_W*PATCH_NUM_H-1)

//== video sample detect
#define INDEX_FEATURE_DIM	128 
#define MATCH_FEATURE_DIM	32
#define	MATCH_FEATURE_STEP	(256/MATCH_FEATURE_DIM)
#define ALL_FEATURES_DIM	(INDEX_FEATURE_DIM+PATCH_NUM*MATCH_FEATURE_DIM)

//== picture sample detect
#define PIC_INDEX_FEA_DIM	10
#define PIC_BLOCK_FEA_DIM	8
#define PIC_PATCH_NUM		(PATCH_NUM_W*PATCH_NUM_H)
#define PIC_MATCH_FEA_DIM	(PATCH_NUM_W*PATCH_NUM_H*PIC_BLOCK_FEA_DIM)

//== vlad features
#define MAX_SIFT_FEATURE_NUM 10000
#define SIFT_DESCRIPTOR_DIM 128
#define VLAD_CENTROIDS_NUM 64

#endif

