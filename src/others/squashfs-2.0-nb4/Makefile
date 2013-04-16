all:
	make -C nb4-mksquashfs
	make -C nb4-unsquashfs
	cp nb4-mksquashfs/mksquashfs .
	cp nb4-unsquashfs/unsquashfs .

clean:
	make -C nb4-mksquashfs clean
	make -C nb4-unsquashfs clean
	rm -rf mksquashfs unsquashfs
