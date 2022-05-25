top: nerv_main.c
	gcc -Wall nerv_main.c `pkg-config fuse3 --cflags --libs` -o nerv_bin

start:
	mkdir -p nerv
	./nerv_bin nerv

fclean:
	fusermount -uz nerv

close:
	umount nerv
	rm -r nerv

clean:
	rm nerv_bin

status:
	ps -aux | grep "nerv"
	mount | grep "nerv"