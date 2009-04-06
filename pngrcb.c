/* pngrcb.c - callbacks while reading a png file

   libpng 1.0 beta 2 - version 0.81
   For conditions of distribution and use, see copyright notice in png.h
   Copyright (c) 1995 Guy Eric Schalnat, Group 42, Inc.
   August 24, 1995
   */

#define PNG_INTERNAL
#include "png.h"

void
png_read_IHDR(png_struct *png_ptr, png_info *info,
   png_uint_32 width, png_uint_32 height, int bit_depth,
   int color_type, int compression_type, int filter_type,
   int interlace_type)
{
   if (!png_ptr || !info)
      return;

   info->width = width;
   info->height = height;
   info->bit_depth = bit_depth;
   info->color_type = color_type;
   info->compression_type = compression_type;
   info->filter_type = filter_type;
   info->interlace_type = interlace_type;
   if (info->color_type == PNG_COLOR_TYPE_PALETTE)
      info->channels = 1;
   else if (info->color_type & PNG_COLOR_MASK_COLOR)
      info->channels = 3;
   else
      info->channels = 1;
   if (info->color_type & PNG_COLOR_MASK_ALPHA)
      info->channels++;
   info->pixel_depth = info->channels * info->bit_depth;
   info->rowbytes = ((info->width * info->pixel_depth + 7) >> 3);
}

void
png_read_PLTE(png_struct *png_ptr, png_info *info,
   png_color *palette, int num)
{
   if (!png_ptr || !info)
      return;

   info->palette = palette;
   info->num_palette = num;
   info->valid |= PNG_INFO_PLTE;
}

#if defined(PNG_READ_gAMA_SUPPORTED)
void
png_read_gAMA(png_struct *png_ptr, png_info *info, float gamma)
{
   if (!png_ptr || !info)
      return;

   info->gamma = gamma;
   info->valid |= PNG_INFO_gAMA;
}
#endif

#if defined(PNG_READ_sBIT_SUPPORTED)
void
png_read_sBIT(png_struct *png_ptr, png_info *info,
   png_color_8 *sig_bit)
{
   if (!png_ptr || !info)
      return;

   memcpy(&(info->sig_bit), sig_bit, sizeof (png_color_8));
   info->valid |= PNG_INFO_sBIT;
}
#endif

#if defined(PNG_READ_cHRM_SUPPORTED)
void
png_read_cHRM(png_struct *png_ptr, png_info *info,
   float white_x, float white_y, float red_x, float red_y,
   float green_x, float green_y, float blue_x, float blue_y)
{
   if (!png_ptr || !info)
      return;

   info->x_white = white_x;
   info->y_white = white_y;
   info->x_red = red_x;
   info->y_red = red_y;
   info->x_green = green_x;
   info->y_green = green_y;
   info->x_blue = blue_x;
   info->y_blue = blue_y;
   info->valid |= PNG_INFO_cHRM;
}
#endif

#if defined(PNG_READ_tRNS_SUPPORTED)
void
png_read_tRNS(png_struct *png_ptr, png_info *info,
   png_byte *trans, int num_trans,   png_color_16 *trans_values)
{
   if (!png_ptr || !info)
      return;

   if (trans)
   {
      info->trans = trans;
   }
   else
   {
      memcpy(&(info->trans_values), trans_values,
         sizeof(png_color_16));
   }
   info->num_trans = num_trans;
   info->valid |= PNG_INFO_tRNS;
}
#endif

#if defined(PNG_READ_bKGD_SUPPORTED)
void
png_read_bKGD(png_struct *png_ptr, png_info *info,
   png_color_16 *background)
{
   if (!png_ptr || !info)
      return;

   memcpy(&(info->background), background, sizeof(png_color_16));
   info->valid |= PNG_INFO_bKGD;
}
#endif

#if defined(PNG_READ_hIST_SUPPORTED)
void
png_read_hIST(png_struct *png_ptr, png_info *info, png_uint_16 *hist)
{
   if (!png_ptr || !info)
      return;

   info->hist = hist;
   info->valid |= PNG_INFO_hIST;
}
#endif

#if defined(PNG_READ_pHYs_SUPPORTED)
void
png_read_pHYs(png_struct *png_ptr, png_info *info,
   png_uint_32 res_x, png_uint_32 res_y, int unit_type)
{
   if (!png_ptr || !info)
      return;

   info->x_pixels_per_unit = res_x;
   info->y_pixels_per_unit = res_y;
   info->phys_unit_type = unit_type;
   info->valid |= PNG_INFO_pHYs;
}
#endif

#if defined(PNG_READ_oFFs_SUPPORTED)
void
png_read_oFFs(png_struct *png_ptr, png_info *info,
   png_uint_32 offset_x, png_uint_32 offset_y, int unit_type)
{
   if (!png_ptr || !info)
      return;

   info->x_offset = offset_x;
   info->y_offset = offset_y;
   info->offset_unit_type = unit_type;
   info->valid |= PNG_INFO_oFFs;
}
#endif

#if defined(PNG_READ_tIME_SUPPORTED)
void
png_read_tIME(png_struct *png_ptr, png_info *info,
   png_time *mod_time)
{
   if (!png_ptr || !info)
      return;

   memcpy(&(info->mod_time), mod_time, sizeof (png_time));
   info->valid |= PNG_INFO_tIME;
}
#endif

#if defined(PNG_READ_zTXt_SUPPORTED)
void
png_read_zTXt(png_struct *png_ptr, png_info *info,
   charf *key, charf *text, png_uint_32 text_len, int compression)
{
   if (!png_ptr || !info)
      return;

   if (info->max_text <= info->num_text)
   {
      if (info->text)
      {
         png_uint_32 old_max;

         old_max = info->max_text;
         info->max_text = info->num_text + 16;
         info->text = (png_text *)png_realloc(png_ptr,
            info->text,
            info->max_text * sizeof (png_text),
            old_max * sizeof (png_text));
      }
      else
      {
         info->max_text = info->num_text + 16;
         info->text = (png_text *)png_malloc(png_ptr,
            info->max_text * sizeof (png_text));
         info->num_text = 0;
      }
   }

   info->text[info->num_text].key = key;
   info->text[info->num_text].text = text;
   info->text[info->num_text].text_length = text_len;
   info->text[info->num_text].compression = compression;
   info->num_text++;
}
#endif

#if defined(PNG_READ_tEXt_SUPPORTED)
void
png_read_tEXt(png_struct *png_ptr, png_info *info,
   charf *key, charf *text, png_uint_32 text_len)
{
   if (!png_ptr || !info)
      return;

   png_read_zTXt(png_ptr, info, key, text, text_len, -1);
}
#endif

