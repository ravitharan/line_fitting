
CFLAGS = -g -MD -O2

ASFLAGS = -g -MD -march=skylake

average : average.o avg_x86.o

clean :
	rm -rf average *.o
