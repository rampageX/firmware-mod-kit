# Modifying Vendor Firmwares #

  * OR Extending Vendor Firmware Functionality
  * OR Hacking Vendor Firmwares

## Introduction ##

In the world of embedded Linux, Vendor firmwares are often preferred because they provide the maximum set of features and hardware compatibility. Often times, closed source hardware (e.g. WiFi) drivers offer additional functionality or improved performance. Therefore, it makes sense to provide instructions on flashing these vendor firmwares.

## Overview ##

In this document, we are going to provide a means to modify vendor firmwares, extending them with features and adding the desired functionality from third-party firmwares.

## Shell Access ##

TODO: Telnetd, sshd (dropbear)

## Writable File System ##

  * Squashfs - Read-only, compressed, file system. (best compression)
  * JFFS2 - Read-write, compressed, file system.

Squashfs is a read-only file system used in most firmware file formats because it offers superior compression to read-write file systems like JFFS2. Squashfs tars groups of files together into blocks and compresses them. Squashfs can use a number of compression algorithms, though ZLIB was the original. LZMA typically performs best and is commonly used in modern firmwares.

For firmwares with a writable root file system, the read-write JFFS2 file-system is often overlaid onto the Squashfs root mount. This means that changes to the Squashfs file system are written to the JFFS2 partition. This mechanism somewhat resembles 'copy-on-write' used in virtual memory, in that files are copied to the JFFS2 partition when they are modified. The abstracted mount the user sees is the combination of the static Squashfs mount and the modified JFFS2 mount.

```
               -- Squashfs (read-only, static file system)
(root mount)- / 
              \
               -- JFFS2 (modified files held here, copied on write)
```

This method works particularly well since most of a firmware image's file system normally remains entirely static. Only in cases where the user plans to make massive modifications to the file system should JFFS2 be preferred.

TODO: Creating a new JFFS2 partition; requirements (kernel and user mode), mapping the ROM, optional creation of an overlay.

## Platform Toolsets (cross-compiler) ##

TODO: Reference OpenWrt platform toolsets (SDKs, Image Builder)

## Extending the Web Interface ##

TODO: How to modify common webif formats

## Adding a Package Repository ##

TODO: document adding a package manager and propose creation of a third-party repository for common platforms. Recommend all packages be statically linked since shared libraries may be pruned in vendor firmwares.