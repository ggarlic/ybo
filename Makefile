CFLAGS=`pkg-config --cflags gtk+-2.0 gstreamer-0.10`
LIBS=`pkg-config --libs gtk+-2.0 gstreamer-0.10`

all: ybo

ybo: main.o playback.o
	gcc -o player main.o playback.o $(LIBS)

playback.o: playback.c playback.h
	gcc -g -c playback.c $(CFLAGS)

main.o: main.c playback.h
	gcc -g -c main.c $(CFLAGS)

clean:
	rm *.o player

