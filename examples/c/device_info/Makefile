all: device_info

device_info: device_info.c
	$(CC) -g -O0 -o $@ $^ `pkg-config libunicap --cflags --libs`

clean:
	rm -f *.o
	rm -f device_info