/*
 * Copyright 2025 Electronic Arts Inc.
 *
 * Derived from Electronic Arts Inc.'s public source release for
 * Command & Conquer: Red Alert.
 * Licensed under GPLv3-or-later with additional Electronic Arts terms.
 * Modified for the macOS port, 2026.
 * See LICENSE.md.
 */

#include "iff.h"
#include "file.h"
#include <misc.h>
#include <wwstd.h>
#include <dos.h>
#include <wwmem.h>

static unsigned short Read_LE16(void const *ptr)
{
	unsigned char const *bytes = (unsigned char const *)ptr;
	return (unsigned short)(bytes[0] | (bytes[1] << 8));
}

static unsigned long Read_LE32(void const *ptr)
{
	unsigned char const *bytes = (unsigned char const *)ptr;
	return (unsigned long)bytes[0] |
		((unsigned long)bytes[1] << 8) |
		((unsigned long)bytes[2] << 16) |
		((unsigned long)bytes[3] << 24);
}


/*=========================================================================*/
/* The following PRIVATE functions are in this file:                       */
/*=========================================================================*/



/*= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =*/

/***************************************************************************
 * LOAD_DATA -- Loads a data file from disk.                               *
 *                                                                         *
 *    This routine will load a data file from disk.  It does no translation*
 *    on the data.                                                         *
 *                                                                         *
 * INPUT:   name  -- Pointer to ASCII filename of the data file.           *
 *                                                                         *
 *          ptr   -- Buffer to load the data file into.                    *
 *                                                                         *
 *          size  -- Maximum size of the buffer (in bytes).                *
 *                                                                         *
 * OUTPUT:  Returns with the number of bytes read.                         *
 *                                                                         *
 * WARNINGS:   none                                                        *
 *                                                                         *
 * HISTORY:                                                                *
 *   06/24/1991 JLB : Created.                                             *
 *=========================================================================*/
unsigned long __cdecl Load_Data(char const *name, void *ptr, unsigned long size)
{
	int fd;

	fd = Open_File(name, READ);
	size = Read_File(fd, ptr, size);
	Close_File(fd);
	return(size);
}


/***************************************************************************
 * WRITE_DATA -- Writes a block of data as a file to disk.                 *
 *                                                                         *
 *    This routine will write a block of data as a file to the disk.  It   *
 *    is the compliment of Load_Data.                                      *
 *                                                                         *
 * INPUT:   name     -- Name of the file to create.                        *
 *                                                                         *
 *          ptr      -- Pointer to the block of data to write.             *
 *                                                                         *
 *          size     -- Size of the data block to be written.              *
 *                                                                         *
 * OUTPUT:  Returns with the number of bytes actually written.             *
 *                                                                         *
 * WARNINGS:   none                                                        *
 *                                                                         *
 * HISTORY:                                                                *
 *   07/05/1992 JLB : Created.                                             *
 *=========================================================================*/
unsigned long __cdecl Write_Data(char const *name, void *ptr, unsigned long size)
{
	int fd;

	fd = Open_File(name, WRITE);
	size = Write_File(fd, ptr, size);
	Close_File(fd);
	return(size);
}


/***************************************************************************
 * LOAD_ALLOC_DATA -- Loads and allocates buffer for a file.               *
 *                                                                         *
 *    The routine will allocate a buffer and load the specified file into  *
 *    it.  The kind of memory used for the buffer is determined by the     *
 *    memory allocation flags passed in.                                   *
 *                                                                         *
 * INPUT:   name  -- Name of the file to load.                             *
 *                                                                         *
 *          flags -- Memory allocation flags to use when allocating.       *
 *                                                                         *
 * OUTPUT:  Returns with a pointer to the buffer that contains the file's  *
 *          data.                                                          *
 *                                                                         *
 * WARNINGS:   A memory error could occur if regular memory flags are      *
 *             specified.  If XMS memory is specified, then this routine   *
 *             could likely return NULL.                                   *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/28/1992 JLB : Created.                                             *
 *=========================================================================*/
void * __cdecl Load_Alloc_Data(char const *name, MemoryFlagType flags)
{
	int	fd;		// Working file handle.
	unsigned long	size;		// Size of the file to load.
	void	*buffer;	// Buffer to hold the file.

	fd = Open_File(name, READ);
	size = File_Size(fd);
	buffer = Alloc(size, flags);
	if (buffer) {
		Read_File(fd, buffer, size);
	}
	Close_File(fd);
	return(buffer);
}


/***************************************************************************
 * LOAD_UNCOMPRESS -- Load and uncompress the given file.                  *
 *                                                                         *
 * INPUT:      char *					- file name to uncompress 					*
 *					GraphicBufferClass&	- to load the source data into			*
 *					GraphicBufferClass&	- for the picture								*
 *             void *					- ptr for header uncompressed data     *
 *                                                                         *
 * OUTPUT:     unsigned long size of uncompressed data                             *
 *                                                                         *
 * WARNINGS:   none                                                        *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/28/1991  CY : Created.                                             *
 *   06/26/1991 JLB : Handles load & uncompress to same buffer.            *
 *=========================================================================*/
unsigned long __cdecl Load_Uncompress(char const *file, BufferClass& uncomp_buff, BufferClass& dest_buff, void *reserved_data)
{
	int	fd;				// Source file handle.
	unsigned int	isize=0;			// Size of the file.
	unsigned int	skipsize;		// Size of the skip data bytes.
	void	*uncomp_ptr;	//	Source buffer pointer.
	char	*newuncomp_ptr;	// Adjusted source pointer.


	uncomp_ptr = uncomp_buff.Get_Buffer();		// get a pointer to buffer

	/*======================================================================*/
	/* Read the file into the uncompression buffer.									*/
	/*======================================================================*/

	fd = Open_File(file, READ);					// Open up the file to read from
	Read_File(fd, (char *) &isize,	2L);		// Read the file size
	Read_File(fd, uncomp_ptr, 			8L);		// Read the header bytes in.
	isize -= 8; 										// Remaining data in file.

	/*======================================================================*/
	/* Check for and read in the skip data block.									*/
	/*======================================================================*/

	skipsize = Read_LE16((char *)uncomp_ptr + 6);

	if (reserved_data && skipsize) {
		Read_File(fd, reserved_data, (unsigned long) skipsize);
	} else {
		Seek_File(fd, skipsize, SEEK_CUR);
	}

	((unsigned char *)uncomp_ptr)[6] = 0;				// K/O any skip value.
	((unsigned char *)uncomp_ptr)[7] = 0;
	isize 							-= skipsize;

	/*======================================================================*/
	/*	If the source and dest buffer are the same, we adjust the pointer so */
	/* that the compressed data is loaded into the end of the buffer.  In 	*/
	/* this way the uncompress code can write to the same buffer.				*/
	/*======================================================================*/
	newuncomp_ptr = (char *)Add_Long_To_Pointer(uncomp_buff.Get_Buffer(), uncomp_buff.Get_Size() - (isize+8L));

	/*======================================================================*/
	/*	Duplicate the header bytes.														*/
	/*======================================================================*/
	Mem_Copy(uncomp_ptr,newuncomp_ptr,8);

	/*======================================================================*/
	/*	Read in the main compressed part of the file.								*/
	/*======================================================================*/
	Read_File(fd, newuncomp_ptr + 8, (unsigned long)isize);
	Close_File(fd);

	/*======================================================================*/
	/* Uncompress the file into the destination buffer (which may very well	*/
	/*		be the source buffer).															*/
	/*======================================================================*/
	return(Uncompress_Data(newuncomp_ptr, dest_buff.Get_Buffer()));
}
#if(0)
/***************************************************************************
 * LOAD_UNCOMPRESS -- Load and uncompress the given file.                  *
 *                                                                         *
 * INPUT:      char *file name to uncompress, BuffType uncomp_buff to load *
 *             the source data into, BuffType dest_buff for the picture,   *
 *             void *reserved_data pointer for header uncompressed data    *
 *                                                                         *
 * OUTPUT:     unsigned long size of uncompressed data                             *
 *                                                                         *
 * WARNINGS:   none                                                        *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/28/1991  CY : Created.                                             *
 *   06/26/1991 JLB : Handles load & uncompress to same buffer.            *
 *=========================================================================*/
unsigned long __cdecl Load_Uncompress(char const *file, BuffType uncomp_buff, BuffType dest_buff, void *reserved_data)
{
	int	fd;				// Source file handle.
	unsigned int	isize;			// Size of the file.
	unsigned int	skipsize;		// Size of the skip data bytes.
	void	*uncomp_ptr;	//	Source buffer pointer.
	char	*newuncomp_ptr;	// Adjusted source pointer.


	uncomp_ptr = Get_Buff(uncomp_buff);		/* Get pointer to uncomp buffer */

	/* Read the file into the uncomp_buff */

	fd = Open_File(file, READ);
	Read_File(fd, (char *) &isize, 2L);				/* Read the file size 		*/
	#if(AMIGA)
		isize = Reverse_Word(isize);
	#endif

	Read_File(fd, uncomp_ptr, 8L);		// Read the header bytes in.
	isize -= 8; 								// Remaining data in file.

	/*
	**	Check for and read in the skip data block.
	*/

	skipsize = *(((short*)uncomp_ptr) + 3);
	#if(AMIGA)
		skipsize = Reverse_Word(skipsize);
	#endif

	if (reserved_data && skipsize) {
		Read_File(fd, reserved_data, (unsigned long) skipsize);
	} else {
		Seek_File(fd, skipsize, SEEK_CUR);
	}
	*( ((short *)uncomp_ptr+3) ) = 0;		// K/O any skip value.
	isize -= skipsize;

	/*
	**	If the source and dest buffer are the same, we
	**	adjust the pointer so that the compressed data is
	**	loaded into the end of the buffer.  In this way the
	**	uncompress code can write to the same buffer.
	*/
	#if(IBM)
		newuncomp_ptr = (char *)Add_Long_To_Pointer(Get_Buff(uncomp_buff), PageArraySize[uncomp_buff] - (isize+8L));
	#else
		newuncomp_ptr = Get_Buff(uncomp_buff);
		newuncomp_ptr += PageArraySize[uncomp_buff] - ((isize+10) & 0xFFFE);
	#endif

	/*
	**	Duplicate the header bytes.
	*/
	Mem_Copy(uncomp_ptr,newuncomp_ptr,8);

	/*
	**	Read in the main compressed part of the file.
	*/
	Read_File(fd, newuncomp_ptr + 8, (unsigned long)isize);
	Close_File(fd);

	return(Uncompress_Data(newuncomp_ptr, Get_Buff(dest_buff)));
}

#endif
/***************************************************************************
 * Uncompress_Data -- Uncompresses data from one buffer to another.        *
 *                                                                         *
 *    This routine takes data from a compressed file (sans the first two   *
 *    size bytes) and uncompresses it to a destination buffer.  The source *
 *    data MUST have the CompHeaderType at its start.                      *
 *                                                                         *
 * INPUT:   src   -- Source compressed data pointer.                       *
 *                                                                         *
 *          dst   -- Destination (paragraph aligned) pointer.              *
 *                                                                         *
 * OUTPUT:  Returns with the size of the uncompressed data.                *
 *                                                                         *
 * WARNINGS:   If LCW compression is used, the destination buffer must     *
 *             be paragraph aligned.                                       *
 *                                                                         *
 * HISTORY:                                                                *
 *   09/17/1993 JLB : Created.                                             *
 *=========================================================================*/
unsigned long __cdecl Uncompress_Data(void const *src, void *dst)
{
	unsigned int					skip;			// Number of leading data to skip.
	CompressionType	method;		// Compression method used.
	unsigned long					uncomp_size=NULL;

	if (!src || !dst) return(NULL);

	/*
	**	Interpret the data block header structure to determine
	**	compression method, size, and skip data amount.
	*/
	unsigned char const *header = (unsigned char const *)src;
	uncomp_size = Read_LE32(header + 2);
	skip = Read_LE16(header + 6);
	method = (CompressionType)header[0];
	src = Add_Long_To_Pointer((void *)src, 8L + (long)skip);

	switch (method) {

		default:
		case NOCOMPRESS:
			Mem_Copy((void *) src, dst, uncomp_size);
			break;

		case HORIZONTAL:
#if LIB_EXTERNS_RESOLVED
			RLE_Uncompress((void *) src, dst, uncomp_size);
#endif
			break;

		case LCW:
			LCW_Uncompress((void *) src, (void *) dst, (unsigned long) uncomp_size);
			break;

	}

	return(uncomp_size);
}

extern "C" int __cdecl LCW_Uncompress_Bounded(void const *source, unsigned long source_length, void *dest, unsigned long dest_length, unsigned long *written);

unsigned long __cdecl Uncompress_Data_Bounded(void const *src, unsigned long src_size, void *dst, unsigned long dst_size)
{
	if (!src || !dst || src_size < 8) return 0;

	unsigned char const *header = (unsigned char const *)src;
	unsigned long uncomp_size = Read_LE32(header + 2);
	unsigned long skip = Read_LE16(header + 6);
	if (uncomp_size == 0 || uncomp_size > dst_size || skip > src_size - 8) return 0;

	unsigned char const *data = header + 8 + skip;
	unsigned long data_size = src_size - 8 - skip;
	switch ((CompressionType)header[0]) {
		case NOCOMPRESS:
			if (uncomp_size > data_size) return 0;
			Mem_Copy(data, dst, uncomp_size);
			return uncomp_size;

		case LCW: {
			unsigned long written = 0;
			return LCW_Uncompress_Bounded(data, data_size, dst, dst_size, &written) && written == uncomp_size ? written : 0;
		}

		default:
			return 0;
	}
}
