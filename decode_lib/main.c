#include <stdio.h>
#include <stdlib.h>
#include "decode.h"

int frameCount = 0;

int processFrame(unsigned char* yuvdata, int width, int height, int useid)
{
	printf("%d : width=%d\theight=%d\n", frameCount++,width,height);
}

int main(int argc, char** argv)
{
	if(argc < 2)
	{
		printf("Para Error!\n");
		printf("Use: decode videopath\n");
	}
	
	CallBackF callBack;
	callBack.sendFrame = processFrame;
	decode_callback(argv[1],&callBack,0);

}
