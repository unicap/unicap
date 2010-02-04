all: sdl_display sdl_display_multibuffer

sdl_display: sdl_display.c
	$(CC) -g -o $@ $^ `pkg-config libunicap --cflags --libs` `sdl-config --cflags --libs`

sdl_display_multibuffer: sdl_display_multibuffer.c
	$(CC) -g -o $@ $^ `pkg-config libunicap --cflags --libs` `sdl-config --cflags --libs`

clean: 
	rm -f *.o
	rm -f *~
	rm -f sdl_display sdl_display_multibuffer
