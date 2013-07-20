#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include "getfeature.h"
// #include "./libvl/generic.h"
int main(int argc, char **argv)
{
	DIR* videodb;   
	struct dirent *p;   
	FILE *pFile;
	int feaid = atoi(argv[4]);	
	char videofile[1024];
	
	if(argc < 5)
	{
		printf("Para Error!\n");
        // 1:videopath, 视频数据存放路径 2:smppath, 特征存放路径; 3:absvideopath, 摘要视频存放路径; 4:startno, -1不对视频名称进行重新编号
		printf("Use: getfea videopath smppath absvideopath startno\n"); 
	}
	
	videodb=opendir(argv[1]);   

	if(videodb == NULL)
	{
		printf("Can not open videodir %s\n",argv[1]);
		return -1;
	}

	long long duration = 0;
	int jpgnum = 20;
	// VL_PRINT("vlfeat test\n");
	while ((p=readdir(videodb)))   
	{   
		if((strcmp(p->d_name,".")==0)||(strcmp(p->d_name,"..")==0))   
			continue;   
		else  
		{   
			memset(videofile,0,1024);
			sprintf(videofile,"%s\%s",argv[1],p->d_name);    
			// while(1)
			{
				printf("Getting %s's Feature\n",videofile);
				if(feaid != -1)
				{
					getfeature_jpg(videofile, argv[2],argv[3],feaid++, &duration, jpgnum);
					//getfeature(videofile,argv[2],argv[3],feaid++);
				}
				else
				{
					getfeature_jpg(videofile, argv[2],argv[3],feaid, &duration, jpgnum);
					//getfeature(videofile,argv[2],argv[3],feaid);
				}
				printf("================================Duration is %I64d s\n", duration);
			}
		}
	}
		
	closedir(videodb);
}
