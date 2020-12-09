all: xwm

xwm: xwm.o isIcon.o
	cc xwm.c isIcon.c -o xwm -L/usr/X11R6/lib -lX11

xwm.o: xwm.c
	cc -c xwm.c

isIcon.o: isIcon.c
	cc -c isIcon.c

clean:
	rm -rf *.o xwm
	