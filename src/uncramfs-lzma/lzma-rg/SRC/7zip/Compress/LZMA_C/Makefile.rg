RGSRC=../../../../../..
include $(RGSRC)/envir.mak

MOD_TARGET=lzma_decode.o
MOD_2_STAT=lzma_decode.o
O_OBJS=LzmaDecode.o decode.o

LINK_DIR=$(BUILDDIR)/os/linux/lib/lzma_decode

include $(RGMK)

ifdef CONFIG_RG_LZMA_COMPRESSED_KERNEL
$(LINK_DIR):
	mkdir -p $(BUILDDIR)/os/linux/lib
	$(RG_LN) $(PWD_SRC) $@

archconfig::$(LINK_DIR)
endif
