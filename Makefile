libmaliwrap.so: egl.c
	gcc egl.c -ldrm -I/usr/include/libdrm -g --shared -fPIC -o libmaliwrap.so

clean:
	-rm libmaliwrap.so
