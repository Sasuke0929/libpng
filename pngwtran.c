
/* pngwtran.c - transforms the data in a row for png writers

   libpng 1.0 beta 1 - version 0.71
   For conditions of distribution and use, see copyright notice in png.h
   Copyright (c) 1995 Guy Eric Schalnat, Group 42, Inc.
   June 26, 1995
   */

#define PNG_INTERNAL
#include "png.h"

/* transform the data according to the users wishes.  The order of
   transformations is significant. */
void
png_do_write_transformations(png_struct *png_ptr)
{
   if (png_ptr->transformations & PNG_RGBA)
      png_do_write_rgbx(&(png_ptr->row_info), png_ptr->row_buf + 1);
   if (png_ptr->transformations & PNG_XRGB)
      png_do_write_xrgb(&(png_ptr->row_info), png_ptr->row_buf + 1);
   if (png_ptr->transformations & PNG_PACK)
      png_do_pack(&(png_ptr->row_info), png_ptr->row_buf + 1,
         png_ptr->bit_depth);
   if (png_ptr->transformations & PNG_SHIFT)
      png_do_shift(&(png_ptr->row_info), png_ptr->row_buf + 1,
         &(png_ptr->shift));
   if (png_ptr->transformations & PNG_SWAP_BYTES)
      png_do_swap(&(png_ptr->row_info), png_ptr->row_buf + 1);
   if (png_ptr->transformations & PNG_BGR)
      png_do_bgr(&(png_ptr->row_info), png_ptr->row_buf + 1);
   if (png_ptr->transformations & PNG_INVERT_MONO)
      png_do_invert(&(png_ptr->row_info), png_ptr->row_buf + 1);
}

/* pack pixels into bytes.  Pass the true bit depth in bit_depth.  The
   row_info bit depth should be 8 (one pixel per byte).  The channels
   should be 1 (this only happens on grayscale and paletted images) */
void
png_do_pack(png_row_info *row_info, png_byte *row, png_byte bit_depth)
{
   if (row_info && row && row_info->bit_depth == 8 &&
      row_info->channels == 1)
   {
      switch (bit_depth)
      {
         case 1:
         {
            png_byte *sp;
            png_byte *dp;
            int mask;
            png_int_32 i;
            int v;

            sp = row;
            dp = row;
            mask = 0x80;
            v = 0;
            for (i = 0; i < row_info->width; i++)
            {
               if (*sp)
                  v |= mask;
               sp++;
               if (mask > 1)
                  mask >>= 1;
               else
               {
                  mask = 0x80;
                  *dp = v;
                  dp++;
                  v = 0;
               }
            }
            if (mask != 0x80)
               *dp = v;
            break;
         }
         case 2:
         {
            png_byte *sp;
            png_byte *dp;
            int shift;
            png_int_32 i;
            int v;
            png_byte value;

            sp = row;
            dp = row;
            shift = 6;
            v = 0;
            for (i = 0; i < row_info->width; i++)
            {
               value = *sp & 0x3;
               v |= (value << shift);
               if (shift == 0)
               {
                  shift = 6;
                  *dp = v;
                  dp++;
                  v = 0;
               }
               else
                  shift -= 2;
               sp++;
            }
            if (shift != 6)
                   *dp = v;
            break;
         }
         case 4:
         {
            png_byte *sp;
            png_byte *dp;
            int shift;
            png_int_32 i;
            int v;
            png_byte value;

            sp = row;
            dp = row;
            shift = 4;
            v = 0;
            for (i = 0; i < row_info->width; i++)
            {
               value = *sp & 0xf;
               v |= (value << shift);

               if (shift == 0)
               {
                  shift = 4;
                  *dp = v;
                  dp++;
                  v = 0;
               }
               else
                  shift -= 4;

               sp++;
            }
            if (shift != 4)
               *dp = v;
            break;
         }
      }
      row_info->bit_depth = bit_depth;
      row_info->pixel_depth = bit_depth * row_info->channels;
      row_info->rowbytes =
         ((row_info->width * row_info->pixel_depth + 7) >> 3);
   }
}

/* shift pixel values to take advantage of whole range.  Pass the
   true number of bits in bit_depth.  The row should be packed
   according to row_info->bit_depth.  Thus, if you had a row of
   bit depth 4, but the pixels only had values from 0 to 7, you
   would pass 3 as bit_depth, and this routine would translate the
   data to 0 to 15. */
void
png_do_shift(png_row_info *row_info, png_byte *row, png_color_8 *bit_depth)
{
   if (row && row_info &&
      row_info->color_type != PNG_COLOR_TYPE_PALETTE)
   {
      int shift_start[4], shift_dec[4];
      int channels;

      channels = 0;
      if (row_info->color_type & PNG_COLOR_MASK_COLOR)
      {
         shift_start[channels] = row_info->bit_depth - bit_depth->red;
         shift_dec[channels] = bit_depth->red;
         channels++;
         shift_start[channels] = row_info->bit_depth - bit_depth->green;
         shift_dec[channels] = bit_depth->green;
         channels++;
         shift_start[channels] = row_info->bit_depth - bit_depth->blue;
         shift_dec[channels] = bit_depth->blue;
         channels++;
      }
      else
      {
         shift_start[channels] = row_info->bit_depth - bit_depth->gray;
         shift_dec[channels] = bit_depth->gray;
         channels++;
      }
      if (row_info->color_type & PNG_COLOR_MASK_ALPHA)
      {
         shift_start[channels] = row_info->bit_depth - bit_depth->alpha;
         shift_dec[channels] = bit_depth->alpha;
         channels++;
      }

      /* with low row dephts, could only be grayscale, so one channel */
      if (row_info->bit_depth < 8)
      {
         png_byte *bp;
         png_uint_32 i;
         int j;
         png_byte mask;

         if (bit_depth->gray == 1 && row_info->bit_depth == 2)
            mask = 0x55;
         else if (row_info->bit_depth == 4 && bit_depth->gray == 3)
            mask = 0x11;
         else
            mask = 0xff;

         for (bp = row, i = 0; i < row_info->rowbytes; i++, bp++)
         {
            int v;

            v = *bp;
            *bp = 0;
            for (j = shift_start[0]; j > -shift_dec[0]; j -= shift_dec[0])
            {
               if (j > 0)
                  *bp |= (png_byte)((v << j) & 0xff);
               else
                  *bp |= (png_byte)((v >> (-j)) & mask);
            }
         }
      }
      else if (row_info->bit_depth == 8)
      {
         png_byte *bp;
         png_uint_32 i;
         int j;

         for (bp = row, i = 0; i < row_info->width; i++)
         {
            int c;

            for (c = 0; c < channels; c++, bp++)
            {
               int v;

               v = *bp;
               *bp = 0;
               for (j = shift_start[c]; j > -shift_dec[c]; j -= shift_dec[c])
               {
                  if (j > 0)
                     *bp |= (png_byte)((v << j) & 0xff);
                  else
                     *bp |= (png_byte)((v >> (-j)) & 0xff);
               }
            }
         }
      }
      else
      {
         png_byte *bp;
         png_uint_32 i;
         int j;

         for (bp = row, i = 0;
            i < row_info->width * row_info->channels;
            i++)
         {
            int c;

            for (c = 0; c < channels; c++, bp += 2)
            {
               png_uint_16 value, v;

               v = ((png_uint_16)(*bp) << 8) + (png_uint_16)(*(bp + 1));
               value = 0;
               for (j = shift_start[c]; j > -shift_dec[c]; j -= shift_dec[c])
               {
                  if (j > 0)
                     value |= (png_uint_16)((v << j) & (png_uint_16)0xffff);
                  else
                     value |= (png_uint_16)((v >> (-j)) & (png_uint_16)0xffff);
               }
               *bp = value >> 8;
               *(bp + 1) = value & 0xff;
            }
         }
      }
   }
}

/* remove filler byte after rgb */
void
png_do_write_rgbx(png_row_info *row_info, png_byte *row)
{
   if (row && row_info && row_info->color_type == PNG_COLOR_TYPE_RGB &&
      row_info->bit_depth == 8)
   {
      png_byte *sp, *dp;
      png_uint_32 i;

      for (i = 1, sp = row + 4, dp = row + 3;
         i < row_info->width;
         i++)
      {
         *dp++ = *sp++;
         *dp++ = *sp++;
         *dp++ = *sp++;
         sp++;
      }
      row_info->channels = 3;
      row_info->pixel_depth = 24;
      row_info->rowbytes = row_info->width * 3;
   }
}

/* remove filler byte before rgb */
void
png_do_write_xrgb(png_row_info *row_info, png_byte *row)
{
   if (row && row_info && row_info->color_type == PNG_COLOR_TYPE_RGB &&
      row_info->bit_depth == 8)
   {
      png_byte *sp, *dp;
      png_uint_32 i;

      for (i = 0, sp = row, dp = row;
         i < row_info->width;
         i++)
      {
         sp++;
         *dp++ = *sp++;
         *dp++ = *sp++;
         *dp++ = *sp++;
      }
      row_info->channels = 3;
      row_info->pixel_depth = 24;
      row_info->rowbytes = row_info->width * 3;
   }
}

