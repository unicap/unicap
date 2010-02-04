all: device_property

device_property: device_property.c
	$(CC) -O0 -g -o $@ $^ `pkg-config libunicap --cflags --libs`

clean:
	rm -f *.o
	rm -f device_property