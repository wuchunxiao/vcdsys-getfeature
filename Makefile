cplus = g++
cc = gcc
CFLAGS = -g -fPIC -I . -I ./include 
LDFLAGS = -lm -lpthread -lz -lbz2 -ldl
EXTRALIBS = -L../ -lavformat -lavcodec -lavutil -L. -ljpeg
objects+=getfeature.o FeatureExtraction.o encoder.o Reduce.o FilterFrame.o save_jpg.o
main_obj+=main.o 

all:libgetfea.so getfea

.cpp.o:
	$(cplus) $(CFLAGS) -c $<
	
.c.o:
	$(cc) $(CFLAGS) -c $<

getfea:$(libgetfea.so) $(main_obj)
	$(cc) -o getfea main.o -L. -lgetfea -Wl,-rpath,./

libgetfea.so:$(objects)
	$(cc) -shared -o libgetfea.so $(objects) $(LDFLAGS) $(EXTRALIBS)

clean:
	rm *.o getfea *.so 
