cplus = g++
cc = gcc
CFLAGS = -g -fPIC -I ../ 
LDFLAGS = -lm -lpthread -lz -lbz2 -ldl
EXTRALIBS = -L../../ -lavformat -lavcodec -lavutil
objects+=decode.o
main_obj+=main.o 

all:libdecode.so decode

.cpp.o:
	$(cplus) $(CFLAGS) -c $<
	
.c.o:
	$(cc) $(CFLAGS) -c $<

decode:$(libdecode.so) $(main_obj)
	$(cc) -o decode main.o -L. -ldecode  -Wl,-rpath,./

libdecode.so: $(objects)
	$(cc) -shared -o libdecode.so $(objects) $(EXTRALIBS) $(LDFLAGS)

clean:
	rm *.o decode *.so 
