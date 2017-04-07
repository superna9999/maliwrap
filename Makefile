libmaliwrap.so:
	gcc egl.c -Iinclude/ -g --shared -fPIC -o libmaliwrap.so

clean:
	-rm libmaliwrap.so
