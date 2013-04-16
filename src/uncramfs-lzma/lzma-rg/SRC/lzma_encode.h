/****************************************************************************
 *
 * rg/pkg/lzma/SRC/lzma_encode.h
 * 
 * Copyright (C) Jungo LTD 2004
 * 
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General 
 * Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
 * MA 02111-1307, USA.
 *
 * Developed by Jungo LTD.
 * Residential Gateway Software Division
 * www.jungo.com
 * info@jungo.com
 */

#ifndef _LZMA_ENCODE_H_
#define _LZMA_ENCODE_H_

/* Compress a buffer usinf LZMA method. This function allows the user to set
 * most of compression parameters. */
int lzma_encode_extended(char *dest, unsigned long *dest_len,
    char *source, unsigned long src_len, unsigned int dicLog,
    unsigned int posStateBits, unsigned int litContextBits,
    unsigned int litPosBits,
    unsigned int algorithm, unsigned int numFastBytes);

/* Compress a buffer usinf LZMA method using default compression settings */
int lzma_encode(char *dest, unsigned long *dest_len, char *source,
    unsigned long src_len);

#endif
