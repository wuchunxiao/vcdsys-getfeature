typedef struct _CallBackF
{
	int (*sendFrame)(unsigned char *, int, int, int);
	int (*sendSmp)(unsigned char *, int, int, int);

}CallBackF;

extern int getfeature(char* videofile,char* fealibpath, char * adspath, int id);
extern int getfeature_for_display(char* videofile,char* fealibpath, char * abspath, CallBackF * callbackf, int id, int useid);
extern int getfeature_callback(char* videofile, CallBackF * callbackf, int useid);
