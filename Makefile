CFLAGS=`pkg-config --cflags gtk+-2.0 gstreamer-0.10`
LIBS=`pkg-config --libs gtk+-2.0 gstreamer-0.10`

all: ybo

ybo: main.o
	gcc -o ybo main.o $(LIBS)

%.o: %.c
	gcc -g -c $< $(CFLAGS)
