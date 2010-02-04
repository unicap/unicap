all: sdl_display

sdl_display: sdl_display.c
	$(CC) -o $@ $^ `pkg-config libunicap --cflags --libs` `sdl-config --cflags --libs`

clean: 
	rm -f *.o
	rm -f *~
	rm -f sdl_display