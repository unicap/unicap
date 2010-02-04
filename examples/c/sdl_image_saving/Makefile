all: sdl_image_saving

sdl_image_saving: sdl_image_saving.c colorspace.c
	$(CC) -o $@ $^ `pkg-config libunicap --cflags --libs` `sdl-config --cflags --libs` -ljpeg

clean: 
	rm -f *.o
	rm -f *~
	rm -f sdl_image_saving