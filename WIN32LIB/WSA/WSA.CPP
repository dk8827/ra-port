/*
 * Copyright 2025 Electronic Arts Inc.
 *
 * Derived from Electronic Arts Inc.'s public source release for
 * Command & Conquer: Red Alert.
 * Licensed under GPLv3-or-later with additional Electronic Arts terms.
 * Modified for the macOS port, 2026.
 * See LICENSE.md.
 */

#include <wwstd.h>
#include "wsa.h"
#include "wsa_file_format.h"
#include <wwmem.h>
#include <file.h>
#include <misc.h>
#include <iff.h>
#include <mem.h>
#include <string.h>
#include <stdint.h>

//
// WSA animation header allocation type.
// If we need more then 8 flags for the flags variable, we can combine
// USER_ALLOCATED with SYS_ALLOCATED  and combine FILE with RESIDENT.
//
#define	WSA_USER_ALLOCATED 	0x01
#define	WSA_SYS_ALLOCATED  	0x02
#define	WSA_FILE           	0x04
#define	WSA_RESIDENT       	0x08
#define	WSA_TARGET_IN_BUFFER	0x10
#define	WSA_LINEAR_ONLY		0x20
#define	WSA_FRAME_0_ON_PAGE  0x40
#define	WSA_AMIGA_ANIMATION  0x80
#define	WSA_PALETTE_PRESENT	0x100
#define	WSA_FRAME_0_IS_DELTA	0x200

// These are used to call Apply_XOR_Delta_To_Page_Or_Viewport() to setup flags parameter.  If
// These change, make sure and change their values in lp_asm.asm.
#define	DO_XOR					0x0
#define	DO_COPY					0x01
#define	TO_VIEWPORT				0x0
#define	TO_PAGE					0x02




typedef struct {
	unsigned short current_frame;
	unsigned short total_frames;
	unsigned short pixel_x;
	unsigned short pixel_y;
	unsigned short pixel_width;
	unsigned short pixel_height;
	unsigned short largest_frame_size;
	char *delta_buffer;
	char *file_buffer;
	char file_name[ 13 ];
	short flags;
	// New fields that animate does not know about below this point. SEE EXTRA_charS_ANIMATE_NOT_KNOW_ABOUT
	short file_handle;
	unsigned long anim_mem_size;
} SysAnimHeaderType;

// NOTE:"THIS IS A BAD THING. SINCE sizeof(SysAnimHeaderType) CHANGED, THE ANIMATE.EXE
// UTILITY DID NOT KNOW I UPDATED IT, IT ADDS IT TO largest_frame_size BEFORE SAVING
// IT TO THE FILE.  THIS MEANS I HAVE TO ADD THESE charS ON NOW FOR IT TO WORK.
#define EXTRA_charS_ANIMATE_NOT_KNOW_ABOUT	(sizeof(short) + sizeof(unsigned long))


//
// Header structure for the file.
// NOTE:  The 'total_frames' field is used to differentiate between Amiga and IBM
// animations.  Amiga animations have the HIGH bit set.
//
// The on-disk header uses 4-byte offsets and no padding; keep the struct
// byte-packed with fixed-width fields so it matches on LP64 platforms.
#pragma pack(push, 1)
typedef struct {
	unsigned short total_frames;
	unsigned short pixel_x;
	unsigned short pixel_y;
	unsigned short pixel_width;
	unsigned short pixel_height;
	unsigned short largest_frame_size;
	short	flags;
	uint32_t frame0_offset;
	uint32_t frame0_end;
	/* uint32_t data_seek_offset, unsigned short frame_size ... */
} WSA_FileHeaderType;
#pragma pack(pop)

/*=========================================================================*/
/* The following PRIVATE functions are in this file:                       */
/*=========================================================================*/

PRIVATE unsigned long Get_Resident_Frame_Offset( char *file_buffer, int frame );
PRIVATE unsigned long Get_File_Frame_Offset( int file_handle, int frame, int palette_adjust);
PRIVATE BOOL Apply_Delta(SysAnimHeaderType *sys_header, int curr_frame, char *dest_ptr, int dest_w);
/*= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =*/


/***************************************************************************
 * OPEN_ANIMATION -- Opens an animation file and reads into buffer         *
 *                                                                         *
 * INPUT:   char *file_name of animation sequence file.                    *
 *          char *user_buffer pointer if one exists  (NULL ok)             *
 *          unsigned long user_buffer_size if known  (NULL ok)                     *
 *          WSAOpenType user_flags - flags on how to open.                 *
 *          unsigned char *palette - pointer to palette space for return (NULL ok) *
 *                                                                         *
 * OUTPUT:  void *pointer to animation data.  Must be used for all 			*
 *               other WSA calls.                                          *
 *                                                                         *
 * WARNINGS: May return NULL, please check.                                *
 *                                                                         *
 * HISTORY:                                                                *
 *   11/26/1991  SB : Created.                                             *
 *=========================================================================*/
void * __cdecl Open_Animation(char const *file_name, char *user_buffer, long user_buffer_size, WSAOpenType user_flags, unsigned char *palette)
{
	int						fh, anim_flags;
	int						palette_adjust;
	unsigned int						offsets_size;
	unsigned int						frame0_size;
	long						target_buffer_size, delta_buffer_size, file_buffer_size;
	long						max_buffer_size,    min_buffer_size;
	char						*sys_anim_header_buffer;
	char						*target_buffer;
	char						*delta_buffer, *delta_back;
	SysAnimHeaderType		*sys_header;
	WSA_FileHeaderType	file_header;

	/*======================================================================*/
	/* Open the file to get the header information									*/
	/*======================================================================*/

	anim_flags	= 0;
	fh				= Open_File(file_name, READ);
	Read_File(fh, (char *) &file_header, sizeof(WSA_FileHeaderType));

	/*======================================================================*/
	/* If the file has an attached palette then if we have a valid palette	*/
	/*		pointer we need to read it in.												*/
	/*======================================================================*/

	if (file_header.flags & 1) {
		anim_flags		|= WSA_PALETTE_PRESENT;
		palette_adjust	 = 768;

		if (palette != NULL) {
			Seek_File(fh, WSA_FILE_OFFSET_SIZE * (file_header.total_frames), SEEK_CUR);
			Read_File(fh, palette, 768L);
		}

	} else {
		palette_adjust = 0;
	}

	// Check for flag from ANIMATE indicating that this animation was
	// created from a .LBM and a .ANM.  These means that the first
	// frame is a XOR Delta from a picture, not black.
	if (file_header.flags & 2) {
		anim_flags		|= WSA_FRAME_0_IS_DELTA;
	}


	// Get the total file size minus the size of the first frame and the size
	// of the file header.  These will not be read in to save even more space.
	file_buffer_size = Seek_File(fh, 0L, SEEK_END);

	if (file_header.frame0_offset) {
		long tlong;

		tlong = file_header.frame0_end - file_header.frame0_offset;
		frame0_size = (unsigned short) tlong;
	}
	else {
		anim_flags |= WSA_FRAME_0_ON_PAGE;
		frame0_size = 0;
	}

	file_buffer_size -= palette_adjust + frame0_size + WSA_FILE_HEADER_SIZE;

	// We need to determine the buffer sizes required for the animation.  At a
	// minimum, we need a target buffer for the uncompressed frame and a delta
	// buffer for the delta data.  We may be able to make the file resident,
	// so we will determine the file size.
	//
	// If the target buffer is in the user buffer
	// Then figure its size
	// and set the allocation flag
	// Else size is zero.
	//
	if (user_flags & WSA_OPEN_DIRECT) {
	 	target_buffer_size = 0L;
	}
	else {
		anim_flags |= WSA_TARGET_IN_BUFFER;
		target_buffer_size = (unsigned long) file_header.pixel_width * file_header.pixel_height;
	}

	// NOTE:"THIS IS A BAD THING. SINCE sizeof(SysAnimHeaderType) CHANGED, THE ANIMATE.EXE
	// UTILITY DID NOT KNOW I UPDATED IT, IT ADDS IT TO largest_frame_size BEFORE SAVING
	// IT TO THE FILE.  THIS MEANS I HAVE TO ADD THESE charS ON NOW FOR IT TO WORK.
	// On 64-bit, SysAnimHeaderType grew (pointers/padding), so the original
	// EXTRA_charS compensation under-sizes the delta region. Reserve the full
	// current header size instead; the file's largest_frame_size already
	// includes the historical 32-bit header slack, so this only over-allocates.
	delta_buffer_size  = (unsigned long) file_header.largest_frame_size + sizeof(SysAnimHeaderType);
	min_buffer_size = target_buffer_size + delta_buffer_size;
	max_buffer_size = min_buffer_size + file_buffer_size;

	// check to see if buffer size is big enough for at least min required
	if (user_buffer && (user_buffer_size < min_buffer_size)) {
		Close_File(fh);
		return(NULL);
	}

	// A buffer was not passed in, so do allocations
	if (user_buffer == NULL) {

		// If the user wants it from the disk, then let us give it to him,
		// otherwise, try to give a max allocation he can have.
		if (user_flags & WSA_OPEN_FROM_DISK) {
			user_buffer_size = min_buffer_size;
		}
		// else no buffer size, then try max configuration.
		else if (!user_buffer_size) {
			user_buffer_size = max_buffer_size;
		}
		// else if buffer specified is less then max needed, give min.
		else if (user_buffer_size < max_buffer_size) {
			user_buffer_size = min_buffer_size;
		}
		// otherwise we only want to alloc what we need.
		else {
			user_buffer_size = max_buffer_size;
		}


		// Check to see if enough RAM available for buffer_size.
		if (user_buffer_size > Ram_Free(MEM_NORMAL)) {

 			// If not enough room for even the min, return no buffer.

			if (min_buffer_size > Ram_Free(MEM_NORMAL)) {
				Close_File(fh);
				return(NULL);
			}

			// Else make buffer size the min and allocate it.
			user_buffer_size = min_buffer_size;
		}

		// allocate buffer needed
		user_buffer = (char *) Alloc(user_buffer_size, MEM_CLEAR);

		anim_flags |= WSA_SYS_ALLOCATED;
	}
	else {
		// Check to see if the user_buffer_size should be min or max.
		if ((user_flags & WSA_OPEN_FROM_DISK) || (user_buffer_size < max_buffer_size)) {
		 	user_buffer_size = min_buffer_size;
		}
		else {
		 	user_buffer_size = max_buffer_size;
		}
		anim_flags |= WSA_USER_ALLOCATED;
	}


	// Set the pointers to the RAM buffers
	sys_anim_header_buffer = user_buffer;
	target_buffer = (char *) Add_Long_To_Pointer(sys_anim_header_buffer, sizeof(SysAnimHeaderType));
	delta_buffer  = (char *) Add_Long_To_Pointer(target_buffer, target_buffer_size);

	//	Clear target buffer if it is in the user buffer.
	if (target_buffer_size) {
		memset( target_buffer, 0, (unsigned short) target_buffer_size );
	}

	// Poke data into the system animation header (start of user_buffer)
	// current_frame is set to total_frames so that Animate_Frame() knows that
	// it needs to clear the target buffer.

	sys_header = ( SysAnimHeaderType * ) sys_anim_header_buffer;
	sys_header -> current_frame =
	sys_header -> total_frames  = file_header.total_frames;
	sys_header -> pixel_x		 = file_header.pixel_x;
	sys_header -> pixel_y		 = file_header.pixel_y;
	sys_header -> pixel_width   = file_header.pixel_width;
	sys_header -> pixel_height  = file_header.pixel_height;
	sys_header -> anim_mem_size = user_buffer_size;
	sys_header -> delta_buffer  = delta_buffer;
	sys_header -> largest_frame_size =
                 (unsigned short) (delta_buffer_size - sizeof(SysAnimHeaderType));

	strcpy(sys_header->file_name, file_name);

	// Figure how much room the frame offsets take up in the file.
	// Add 2 - one for the wrap around and one for the final end offset.
	offsets_size = (file_header.total_frames + 2) << 2;

	// Can the user_buffer_size handle the maximum case buffer?
	if ( user_buffer_size == max_buffer_size) {

		//
		//	set the file buffer pointer,
		// Skip over the header information.
		// Read in the offsets.
		// Skip over the first frame.
		// Read in remaining frames.
		//

		sys_header->file_buffer = (char *)Add_Long_To_Pointer(delta_buffer,sys_header->largest_frame_size);
		Seek_File( fh, WSA_FILE_HEADER_SIZE, SEEK_SET);
		Read_File( fh, sys_header->file_buffer, offsets_size);
		Seek_File( fh, frame0_size + palette_adjust, SEEK_CUR);
		Read_File( fh, sys_header->file_buffer + offsets_size,
		           file_buffer_size - offsets_size);

		//
		// Find out if there is an ending value for the last frame.
		// If there is not, then this animation will not be able to
		// loop back to the beginning.
		//
		if (Get_Resident_Frame_Offset( sys_header->file_buffer, sys_header->total_frames + 1))
			anim_flags |= WSA_RESIDENT;
		else
			anim_flags |= WSA_LINEAR_ONLY | WSA_RESIDENT;
	}
	else {	// buffer cannot handle max_size of buffer

		if(Get_File_Frame_Offset( fh, sys_header->total_frames + 1, palette_adjust))
			anim_flags |= WSA_FILE;
		else
			anim_flags |= WSA_LINEAR_ONLY | WSA_FILE;
////
		sys_header->file_buffer = NULL;
	}

	// Figure where to back load frame 0 into the delta buffer.
	delta_back = (char *)Add_Long_To_Pointer(delta_buffer,
					 sys_header->largest_frame_size - frame0_size);

	// Read the first frame into the delta buffer and uncompress it.
	// Then close it.
	Seek_File( fh, WSA_FILE_HEADER_SIZE + offsets_size + palette_adjust, SEEK_SET);
	Read_File( fh, delta_back, frame0_size);

	// We do not use the file handle when it is in RAM.
	if (anim_flags & WSA_RESIDENT) {
		sys_header -> file_handle = (short) -1;
	 	Close_File(fh);
	}
	else {
		sys_header -> file_handle = (short)fh;
	}

	LCW_Uncompress(delta_back, delta_buffer, sys_header->largest_frame_size);

	// Finally set the flags,
	sys_header->flags = (short)anim_flags;

	// return valid handle
	return( user_buffer );
}


/***************************************************************************
 * CLOSE_ANIMATION -- Close the animation, freeing the space if necessary. *
 *                                                                         *
 * INPUT:      void *handle to the animation data buffer                   *
 *                                                                         *
 * OUTPUT:     none                                                        *
 *                                                                         *
 * WARNINGS:   handle MUST have been returned by Open_Animation            *
 *                                                                         *
 * HISTORY:                                                                *
 *   11/23/1991  ML : Created.                                             *
 *=========================================================================*/
void __cdecl Close_Animation( void *handle )
{
	SysAnimHeaderType *sys_header;

	// Assign our local system header pointer to the beginning of the handle space
	sys_header = (SysAnimHeaderType *) handle;

	// Close the WSA file in it was disk based.
	if (sys_header->flags & WSA_FILE) {
		Close_File(sys_header->file_handle);
	}

	// Check to see if the buffer was allocated OR the programmer provided the buffer
	if (handle && sys_header->flags & WSA_SYS_ALLOCATED) {
		Free(handle);
	}
}

/***************************************************************************
 * ANIMATE_FRAME -- Displays a frame of a given animation                  *
 *                                                                         *
 * INPUT:   void *handle to the animation.                                 *
 *          int frame_number wanted to be displayed                       *
 *          int x_pixel position of left side of animation on page        *
 *          int y_pixel position of top of animation on page              *
 *                                                                         *
 * OUTPUT:  BOOL if successfull or not.                                   *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   11/27/1991  SB : Created.                                             *
 *=========================================================================*/
#pragma off (unreferenced)
BOOL __cdecl Animate_Frame(void *handle, GraphicViewPortClass& view,
                         int frame_number, int x_pixel, int y_pixel,
                         WSAType flags_and_prio, void *magic_cols, void *magic)
{
	SysAnimHeaderType 	*sys_header;		// fix up the void pointer past in.
	int 						curr_frame;			// current frame we are on.
	int						total_frames;		// number of frames in anim.
	int						distance;			// distance to desired frame.
	int						search_dir;			// direcion to search for desired frame.
	int						search_frames;		// How many frames to search.
	int						loop;					// Just a loop varible.
	char						*frame_buffer;		// our destination.
	BOOL						direct_to_dest;	// are we going directly to the destination?
	int						dest_width;			// the width of the destination buffer or page.


	// Assign local pointer to the beginning of the buffer where the system information
	// resides
	sys_header = (SysAnimHeaderType 	*)handle;

	// Get the total number of frames
	total_frames = sys_header->total_frames;

	// Are the animation handle and the frame number valid?
	if (!handle || (total_frames <= frame_number)) {
		return FALSE;
	}

	if (view.Lock()!=TRUE) return (FALSE);

	// Decide if we are going to a page or a viewport (part of a buffer).
	dest_width	= view.Get_Width() + view.Get_XAdd() + view.Get_Pitch();

	//
	// adjust x_pixel and y_pixel by system pixel_x and pixel_y respectively.
	//
	x_pixel += (short)sys_header->pixel_x;
	y_pixel += (short)sys_header->pixel_y;

	//
	// Check to see if we are using a buffer inside of the animation buffer or if
	// it is being drawn directly to the destination page or buffer.
	//
	if (sys_header->flags & WSA_TARGET_IN_BUFFER) {
		// Get a pointer to the frame in animation buffer.
		frame_buffer = (char *)Add_Long_To_Pointer(sys_header, sizeof(SysAnimHeaderType));
		direct_to_dest = FALSE;
	}
	else {
		frame_buffer	= (char *)view.Get_Offset();
		frame_buffer  += (y_pixel * dest_width) + x_pixel;
		direct_to_dest	= TRUE;
	}
	//
	// If current_frame is equal to tatal_frames, then no animations have taken place
	// so must uncompress frame 0 in delta buffer to the frame_buffer/page if it
	// exists.
	//
	if (sys_header->current_frame == total_frames) {

		// Call apply delta telling it wether to copy or to xor depending on if the
		// target is a page or a buffer.

		if (!(sys_header->flags & WSA_FRAME_0_ON_PAGE)) {
			if (direct_to_dest) {

				// The last parameter says weather to copy or to XOR.  If  the
				// first frame is a DELTA, then it must be XOR'd.  A TRUE is
				// copy while FALSE is XOR.

				Apply_XOR_Delta_To_Page_Or_Viewport(frame_buffer, sys_header->delta_buffer,
				                        	sys_header->pixel_width, dest_width, //dest_width - sys_header->pixel_width,
													(sys_header->flags & WSA_FRAME_0_IS_DELTA)? DO_XOR : DO_COPY);
			}
			else {
				Apply_XOR_Delta(frame_buffer, sys_header->delta_buffer);
			}
		}
		sys_header->current_frame = 0;
	}

	//
	// Get the current frame
	// If no looping aloud, are the trying to do it anyways?
	//
	curr_frame = sys_header->current_frame;
#if (FALSE)
	// This is commented out since we will let them loop even though they should
	// not - it will be slower.
	if ( (sys_header->flags & WSA_LINEAR_ONLY) && (frame_number < cur_frame) ) {
		view.Unlock();
	 	return FALSE;
	}
#endif

	// Get absoulte distance from our current frame to the target frame
	distance = ABS(curr_frame - frame_number);

	// Assume we are searching right
	search_dir = 1;

	// Calculate the number of frames to search if we go right and wrap

	if (frame_number > curr_frame) {
		search_frames = total_frames - frame_number + curr_frame;

		// Is going right faster than going backwards?
		// Or are they trying to loop when the should not?
		if ((search_frames < distance) && !(sys_header->flags & WSA_LINEAR_ONLY)) {
			search_dir = -1;								// No, so go left
		}
		else {
			search_frames = distance;
		}
	}
	else {
		search_frames = total_frames - curr_frame + frame_number;

		// Is going right faster than going backwards?
		// Or are they trying to loop when the should not?
		if ((search_frames >= distance) || (sys_header->flags & WSA_LINEAR_ONLY)) {
			search_dir = -1;								// No, so go left
			search_frames = distance;
		}
	}

	// Take care of the case when we are searching right (possibly right)

	if (search_dir > 0) {
		for (loop = 0; loop < search_frames; loop++) {

			// Move the logical frame number ordinally right
			curr_frame += search_dir;

			Apply_Delta(sys_header, curr_frame, frame_buffer, dest_width);

			// Adjust the current frame number, taking into consideration that we could
			// have wrapped

			if (curr_frame == total_frames) {
				curr_frame = 0;
			}
		}
	}
	else {
		for (loop = 0; loop < search_frames; loop++) {

			// If we are going backwards and we are on frame 0, the delta to get
			// to the last frame is the n + 1 delta (wrap delta)

			if (curr_frame == 0) {
				curr_frame = total_frames;
			}

			Apply_Delta(sys_header, curr_frame, frame_buffer, dest_width);

			curr_frame += search_dir;
		}
	}

	sys_header->current_frame = (short)frame_number;

	// If we did this all in a hidden buffer, then copy it to the desired page or viewport.
	if (sys_header->flags & WSA_TARGET_IN_BUFFER) {
#if TRUE

		Buffer_To_Page(x_pixel, y_pixel, sys_header->pixel_width, sys_header->pixel_height, frame_buffer, view);

#else
		int	flags = ((unsigned short)flags_and_prio & 0xFF00u) >> 12u;
		int	pri = flags_and_prio & 0x00FF;

		Buffer_Bitblit_To_LogicPage(x_pixel, y_pixel, sys_header->pixel_width,
										  sys_header->pixel_height, 0, flags, frame_buffer, pri,
										  magic_cols, magic);
#endif
	}

	view.Unlock();
	return TRUE;
}
/***************************************************************************
 * ANIMATE_FRAME -- Displays a frame of a given animation                  *
 *                                                                         *
 * INPUT:   void *handle to the animation.                                 *
 *          int frame_number wanted to be displayed                       *
 *          int x_pixel position of left side of animation on page        *
 *          int y_pixel position of top of animation on page              *
 *                                                                         *
 * OUTPUT:  BOOL if successfull or not.                                   *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   11/27/1991  SB : Created.                                             *
 *=========================================================================*/
#pragma argsused
#ifdef cuts
BOOL __cdecl Animate_Frame(void *handle, GraphicViewPortClass& view,
                         int frame_number, int x_pixel, int y_pixel,
                         WSAType flags_and_prio, void *magic_cols, void *magic)
{
	SysAnimHeaderType 	*sys_header;		// fix up the void pointer past in.
	int 						curr_frame;			// current frame we are on.
	int						total_frames;		// number of frames in anim.
	int						distance;			// distance to desired frame.
	int						search_dir;			// direcion to search for desired frame.
	int						search_frames;		// How many frames to search.
	int						loop;					// Just a loop varible.
	char						*frame_buffer;		// our destination.
	BOOL						direct_to_dest;	// are we going directly to the destination?
	int						dest_width;			// the width of the destination buffer or page.

	// Assign local pointer to the beginning of the buffer where the system information
	// resides
	sys_header = (SysAnimHeaderType 	*)handle;

	// Get the total number of frames
	total_frames = sys_header->total_frames;

	// Are the animation handle and the frame number valid?
	if (!handle || (total_frames <= frame_number)) {
		return FALSE;
	}


	// Decide if we are going to a page or a viewport (part of a buffer).
	dest_width	= view.Get_Width() + view.Get_XAdd();

	//
	// adjust x_pixel and y_pixel by system pixel_x and pixel_y respectively.
	//
	x_pixel += (short)sys_header->pixel_x;
	y_pixel += (short)sys_header->pixel_y;

	//
	// Check to see if we are using a buffer inside of the animation buffer or if
	// it is being drawn directly to the destination page or buffer.
	//
	if (sys_header->flags & WSA_TARGET_IN_BUFFER) {
		// Get a pointer to the frame in animation buffer.
		frame_buffer = (char *)Add_Long_To_Pointer(sys_header, sizeof(SysAnimHeaderType));
		direct_to_dest = FALSE;
	}
	else {
		frame_buffer	= (char *)view.Get_Offset();
		frame_buffer  += (y_pixel * dest_width) + x_pixel;
		direct_to_dest	= TRUE;
	}
	//
	// If current_frame is equal to tatal_frames, then no animations have taken place
	// so must uncompress frame 0 in delta buffer to the frame_buffer/page if it
	// exists.
	//
	if (sys_header->current_frame == total_frames) {

		// Call apply delta telling it wether to copy or to xor depending on if the
		// target is a page or a buffer.

		if (!(sys_header->flags & WSA_FRAME_0_ON_PAGE)) {
			if (direct_to_dest) {

				// The last parameter says weather to copy or to XOR.  If  the
				// first frame is a DELTA, then it must be XOR'd.  A TRUE is
				// copy while FALSE is XOR.
				Apply_XOR_Delta_To_Page_Or_Viewport(frame_buffer, sys_header->delta_buffer,
				                        	sys_header->pixel_width, dest_width, //dest_width - sys_header->pixel_width,
													(sys_header->flags & WSA_FRAME_0_IS_DELTA)? DO_XOR : DO_COPY);
			}
			else {
				Apply_XOR_Delta(frame_buffer, sys_header->delta_buffer);
			}
		}
		sys_header->current_frame = 0;
	}

	//
	// Get the current frame
	// If no looping aloud, are the trying to do it anyways?
	//
	curr_frame = sys_header->current_frame;
#if (FALSE)
	// This is commented out since we will let them loop even though they should
	// not - it will be slower.
	if ( (sys_header->flags & WSA_LINEAR_ONLY) && (frame_number < cur_frame) ) {
	 	return FALSE;
	}
#endif

	// Get absoulte distance from our current frame to the target frame
	distance = ABS(curr_frame - frame_number);

	// Assume we are searching right
	search_dir = 1;

	// Calculate the number of frames to search if we go right and wrap

	if (frame_number > curr_frame) {
		search_frames = total_frames - frame_number + curr_frame;

		// Is going right faster than going backwards?
		// Or are they trying to loop when the should not?
		if ((search_frames < distance) && !(sys_header->flags & WSA_LINEAR_ONLY)) {
			search_dir = -1;								// No, so go left
		}
		else {
			search_frames = distance;
		}
	}
	else {
		search_frames = total_frames - curr_frame + frame_number;

		// Is going right faster than going backwards?
		// Or are they trying to loop when the should not?
		if ((search_frames >= distance) || (sys_header->flags & WSA_LINEAR_ONLY)) {
			search_dir = -1;								// No, so go left
			search_frames = distance;
		}
	}

	// Take care of the case when we are searching right (possibly right)

	if (search_dir > 0) {
		for (loop = 0; loop < search_frames; loop++) {

			// Move the logical frame number ordinally right
			curr_frame += search_dir;

			Apply_Delta(sys_header, curr_frame, frame_buffer, dest_width);

			// Adjust the current frame number, taking into consideration that we could
			// have wrapped

			if (curr_frame == total_frames) {
				curr_frame = 0;
			}
		}
	}
	else {
		for (loop = 0; loop < search_frames; loop++) {

			// If we are going backwards and we are on frame 0, the delta to get
			// to the last frame is the n + 1 delta (wrap delta)

			if (curr_frame == 0) {
				curr_frame = total_frames;
			}

			Apply_Delta(sys_header, curr_frame, frame_buffer, dest_width);

			curr_frame += search_dir;
		}
	}

	sys_header->current_frame = frame_number;

	// If we did this all in a hidden buffer, then copy it to the desired page or viewport.
	if (sys_header->flags & WSA_TARGET_IN_BUFFER) {
#if TRUE

		Buffer_To_Page(x_pixel, y_pixel, sys_header->pixel_width, sys_header->pixel_height, frame_buffer, view);

#else
		int	flags = ((unsigned short)flags_and_prio & 0xFF00u) >> 12u;
		int	pri = flags_and_prio & 0x00FF;

		Buffer_Bitblit_To_LogicPage(x_pixel, y_pixel, sys_header->pixel_width,
										  sys_header->pixel_height, 0, flags, frame_buffer, pri,
										  magic_cols, magic);
#endif
	}


	return TRUE;
}
#endif


/***************************************************************************
 * ANIMATE_FRAME_COUNT -- Return Number of frames in an animation.         *
 *                                                                         *
 * INPUT:   void *handle to the animation.                                 *
 *                                                                         *
 * OUTPUT:  int number of frames in animation.                            *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/05/1991  SB : Created.                                             *
 *=========================================================================*/
int __cdecl Get_Animation_Frame_Count(void *handle)
{
	SysAnimHeaderType *sys_header;

	if (!handle) {
		return FALSE;
	}
	sys_header = (SysAnimHeaderType *) handle;
	return((short)sys_header->total_frames);
}


/***************************************************************************
 * GET_ANIM_X -- Gets the x from an animation                              *
 *                                                                         *
 *                                                                         *
 * INPUT:		void * to the animation that we are processing              *
 *                                                                         *
 * OUTPUT:     int the x of the animation we are processing				   *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   07/03/1992 DRD : Created.                                             *
 *=========================================================================*/
int __cdecl Get_Animation_X(void const *handle)
{
 	SysAnimHeaderType const *sys_header;

	if (!handle) {
		return FALSE;
	}
	sys_header = (SysAnimHeaderType *) handle;
	return(sys_header->pixel_x);
}

/***************************************************************************
 * GET_ANIM_Y -- Gets the y from an animation                              *
 *                                                                         *
 *                                                                         *
 * INPUT:		void * to the animation that we are processing              *
 *                                                                         *
 * OUTPUT:     int the y of the animation we are processing				   *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   10/14/1992 PWG : Created.                                             *
 *=========================================================================*/
int __cdecl Get_Animation_Y(void const *handle)
{
 	SysAnimHeaderType const *sys_header;

	if (!handle) {
		return FALSE;
	}
	sys_header = (SysAnimHeaderType *) handle;
	return(sys_header->pixel_y);
}

/***************************************************************************
 * GET_ANIM_WIDTH -- Gets the width from an animation                      *
 *                                                                         *
 *                                                                         *
 * INPUT:		void * to the animation that we are processing              *
 *                                                                         *
 * OUTPUT:     int the width of the animation we are processing				*
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   10/14/1992 PWG : Created.                                             *
 *=========================================================================*/
int __cdecl Get_Animation_Width(void const *handle)
{
 	SysAnimHeaderType const *sys_header;

	if (!handle) {
		return FALSE;
	}
	sys_header = (SysAnimHeaderType *) handle;
	return(sys_header->pixel_width);
}

/***************************************************************************
 * GET_ANIM_HEIGHT -- The height of the animation we are processing        *
 *                                                                         *
 * INPUT:		void * to the animation that we are processing              *
 *                                                                         *
 * OUTPUT:     int the height of the animation we are processing				*
 *                                                                         *
 * HISTORY:                                                                *
 *   10/14/1992 PWG : Created.                                             *
 *=========================================================================*/
int __cdecl Get_Animation_Height(void const *handle)
{
 	SysAnimHeaderType const *sys_header;

	if (!handle) {
		return FALSE;
	}
	sys_header = (SysAnimHeaderType *) handle;
	return(sys_header->pixel_height);
}


/***************************************************************************
 * GET_ANIM_PALETTE -- Returns true if the anim had an attached palette    *
 *                                                                         *
 * INPUT:		void * to the animation that we are processing              *
 *                                                                         *
 * OUTPUT:		int True if the animation has a set palette.  False if the	*
 *					animation does not.														*
 *                                                                         *
 * HISTORY:                                                                *
 *   10/14/1992 PWG : Created.                                             *
 *=========================================================================*/
int __cdecl Get_Animation_Palette(void const *handle)
{
 	SysAnimHeaderType const *sys_header;

	if (!handle) {
		return FALSE;
	}
	sys_header = (SysAnimHeaderType *) handle;
	return(sys_header->flags & WSA_PALETTE_PRESENT);
}


/***************************************************************************
 * GET_ANIMATION_SIZE -- Return the amount of memory the animation is using*
 *                                                                         *
 *                                                                         *
 * INPUT:		void * to the animation that we are processing              *
 *                                                                         *
 * OUTPUT:		unsigned long number of byte used by animation. 							*
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/23/1994 SKB : Created.                                             *
 *=========================================================================*/
unsigned long __cdecl Get_Animation_Size(void const *handle)
{
 	SysAnimHeaderType const *sys_header;

	if (!handle) {
		return FALSE;
	}
	sys_header = (SysAnimHeaderType *) handle;
	return(sys_header->anim_mem_size);
}

/* ::::::::::::::::::::::::::::  PRIVATE FUNCTIONS  :::::::::::::::::::::::::::::: */


/***************************************************************************
 * GET_RESIDENT_FRAME_OFFSET -- Gets frame offset of animate file in RAM   *
 *                                                                         *
 * INPUT:      char *file_buffer in RAM of animation file.                 *
 *             int frame number that we need the offset of.               *
 *                                                                         *
 * OUTPUT:     int offset of frame requested.                             *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   11/26/1991  SB : Created.                                             *
 *=========================================================================*/
PRIVATE unsigned long Get_Resident_Frame_Offset( char *file_buffer, int frame )
{
	return WSA_Resident_Frame_Offset(file_buffer, frame);
}


/***************************************************************************
 * GET_FILE_FRAME_OFFSET -- Get offset of a delta frame from animate file. *
 *                                                                         *
 * INPUT:      int file_handle of animation file.                         *
 *             int frame number that we need the offset of.               *
 *                                                                         *
 * OUTPUT:     int offset of frame requested.                             *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   11/26/1991  SB : Created.                                             *
 *=========================================================================*/
PRIVATE unsigned long Get_File_Frame_Offset( int file_handle, int frame, int palette_adjust)
{
	uint32_t offset;

	Seek_File(file_handle, (frame * WSA_FILE_OFFSET_SIZE) + WSA_FILE_HEADER_SIZE, SEEK_SET);

	if (Read_File(file_handle, (char *) &offset, WSA_FILE_OFFSET_SIZE) != WSA_FILE_OFFSET_SIZE) {
		offset = 0L;
	}
	offset += palette_adjust;
	return( offset );
}


/***************************************************************************
 * APPLY_DELTA -- Copies frame into delta buffer, then applies to target   *
 *                                                                         *
 * INPUT:      SysAnimHeaderType *sys_header - pointer to animation buffer.*
 *             int curr_frame - frame to put into target buffer.          *
 *																									*
 * OUTPUT:     BOOL - Return wether or not it worked.                      *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   11/26/1991  SB : Created.                                             *
 *=========================================================================*/
PRIVATE BOOL Apply_Delta(SysAnimHeaderType *sys_header, int curr_frame, char *dest_ptr, int dest_w)
{
	char *data_ptr, *delta_back;
	int file_handle, palette_adjust;
	unsigned long frame_data_size, frame_offset;


	palette_adjust = ((sys_header->flags & WSA_PALETTE_PRESENT) ? 768 : 0);
	delta_back		= sys_header->delta_buffer;

	if (sys_header->flags & WSA_RESIDENT) {
		// Get offset of the given frame in the resident file
		// Get the size of the frame <- (frame+1 offset) - (offset)
		// Point at the delta data
		// figure offset to load data into end of delta buffer
		// copy it into buffer

		frame_offset = Get_Resident_Frame_Offset(sys_header->file_buffer, curr_frame);
		frame_data_size = Get_Resident_Frame_Offset(sys_header->file_buffer, curr_frame + 1) -	frame_offset;

	  	data_ptr 	= (char *)Add_Long_To_Pointer(sys_header->file_buffer, frame_offset);
	  	delta_back 	= (char *)Add_Long_To_Pointer(delta_back,
	  						 sys_header->largest_frame_size - frame_data_size);

		Mem_Copy( data_ptr, delta_back, frame_data_size );

	} else if (sys_header -> flags & WSA_FILE) {

		//	Open up file because not file not in RAM.
		// Get offset of the given frame in the file on disk
		// Get the size of the frame <- (frame+1 offset) - (offset)
		// Return if Get_.._offset() failed.  -- need error handling????
		//	Seek to delta data.
		// figure offset to load data into end of delta buffer
		//	Read it into buffer -- Return if correct amount not read.-- errors??

		file_handle  = sys_header->file_handle;
		Seek_File(file_handle, 0L, SEEK_SET);

		frame_offset = Get_File_Frame_Offset(file_handle, curr_frame, palette_adjust);
		frame_data_size = Get_File_Frame_Offset(file_handle, curr_frame + 1, palette_adjust) - frame_offset;

		if (!frame_offset || !frame_data_size) {
			return(FALSE);
		}

		Seek_File(file_handle, frame_offset, SEEK_SET);
		delta_back 	= (char *)Add_Long_To_Pointer(delta_back, sys_header->largest_frame_size - frame_data_size);

		if (Read_File(file_handle, delta_back, frame_data_size) != frame_data_size) {
			return(FALSE);
		}
	}

	// Uncompress data at end of delta buffer to the beginning of delta buffer.
	// Find start of target buffer.
	// Apply the XOR delta.

	LCW_Uncompress(delta_back, sys_header->delta_buffer, sys_header->largest_frame_size);

	if (sys_header->flags & WSA_TARGET_IN_BUFFER) {
		Apply_XOR_Delta(dest_ptr, sys_header->delta_buffer);
	}
	else {
		Apply_XOR_Delta_To_Page_Or_Viewport(dest_ptr, sys_header->delta_buffer,
		                        sys_header->pixel_width,  dest_w, DO_XOR);
	}

	return(TRUE);
}
