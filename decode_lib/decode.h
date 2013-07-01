typedef struct _CallBackF
{
	int (*sendFrame)(unsigned char *, int, int, int);
	int (*sendSmp)(unsigned char *, int, int, int);

}CallBackF;

extern int decode_callback(char* videofile, CallBackF * callbackf, int useid);
