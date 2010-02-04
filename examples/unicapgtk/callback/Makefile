all: callback

callback: callback.c
	$(CC) -o $@ $^ $(CFLAGS) `pkg-config libunicapgtk libucil --libs --cflags`

clean: 
	rm -f *.o
	rm -f *~
	rm -f callback
	rm -f TAGS
