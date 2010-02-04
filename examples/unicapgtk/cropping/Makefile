all: unicapgtk_cropping

unicapgtk_cropping: unicapgtk_cropping.c
	$(CC) -o $@ $^ `pkg-config libunicapgtk --libs --cflags`

clean: 
	rm -f *.o
	rm -f *~
	rm -f unicapgtk_cropping
	rm -f TAGS
