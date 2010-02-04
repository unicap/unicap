all: raw_image

raw_image: raw_image.c
	$(CC) -o $@ $^ `pkg-config libunicap --cflags --libs`

clean: 
	rm -f *.o
	rm -f raw_image