libmaliwrap.so: egl.c
	gcc egl.c -g --shared -fPIC -o libmaliwrap.so

clean:
	-rm libmaliwrap.so
