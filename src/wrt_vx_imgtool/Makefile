# this makefile is for linux, use cl for Windows and undefine _LINUX
INCLUDEDIR = .
CXX := g++
CFLAGS := -O3 -D_LINUX

all: wrt_vx_imgtool

wrt_vx_imgtool: 
	$(CXX) $(CFLAGS) wrt54gv5_img.cpp -o $@
	
clean:
	rm -f *.o
	rm -f wrt_vx_imgtool
