SQDIR=squashfs-tools

all:
	make -C $(SQDIR)
	cp $(SQDIR)/unsquashfs .
	cp $(SQDIR)/mksquashfs .

clean:
	make -C $(SQDIR) clean
	rm -f unsquashfs mksquashfs
