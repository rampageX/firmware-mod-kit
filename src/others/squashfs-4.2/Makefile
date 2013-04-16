all:
	make -C squashfs-tools
	cp squashfs-tools/unsquashfs .
	cp squashfs-tools/mksquashfs .

clean:
	make -C squashfs-tools clean
	rm -f unsquashfs mksquashfs
