
/* png.c - location for general purpose png functions

   libpng 1.0 beta 2 - version 0.81
   For conditions of distribution and use, see copyright notice in png.h
   Copyright (c) 1995 Guy Eric Schalnat, Group 42, Inc.
   August 24, 1995
   */

#define PNG_INTERNAL
#define PNG_NO_EXTERN
#include "png.h"

/* version information for c files.  This better match the version
   string defined in png.h */
char png_libpng_ver[] = "0.81";

/* place to hold the signiture string for a png file. */
png_byte png_sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};

/* constant strings for known chunk types.  If you need to add a chunk,
   add a string holding the name here.  If you want to make the code
   portable to EBCDIC machines, use ASCII numbers, not characters. */
png_byte png_IHDR[4] = { 73,  72,  68,  82};
png_byte png_IDAT[4] = { 73,  68,  65,  84};
png_byte png_IEND[4] = { 73,  69,  78,  68};
png_byte png_PLTE[4] = { 80,  76,  84,  69};
#if defined(PNG_READ_gAMA_SUPPORTED) || defined(PNG_WRITE_gAMA_SUPPORTED)
png_byte png_gAMA[4] = {103,  65,  77,  65};
#endif
#if defined(PNG_READ_sBIT_SUPPORTED) || defined(PNG_WRITE_sBIT_SUPPORTED)
png_byte png_sBIT[4] = {115,  66,  73,  84};
#endif
#if defined(PNG_READ_cHRM_SUPPORTED) || defined(PNG_WRITE_cHRM_SUPPORTED)
png_byte png_cHRM[4] = { 99,  72,  82,  77};
#endif
#if defined(PNG_READ_tRNS_SUPPORTED) || defined(PNG_WRITE_tRNS_SUPPORTED)
png_byte png_tRNS[4] = {116,  82,  78,  83};
#endif
#if defined(PNG_READ_bKGD_SUPPORTED) || defined(PNG_WRITE_bKGD_SUPPORTED)
png_byte png_bKGD[4] = { 98,  75,  71,  68};
#endif
#if defined(PNG_READ_hIST_SUPPORTED) || defined(PNG_WRITE_hIST_SUPPORTED)
png_byte png_hIST[4] = {104,  73,  83,  84};
#endif
#if defined(PNG_READ_tEXt_SUPPORTED) || defined(PNG_WRITE_tEXt_SUPPORTED)
png_byte png_tEXt[4] = {116,  69,  88, 116};
#endif
#if defined(PNG_READ_zTXt_SUPPORTED) || defined(PNG_WRITE_zTXt_SUPPORTED)
png_byte png_zTXt[4] = {122,  84,  88, 116};
#endif
#if defined(PNG_READ_pHYs_SUPPORTED) || defined(PNG_WRITE_pHYs_SUPPORTED)
png_byte png_pHYs[4] = {112,  72,  89, 115};
#endif
#if defined(PNG_READ_oFFs_SUPPORTED) || defined(PNG_WRITE_oFFs_SUPPORTED)
png_byte png_oFFs[4] = {111,  70,  70, 115};
#endif
#if defined(PNG_READ_tIME_SUPPORTED) || defined(PNG_WRITE_tIME_SUPPORTED)
png_byte png_tIME[4] = {116,  73,  77,  69};
#endif

/* arrays to facilitate easy interlacing - use pass (0 - 6) as index */

/* start of interlace block */
int png_pass_start[] = {0, 4, 0, 2, 0, 1, 0};

/* offset to next interlace block */
int png_pass_inc[] = {8, 8, 4, 4, 2, 2, 1};

/* start of interlace block in the y direction */
int png_pass_ystart[] = {0, 0, 4, 0, 2, 0, 1};

/* offset to next interlace block in the y direction */
int png_pass_yinc[] = {8, 8, 8, 4, 4, 2, 2};

/* width of interlace block */
/* this is not currently used - if you need it, uncomment it here and
   in png.h
int png_pass_width[] = {8, 4, 4, 2, 2, 1, 1};
*/

/* height of interlace block */
/* this is not currently used - if you need it, uncomment it here and
   in png.h
int png_pass_height[] = {8, 8, 4, 4, 4, 2, 2, 1};
*/

/* mask to determine which pixels are valid in a pass */
int png_pass_mask[] = {0x80, 0x08, 0x88, 0x22, 0xaa, 0x55, 0xff};

/* mask to determine which pixels to overwrite while displaying */
int png_pass_dsp_mask[] = {0xff, 0x0f, 0xff, 0x33, 0xff, 0x55, 0xff};


int
png_check_sig(png_byte *sig, int num)
{
   if (num > 8)
      num = 8;
   if (num < 1)
      return 0;

   return (!memcmp(sig, png_sig, num));
}

/* Function to allocate memory for zlib. */
voidpf
png_zalloc(voidpf png_ptr, uInt items, uInt size)
{
   return ((voidpf)png_large_malloc((png_struct FAR *)png_ptr,
      (png_uint_32)items * (png_uint_32)size));
}

/* function to free memory for zlib */
void
png_zfree(voidpf png_ptr, voidpf ptr)
{
   png_large_free((png_struct FAR *)png_ptr, (voidpf)ptr);
}

/* reset the crc variable to 32 bits of 1's.  Care must be taken
   in case crc is > 32 bits to leave the top bits 0 */
void
png_reset_crc(png_struct *png_ptr)
{
   /* set crc to all 1's */
   png_ptr->crc = 0xffffffffL;
}

/* Note: the crc code below was copied from the sample code in the
   PNG spec, with appropriate modifications made to ensure the
   variables are large enough */

/* table of crc's of all 8-bit messages.  If you wish to png_malloc this
   table, turn this into a pointer, and png_malloc it in make_crc_table().
   You may then want to hook it into png_struct and free it with the
   destroy functions. */
static png_uint_32 crc_table[256];

/* Flag: has the table been computed? Initially false. */
static int crc_table_computed = 0;

/* make the table for a fast crc */
static void
make_crc_table(void)
{
  png_uint_32 c;
  int n, k;

  for (n = 0; n < 256; n++)
  {
   c = (png_uint_32)n;
   for (k = 0; k < 8; k++)
     c = c & 1 ? 0xedb88320L ^ (c >> 1) : c >> 1;
   crc_table[n] = c;
  }
  crc_table_computed = 1;
}

/* update a running crc with the bytes buf[0..len-1]--the crc should be
   initialized to all 1's, and the transmitted value is the 1's complement
   of the final running crc. */
static png_uint_32
update_crc(png_uint_32 crc, png_bytef *buf, png_uint_32 len)
{
  png_uint_32 c;
  png_bytef *p;
  png_uint_32 n;

  c = crc;
  p = buf;
  n = len;

  if (!crc_table_computed)
  {
   make_crc_table();
  }

  if (n > 0) do
  {
   c = crc_table[(png_byte)((c ^ (*p++)) & 0xff)] ^ (c >> 8);
  } while (--n);

  return c;
}

/* calculate the crc over a section of data.  Note that while we
   are passing in a 32 bit value for length, on 16 bit machines, you
   would need to use huge pointers to access all that data.  If you
   need this, put huge here and above. */
void
png_calculate_crc(png_struct *png_ptr, png_bytef *ptr,
   png_uint_32 length)
{
   png_ptr->crc = update_crc(png_ptr->crc, ptr, length);
}
void
png_info_init(png_info *info)
{
   /* set everything to 0 */
   memset(info, 0, sizeof (png_info));
}

