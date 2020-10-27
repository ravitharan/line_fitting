
CFLAGS = -g -MD

ASFLAGS = -g -march=skylake

#average : average.o
average : average.o avg_x86.o


clean :
	rm -rf average *.o
