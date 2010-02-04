all: smallapp

smallapp: smallapp.c
	$(CC) -o $@ $^ `pkg-config libunicapgtk --libs --cflags` -Wall $(CFLAGS) 

clean: 
	rm -f *.o
	rm -f *~
	rm -f smallapp
	rm -f TAGS
