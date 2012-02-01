
/* png.c - location for general purpose libpng functions
 *
 * Last changed in libpng 1.6.0 [(PENDING RELEASE)]
 * Copyright (c) 1998-2012 Glenn Randers-Pehrson
 * (Version 0.96 Copyright (c) 1996, 1997 Andreas Dilger)
 * (Version 0.88 Copyright (c) 1995, 1996 Guy Eric Schalnat, Group 42, Inc.)
 *
 * This code is released under the libpng license.
 * For conditions of distribution and use, see the disclaimer
 * and license in png.h
 */

#include "pngpriv.h"

/* Generate a compiler error if there is an old png.h in the search path. */
typedef png_libpng_version_1_6_0beta08 Your_png_h_is_not_version_1_6_0beta08;

/* Tells libpng that we have already handled the first "num_bytes" bytes
 * of the PNG file signature.  If the PNG data is embedded into another
 * stream we can set num_bytes = 8 so that libpng will not attempt to read
 * or write any of the magic bytes before it starts on the IHDR.
 */

#ifdef PNG_READ_SUPPORTED
void PNGAPI
png_set_sig_bytes(png_structrp png_ptr, int num_bytes)
{
   png_debug(1, "in png_set_sig_bytes");

   if (png_ptr == NULL)
      return;

   if (num_bytes > 8)
      png_error(png_ptr, "Too many bytes for PNG signature");

   png_ptr->sig_bytes = (png_byte)(num_bytes < 0 ? 0 : num_bytes);
}

/* Checks whether the supplied bytes match the PNG signature.  We allow
 * checking less than the full 8-byte signature so that those apps that
 * already read the first few bytes of a file to determine the file type
 * can simply check the remaining bytes for extra assurance.  Returns
 * an integer less than, equal to, or greater than zero if sig is found,
 * respectively, to be less than, to match, or be greater than the correct
 * PNG signature (this is the same behavior as strcmp, memcmp, etc).
 */
int PNGAPI
png_sig_cmp(png_const_bytep sig, png_size_t start, png_size_t num_to_check)
{
   png_byte png_signature[8] = {137, 80, 78, 71, 13, 10, 26, 10};

   if (num_to_check > 8)
      num_to_check = 8;

   else if (num_to_check < 1)
      return (-1);

   if (start > 7)
      return (-1);

   if (start + num_to_check > 8)
      num_to_check = 8 - start;

   return ((int)(png_memcmp(&sig[start], &png_signature[start], num_to_check)));
}

#endif /* PNG_READ_SUPPORTED */

#if defined(PNG_READ_SUPPORTED) || defined(PNG_WRITE_SUPPORTED)
/* Function to allocate memory for zlib */
PNG_FUNCTION(voidpf /* PRIVATE */,
png_zalloc,(voidpf png_ptr, uInt items, uInt size),PNG_ALLOCATED)
{
   png_alloc_size_t num_bytes = size;

   if (png_ptr == NULL)
      return NULL;

   if (items >= (~(png_alloc_size_t)0)/size)
   {
      png_warning (png_voidcast(png_structrp, png_ptr),
         "Potential overflow in png_zalloc()");
      return NULL;
   }

   num_bytes *= items;
   return png_malloc_warn(png_voidcast(png_structrp, png_ptr), num_bytes);
}

/* Function to free memory for zlib */
void /* PRIVATE */
png_zfree(voidpf png_ptr, voidpf ptr)
{
   png_free(png_voidcast(png_const_structrp,png_ptr), ptr);
}

/* Reset the CRC variable to 32 bits of 1's.  Care must be taken
 * in case CRC is > 32 bits to leave the top bits 0.
 */
void /* PRIVATE */
png_reset_crc(png_structrp png_ptr)
{
   /* The cast is safe because the crc is a 32 bit value. */
   png_ptr->crc = (png_uint_32)crc32(0, Z_NULL, 0);
}

/* Calculate the CRC over a section of data.  We can only pass as
 * much data to this routine as the largest single buffer size.  We
 * also check that this data will actually be used before going to the
 * trouble of calculating it.
 */
void /* PRIVATE */
png_calculate_crc(png_structrp png_ptr, png_const_bytep ptr, png_size_t length)
{
   int need_crc = 1;

   if (PNG_CHUNK_ANCILLIARY(png_ptr->chunk_name))
   {
      if ((png_ptr->flags & PNG_FLAG_CRC_ANCILLARY_MASK) ==
          (PNG_FLAG_CRC_ANCILLARY_USE | PNG_FLAG_CRC_ANCILLARY_NOWARN))
         need_crc = 0;
   }

   else /* critical */
   {
      if (png_ptr->flags & PNG_FLAG_CRC_CRITICAL_IGNORE)
         need_crc = 0;
   }

   /* 'uLong' is defined as unsigned long, this means that on some systems it is
    * a 64 bit value.  crc32, however, returns 32 bits so the following cast is
    * safe.  'uInt' may be no more than 16 bits, so it is necessary to perform a
    * loop here.
    */
   if (need_crc && length > 0)
   {
      uLong crc = png_ptr->crc; /* Should never issue a warning */

      do
      {
         uInt safeLength = (uInt)length;
         if (safeLength == 0)
            safeLength = (uInt)-1; /* evil, but safe */

         crc = crc32(crc, ptr, safeLength);

         /* The following should never issue compiler warnings, if they do the
          * target system has characteristics that will probably violate other
          * assumptions within the libpng code.
          */
         ptr += safeLength;
         length -= safeLength;
      }
      while (length > 0);

      /* And the following is always safe because the crc is only 32 bits. */
      png_ptr->crc = (png_uint_32)crc;
   }
}

/* Check a user supplied version number, called from both read and write
 * functions that create a png_struct
 */
int
png_user_version_check(png_structrp png_ptr, png_const_charp user_png_ver)
{
   if (user_png_ver)
   {
      int i = 0;

      do
      {
         if (user_png_ver[i] != png_libpng_ver[i])
            png_ptr->flags |= PNG_FLAG_LIBRARY_MISMATCH;
      } while (png_libpng_ver[i++]);
   }

   else
      png_ptr->flags |= PNG_FLAG_LIBRARY_MISMATCH;

   if (png_ptr->flags & PNG_FLAG_LIBRARY_MISMATCH)
   {
     /* Libpng 0.90 and later are binary incompatible with libpng 0.89, so
      * we must recompile any applications that use any older library version.
      * For versions after libpng 1.0, we will be compatible, so we need
      * only check the first digit.
      */
      if (user_png_ver == NULL || user_png_ver[0] != png_libpng_ver[0] ||
          (user_png_ver[0] == '1' && user_png_ver[2] != png_libpng_ver[2]) ||
          (user_png_ver[0] == '0' && user_png_ver[2] < '9'))
      {
#ifdef PNG_WARNINGS_SUPPORTED
         size_t pos = 0;
         char m[128];

         pos = png_safecat(m, sizeof m, pos, "Application built with libpng-");
         pos = png_safecat(m, sizeof m, pos, user_png_ver);
         pos = png_safecat(m, sizeof m, pos, " but running with ");
         pos = png_safecat(m, sizeof m, pos, png_libpng_ver);

         png_warning(png_ptr, m);
#endif

#ifdef PNG_ERROR_NUMBERS_SUPPORTED
         png_ptr->flags = 0;
#endif

         return 0;
      }
   }

   /* Success return. */
   return 1;
}

/* Generic function to create a png_struct for either read or write - this
 * contains the common initialization.
 */
PNG_FUNCTION(png_structp /* PRIVATE */,
png_create_png_struct,(png_const_charp user_png_ver, png_voidp error_ptr,
    png_error_ptr error_fn, png_error_ptr warn_fn, png_voidp mem_ptr,
    png_malloc_ptr malloc_fn, png_free_ptr free_fn),PNG_ALLOCATED)
{
   png_struct create_struct;
#  ifdef PNG_SETJMP_SUPPORTED
      jmp_buf create_jmp_buf;
#  endif

   /* This temporary stack-allocated structure is used to provide a place to
    * build enough context to allow the user provided memory allocator (if any)
    * to be called.
    */
   png_memset(&create_struct, 0, sizeof create_struct);

   /* Added at libpng-1.2.6 */
#  ifdef PNG_USER_LIMITS_SUPPORTED
      create_struct.user_width_max = PNG_USER_WIDTH_MAX;
      create_struct.user_height_max = PNG_USER_HEIGHT_MAX;

#     ifdef PNG_USER_CHUNK_CACHE_MAX
         /* Added at libpng-1.2.43 and 1.4.0 */
         create_struct.user_chunk_cache_max = PNG_USER_CHUNK_CACHE_MAX;
#     endif

#     ifdef PNG_SET_USER_CHUNK_MALLOC_MAX
         /* Added at libpng-1.2.43 and 1.4.1, required only for read but exists
          * in png_struct regardless.
          */
         create_struct.user_chunk_malloc_max = PNG_USER_CHUNK_MALLOC_MAX;
#     endif
#  endif

   /* The following two API calls simply set fields in png_struct, so it is safe
    * to do them now even though error handling is not yet set up.
    */
#  ifdef PNG_USER_MEM_SUPPORTED
      png_set_mem_fn(&create_struct, mem_ptr, malloc_fn, free_fn);
#  endif

   /* (*error_fn) can return control to the caller after the error_ptr is set,
    * this will result in a memory leak unless the error_fn does something
    * extremely sophisticated.  The design lacks merit but is implicit in the
    * API.
    */
   png_set_error_fn(&create_struct, error_ptr, error_fn, warn_fn);

#  ifdef PNG_SETJMP_SUPPORTED
      if (!setjmp(create_jmp_buf))
      {
         /* Temporarily fake out the longjmp information until we have
          * successfully completed this function.  This only works if we have
          * setjmp() support compiled in, but it is safe - this stuff should
          * never happen.
          */
         create_struct.jmp_buf_ptr = &create_jmp_buf;
         create_struct.jmp_buf_size = 0; /*stack allocation*/
         create_struct.longjmp_fn = longjmp;
#  else
      {
#  endif
         /* Call the general version checker (shared with read and write code):
          */
         if (png_user_version_check(&create_struct, user_png_ver))
         {

            /* TODO: delay initializing the zlib structure until it really is
             * needed.
             */
            /* Initialize zbuf - compression/decompression buffer */
            create_struct.zbuf_size = PNG_ZBUF_SIZE;
            create_struct.zbuf = png_voidcast(png_bytep,
               png_malloc_warn(&create_struct, create_struct.zbuf_size));

            /* Finally allocate the png_struct itself. */
            if (create_struct.zbuf != NULL)
            {
               png_structrp png_ptr = png_voidcast(png_structrp,
                  png_malloc_warn(&create_struct, sizeof *png_ptr));

               if (png_ptr != NULL)
               {
#                 ifdef PNG_SETJMP_SUPPORTED
                     /* Eliminate the local error handling: */
                     create_struct.jmp_buf_ptr = NULL;
                     create_struct.jmp_buf_size = 0;
                     create_struct.longjmp_fn = 0;
#                 endif

                  *png_ptr = create_struct;

                  /* This is the successful return point */
                  return png_ptr;
               }
            }
         }
      }

   /* A longjmp because of a bug in the application storage allocator or a
    * simple failure to allocate the png_struct.
    */
   if (create_struct.zbuf != NULL)
   {
      png_bytep zbuf = create_struct.zbuf;

      /* Ensure we don't keep on returning to this point: */
      create_struct.zbuf = NULL;
      png_free(&create_struct, zbuf);
   }

   return NULL;
}

/* Allocate the memory for an info_struct for the application. */
PNG_FUNCTION(png_infop,PNGAPI
png_create_info_struct,(png_const_structrp png_ptr),PNG_ALLOCATED)
{
   png_inforp info_ptr;

   png_debug(1, "in png_create_info_struct");

   if (png_ptr == NULL)
      return NULL;

   /* Use the internal API that does not (or at least should not) error out, so
    * that this call always returns ok.  The application typically sets up the
    * error handling *after* creating the info_struct because this is the way it
    * has always been done in 'example.c'.
    */
   info_ptr = png_voidcast(png_inforp, png_malloc_base(png_ptr,
      sizeof *info_ptr));

   if (info_ptr != NULL)
      png_memset(info_ptr, 0, sizeof *info_ptr);

   return info_ptr;
}

/* This function frees the memory associated with a single info struct.
 * Normally, one would use either png_destroy_read_struct() or
 * png_destroy_write_struct() to free an info struct, but this may be
 * useful for some applications.  From libpng 1.6.0 this function is also used
 * internally to implement the png_info release part of the 'struct' destroy
 * APIs.  This ensures that all possible approaches free the same data (all of
 * it).
 */
void PNGAPI
png_destroy_info_struct(png_const_structrp png_ptr, png_infopp info_ptr_ptr)
{
   png_inforp info_ptr = NULL;

   png_debug(1, "in png_destroy_info_struct");

   if (png_ptr == NULL)
      return;

   if (info_ptr_ptr != NULL)
      info_ptr = *info_ptr_ptr;

   if (info_ptr != NULL)
   {
      /* Do this first in case of an error below; if the app implements its own
       * memory management this can lead to png_free calling png_error, which
       * will abort this routine and return control to the app error handler.
       * An infinite loop may result if it then tries to free the same info
       * ptr.
       */
      *info_ptr_ptr = NULL;

      png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
      png_memset(info_ptr, 0, sizeof *info_ptr);
      png_free(png_ptr, info_ptr);
   }
}

/* Initialize the info structure.  This is now an internal function (0.89)
 * and applications using it are urged to use png_create_info_struct()
 * instead.  Use deprecated in 1.6.0, internal use removed (used internally it
 * is just a memset).
 *
 * NOTE: it is almost inconceivable that this API is used because it bypasses
 * the user-memory mechanism and the user error handling/warning mechanisms in
 * those cases where it does anything other than a memset.
 */
PNG_FUNCTION(void,PNGAPI
png_info_init_3,(png_infopp ptr_ptr, png_size_t png_info_struct_size),
   PNG_DEPRECATED)
{
   png_inforp info_ptr = *ptr_ptr;

   png_debug(1, "in png_info_init_3");

   if (info_ptr == NULL)
      return;

   if (png_sizeof(png_info) > png_info_struct_size)
   {
      *ptr_ptr = NULL;
      /* The following line is why this API should not be used: */
      free(info_ptr);
      info_ptr = png_voidcast(png_inforp, png_malloc_base(NULL,
         sizeof *info_ptr));
      *ptr_ptr = info_ptr;
   }

   /* Set everything to 0 */
   png_memset(info_ptr, 0, sizeof *info_ptr);
}

void PNGAPI
png_data_freer(png_const_structrp png_ptr, png_inforp info_ptr,
   int freer, png_uint_32 mask)
{
   png_debug(1, "in png_data_freer");

   if (png_ptr == NULL || info_ptr == NULL)
      return;

   if (freer == PNG_DESTROY_WILL_FREE_DATA)
      info_ptr->free_me |= mask;

   else if (freer == PNG_USER_WILL_FREE_DATA)
      info_ptr->free_me &= ~mask;

   else
      png_warning(png_ptr,
         "Unknown freer parameter in png_data_freer");
}

void PNGAPI
png_free_data(png_const_structrp png_ptr, png_inforp info_ptr, png_uint_32 mask,
   int num)
{
   png_debug(1, "in png_free_data");

   if (png_ptr == NULL || info_ptr == NULL)
      return;

#ifdef PNG_TEXT_SUPPORTED
   /* Free text item num or (if num == -1) all text items */
   if ((mask & PNG_FREE_TEXT) & info_ptr->free_me)
   {
      if (num != -1)
      {
         if (info_ptr->text && info_ptr->text[num].key)
         {
            png_free(png_ptr, info_ptr->text[num].key);
            info_ptr->text[num].key = NULL;
         }
      }

      else
      {
         int i;
         for (i = 0; i < info_ptr->num_text; i++)
             png_free_data(png_ptr, info_ptr, PNG_FREE_TEXT, i);
         png_free(png_ptr, info_ptr->text);
         info_ptr->text = NULL;
         info_ptr->num_text=0;
      }
   }
#endif

#ifdef PNG_tRNS_SUPPORTED
   /* Free any tRNS entry */
   if ((mask & PNG_FREE_TRNS) & info_ptr->free_me)
   {
      png_free(png_ptr, info_ptr->trans_alpha);
      info_ptr->trans_alpha = NULL;
      info_ptr->valid &= ~PNG_INFO_tRNS;
   }
#endif

#ifdef PNG_sCAL_SUPPORTED
   /* Free any sCAL entry */
   if ((mask & PNG_FREE_SCAL) & info_ptr->free_me)
   {
      png_free(png_ptr, info_ptr->scal_s_width);
      png_free(png_ptr, info_ptr->scal_s_height);
      info_ptr->scal_s_width = NULL;
      info_ptr->scal_s_height = NULL;
      info_ptr->valid &= ~PNG_INFO_sCAL;
   }
#endif

#ifdef PNG_pCAL_SUPPORTED
   /* Free any pCAL entry */
   if ((mask & PNG_FREE_PCAL) & info_ptr->free_me)
   {
      png_free(png_ptr, info_ptr->pcal_purpose);
      png_free(png_ptr, info_ptr->pcal_units);
      info_ptr->pcal_purpose = NULL;
      info_ptr->pcal_units = NULL;
      if (info_ptr->pcal_params != NULL)
         {
            int i;
            for (i = 0; i < (int)info_ptr->pcal_nparams; i++)
            {
               png_free(png_ptr, info_ptr->pcal_params[i]);
               info_ptr->pcal_params[i] = NULL;
            }
            png_free(png_ptr, info_ptr->pcal_params);
            info_ptr->pcal_params = NULL;
         }
      info_ptr->valid &= ~PNG_INFO_pCAL;
   }
#endif

#ifdef PNG_iCCP_SUPPORTED
   /* Free any iCCP entry */
   if ((mask & PNG_FREE_ICCP) & info_ptr->free_me)
   {
      png_free(png_ptr, info_ptr->iccp_name);
      png_free(png_ptr, info_ptr->iccp_profile);
      info_ptr->iccp_name = NULL;
      info_ptr->iccp_profile = NULL;
      info_ptr->valid &= ~PNG_INFO_iCCP;
   }
#endif

#ifdef PNG_sPLT_SUPPORTED
   /* Free a given sPLT entry, or (if num == -1) all sPLT entries */
   if ((mask & PNG_FREE_SPLT) & info_ptr->free_me)
   {
      if (num != -1)
      {
         if (info_ptr->splt_palettes)
         {
            png_free(png_ptr, info_ptr->splt_palettes[num].name);
            png_free(png_ptr, info_ptr->splt_palettes[num].entries);
            info_ptr->splt_palettes[num].name = NULL;
            info_ptr->splt_palettes[num].entries = NULL;
         }
      }

      else
      {
         if (info_ptr->splt_palettes_num)
         {
            int i;
            for (i = 0; i < (int)info_ptr->splt_palettes_num; i++)
               png_free_data(png_ptr, info_ptr, PNG_FREE_SPLT, i);

            png_free(png_ptr, info_ptr->splt_palettes);
            info_ptr->splt_palettes = NULL;
            info_ptr->splt_palettes_num = 0;
         }
         info_ptr->valid &= ~PNG_INFO_sPLT;
      }
   }
#endif

#ifdef PNG_UNKNOWN_CHUNKS_SUPPORTED
   if ((mask & PNG_FREE_UNKN) & info_ptr->free_me)
   {
      if (num != -1)
      {
          if (info_ptr->unknown_chunks)
          {
             png_free(png_ptr, info_ptr->unknown_chunks[num].data);
             info_ptr->unknown_chunks[num].data = NULL;
          }
      }

      else
      {
         int i;

         if (info_ptr->unknown_chunks_num)
         {
            for (i = 0; i < info_ptr->unknown_chunks_num; i++)
               png_free_data(png_ptr, info_ptr, PNG_FREE_UNKN, i);

            png_free(png_ptr, info_ptr->unknown_chunks);
            info_ptr->unknown_chunks = NULL;
            info_ptr->unknown_chunks_num = 0;
         }
      }
   }
#endif

#ifdef PNG_hIST_SUPPORTED
   /* Free any hIST entry */
   if ((mask & PNG_FREE_HIST)  & info_ptr->free_me)
   {
      png_free(png_ptr, info_ptr->hist);
      info_ptr->hist = NULL;
      info_ptr->valid &= ~PNG_INFO_hIST;
   }
#endif

   /* Free any PLTE entry that was internally allocated */
   if ((mask & PNG_FREE_PLTE) & info_ptr->free_me)
   {
      png_free(png_ptr, info_ptr->palette);
      info_ptr->palette = NULL;
      info_ptr->valid &= ~PNG_INFO_PLTE;
      info_ptr->num_palette = 0;
   }

#ifdef PNG_INFO_IMAGE_SUPPORTED
   /* Free any image bits attached to the info structure */
   if ((mask & PNG_FREE_ROWS) & info_ptr->free_me)
   {
      if (info_ptr->row_pointers)
      {
         int row;
         for (row = 0; row < (int)info_ptr->height; row++)
         {
            png_free(png_ptr, info_ptr->row_pointers[row]);
            info_ptr->row_pointers[row] = NULL;
         }
         png_free(png_ptr, info_ptr->row_pointers);
         info_ptr->row_pointers = NULL;
      }
      info_ptr->valid &= ~PNG_INFO_IDAT;
   }
#endif

   if (num != -1)
      mask &= ~PNG_FREE_MUL;

   info_ptr->free_me &= ~mask;
}
#endif /* defined(PNG_READ_SUPPORTED) || defined(PNG_WRITE_SUPPORTED) */

/* This function returns a pointer to the io_ptr associated with the user
 * functions.  The application should free any memory associated with this
 * pointer before png_write_destroy() or png_read_destroy() are called.
 */
png_voidp PNGAPI
png_get_io_ptr(png_const_structrp png_ptr)
{
   if (png_ptr == NULL)
      return (NULL);

   return (png_ptr->io_ptr);
}

#if defined(PNG_READ_SUPPORTED) || defined(PNG_WRITE_SUPPORTED)
#  ifdef PNG_STDIO_SUPPORTED
/* Initialize the default input/output functions for the PNG file.  If you
 * use your own read or write routines, you can call either png_set_read_fn()
 * or png_set_write_fn() instead of png_init_io().  If you have defined
 * PNG_NO_STDIO or otherwise disabled PNG_STDIO_SUPPORTED, you must use a
 * function of your own because "FILE *" isn't necessarily available.
 */
void PNGAPI
png_init_io(png_structrp png_ptr, png_FILE_p fp)
{
   png_debug(1, "in png_init_io");

   if (png_ptr == NULL)
      return;

   png_ptr->io_ptr = (png_voidp)fp;
}
#  endif

#  ifdef PNG_TIME_RFC1123_SUPPORTED
/* Convert the supplied time into an RFC 1123 string suitable for use in
 * a "Creation Time" or other text-based time string.
 */
int PNGAPI
png_convert_to_rfc1123_buffer(char out[29], png_const_timep ptime)
{
   static PNG_CONST char short_months[12][4] =
        {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
         "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

   if (out == NULL)
      return 0;

   if (ptime->year > 9999 /* RFC1123 limitation */ ||
       ptime->month == 0    ||  ptime->month > 12  ||
       ptime->day   == 0    ||  ptime->day   > 31  ||
       ptime->hour  > 23    ||  ptime->minute > 59 ||
       ptime->second > 60)
      return 0;

   {
      size_t pos = 0;
      char number_buf[5]; /* enough for a four-digit year */

#     define APPEND_STRING(string) pos = png_safecat(out, 29, pos, (string))
#     define APPEND_NUMBER(format, value)\
         APPEND_STRING(PNG_FORMAT_NUMBER(number_buf, format, (value)))
#     define APPEND(ch) if (pos < 28) out[pos++] = (ch)

      APPEND_NUMBER(PNG_NUMBER_FORMAT_u, (unsigned)ptime->day);
      APPEND(' ');
      APPEND_STRING(short_months[(ptime->month - 1)]);
      APPEND(' ');
      APPEND_NUMBER(PNG_NUMBER_FORMAT_u, ptime->year);
      APPEND(' ');
      APPEND_NUMBER(PNG_NUMBER_FORMAT_02u, (unsigned)ptime->hour);
      APPEND(':');
      APPEND_NUMBER(PNG_NUMBER_FORMAT_02u, (unsigned)ptime->minute);
      APPEND(':');
      APPEND_NUMBER(PNG_NUMBER_FORMAT_02u, (unsigned)ptime->second);
      APPEND_STRING(" +0000"); /* This reliably terminates the buffer */

#     undef APPEND
#     undef APPEND_NUMBER
#     undef APPEND_STRING
   }

   return 1;
}

#     if PNG_LIBPNG_VER < 10700
/* Original API that uses a private buffer in png_struct.
 * TODO: deprecate this, it causes png_struct to carry a spurious temporary
 * buffer (png_struct::time_buffer), better to have the caller pass this in.
 */
png_const_charp PNGAPI
png_convert_to_rfc1123(png_structrp png_ptr, png_const_timep ptime)
{
   if (png_ptr != NULL)
   {
      /* The only failure above if png_ptr != NULL is from an invalid ptime */
      if (!png_convert_to_rfc1123_buffer(png_ptr->time_buffer, ptime))
         png_warning(png_ptr, "Ignoring invalid time value");

      else
         return png_ptr->time_buffer;
   }

   return NULL;
}
#     endif
#  endif /* PNG_TIME_RFC1123_SUPPORTED */

#endif /* defined(PNG_READ_SUPPORTED) || defined(PNG_WRITE_SUPPORTED) */

png_const_charp PNGAPI
png_get_copyright(png_const_structrp png_ptr)
{
   PNG_UNUSED(png_ptr)  /* Silence compiler warning about unused png_ptr */
#ifdef PNG_STRING_COPYRIGHT
   return PNG_STRING_COPYRIGHT
#else
#  ifdef __STDC__
   return PNG_STRING_NEWLINE \
     "libpng version 1.6.0beta08 - February 1, 2012" PNG_STRING_NEWLINE \
     "Copyright (c) 1998-2012 Glenn Randers-Pehrson" PNG_STRING_NEWLINE \
     "Copyright (c) 1996-1997 Andreas Dilger" PNG_STRING_NEWLINE \
     "Copyright (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc." \
     PNG_STRING_NEWLINE;
#  else
      return "libpng version 1.6.0beta08 - February 1, 2012\
      Copyright (c) 1998-2012 Glenn Randers-Pehrson\
      Copyright (c) 1996-1997 Andreas Dilger\
      Copyright (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc.";
#  endif
#endif
}

/* The following return the library version as a short string in the
 * format 1.0.0 through 99.99.99zz.  To get the version of *.h files
 * used with your application, print out PNG_LIBPNG_VER_STRING, which
 * is defined in png.h.
 * Note: now there is no difference between png_get_libpng_ver() and
 * png_get_header_ver().  Due to the version_nn_nn_nn typedef guard,
 * it is guaranteed that png.c uses the correct version of png.h.
 */
png_const_charp PNGAPI
png_get_libpng_ver(png_const_structrp png_ptr)
{
   /* Version of *.c files used when building libpng */
   return png_get_header_ver(png_ptr);
}

png_const_charp PNGAPI
png_get_header_ver(png_const_structrp png_ptr)
{
   /* Version of *.h files used when building libpng */
   PNG_UNUSED(png_ptr)  /* Silence compiler warning about unused png_ptr */
   return PNG_LIBPNG_VER_STRING;
}

png_const_charp PNGAPI
png_get_header_version(png_const_structrp png_ptr)
{
   /* Returns longer string containing both version and date */
   PNG_UNUSED(png_ptr)  /* Silence compiler warning about unused png_ptr */
#ifdef __STDC__
   return PNG_HEADER_VERSION_STRING
#  ifndef PNG_READ_SUPPORTED
   "     (NO READ SUPPORT)"
#  endif
   PNG_STRING_NEWLINE;
#else
   return PNG_HEADER_VERSION_STRING;
#endif
}

#ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
int PNGAPI
png_handle_as_unknown(png_structrp png_ptr, png_const_bytep chunk_name)
{
   /* Check chunk_name and return "keep" value if it's on the list, else 0 */
   png_const_bytep p, p_end;

   if (png_ptr == NULL || chunk_name == NULL || png_ptr->num_chunk_list == 0)
      return PNG_HANDLE_CHUNK_AS_DEFAULT;

   p_end = png_ptr->chunk_list;
   p = p_end + png_ptr->num_chunk_list*5; /* beyond end */

   /* The code is the fifth byte after each four byte string.  Historically this
    * code was always searched from the end of the list, so it should continue
    * to do so in case there are duplicated entries.
    */
   do /* num_chunk_list > 0, so at least one */
   {
      p -= 5;
      if (!png_memcmp(chunk_name, p, 4))
         return p[4];
   }
   while (p > p_end);

   return PNG_HANDLE_CHUNK_AS_DEFAULT;
}

int /* PRIVATE */
png_chunk_unknown_handling(png_structrp png_ptr, png_uint_32 chunk_name)
{
   png_byte chunk_string[5];

   PNG_CSTRING_FROM_CHUNK(chunk_string, chunk_name);
   return png_handle_as_unknown(png_ptr, chunk_string);
}
#endif

#ifdef PNG_READ_SUPPORTED
/* This function, added to libpng-1.0.6g, is untested. */
int PNGAPI
png_reset_zstream(png_structrp png_ptr)
{
   if (png_ptr == NULL)
      return Z_STREAM_ERROR;

   return (inflateReset(&png_ptr->zstream));
}
#endif /* PNG_READ_SUPPORTED */

/* This function was added to libpng-1.0.7 */
png_uint_32 PNGAPI
png_access_version_number(void)
{
   /* Version of *.c files used when building libpng */
   return((png_uint_32)PNG_LIBPNG_VER);
}



#if defined(PNG_READ_SUPPORTED) || defined(PNG_WRITE_SUPPORTED)
/* png_convert_size: a PNGAPI but no longer in png.h, so deleted
 * at libpng 1.5.5!
 */

/* Added at libpng version 1.2.34 and 1.4.0 (moved from pngset.c) */
#  ifdef PNG_CHECK_cHRM_SUPPORTED

int /* PRIVATE */
png_check_cHRM_fixed(png_const_structrp png_ptr,
   png_fixed_point white_x, png_fixed_point white_y, png_fixed_point red_x,
   png_fixed_point red_y, png_fixed_point green_x, png_fixed_point green_y,
   png_fixed_point blue_x, png_fixed_point blue_y)
{
   int ret = 1;
   unsigned long xy_hi,xy_lo,yx_hi,yx_lo;

   png_debug(1, "in function png_check_cHRM_fixed");

   if (png_ptr == NULL)
      return 0;

   /* (x,y,z) values are first limited to 0..100000 (PNG_FP_1), the white
    * y must also be greater than 0.  To test for the upper limit calculate
    * (PNG_FP_1-y) - x must be <= to this for z to be >= 0 (and the expression
    * cannot overflow.)  At this point we know x and y are >= 0 and (x+y) is
    * <= PNG_FP_1.  The previous test on PNG_MAX_UINT_31 is removed because it
    * pointless (and it produces compiler warnings!)
    */
   if (white_x < 0 || white_y <= 0 ||
         red_x < 0 ||   red_y <  0 ||
       green_x < 0 || green_y <  0 ||
        blue_x < 0 ||  blue_y <  0)
   {
      png_warning(png_ptr,
        "Ignoring attempt to set negative chromaticity value");
      ret = 0;
   }
   /* And (x+y) must be <= PNG_FP_1 (so z is >= 0) */
   if (white_x > PNG_FP_1 - white_y)
   {
      png_warning(png_ptr, "Invalid cHRM white point");
      ret = 0;
   }

   if (red_x > PNG_FP_1 - red_y)
   {
      png_warning(png_ptr, "Invalid cHRM red point");
      ret = 0;
   }

   if (green_x > PNG_FP_1 - green_y)
   {
      png_warning(png_ptr, "Invalid cHRM green point");
      ret = 0;
   }

   if (blue_x > PNG_FP_1 - blue_y)
   {
      png_warning(png_ptr, "Invalid cHRM blue point");
      ret = 0;
   }

   png_64bit_product(green_x - red_x, blue_y - red_y, &xy_hi, &xy_lo);
   png_64bit_product(green_y - red_y, blue_x - red_x, &yx_hi, &yx_lo);

   if (xy_hi == yx_hi && xy_lo == yx_lo)
   {
      png_warning(png_ptr,
         "Ignoring attempt to set cHRM RGB triangle with zero area");
      ret = 0;
   }

   return ret;
}
#  endif /* PNG_CHECK_cHRM_SUPPORTED */

#ifdef PNG_cHRM_SUPPORTED
/* Added at libpng-1.5.5 to support read and write of true CIEXYZ values for
 * cHRM, as opposed to using chromaticities.  These internal APIs return
 * non-zero on a parameter error.  The X, Y and Z values are required to be
 * positive and less than 1.0.
 */
int png_xy_from_XYZ(png_xy *xy, png_XYZ XYZ)
{
   png_int_32 d, dwhite, whiteX, whiteY;

   d = XYZ.redX + XYZ.redY + XYZ.redZ;
   if (!png_muldiv(&xy->redx, XYZ.redX, PNG_FP_1, d)) return 1;
   if (!png_muldiv(&xy->redy, XYZ.redY, PNG_FP_1, d)) return 1;
   dwhite = d;
   whiteX = XYZ.redX;
   whiteY = XYZ.redY;

   d = XYZ.greenX + XYZ.greenY + XYZ.greenZ;
   if (!png_muldiv(&xy->greenx, XYZ.greenX, PNG_FP_1, d)) return 1;
   if (!png_muldiv(&xy->greeny, XYZ.greenY, PNG_FP_1, d)) return 1;
   dwhite += d;
   whiteX += XYZ.greenX;
   whiteY += XYZ.greenY;

   d = XYZ.blueX + XYZ.blueY + XYZ.blueZ;
   if (!png_muldiv(&xy->bluex, XYZ.blueX, PNG_FP_1, d)) return 1;
   if (!png_muldiv(&xy->bluey, XYZ.blueY, PNG_FP_1, d)) return 1;
   dwhite += d;
   whiteX += XYZ.blueX;
   whiteY += XYZ.blueY;

   /* The reference white is simply the same of the end-point (X,Y,Z) vectors,
    * thus:
    */
   if (!png_muldiv(&xy->whitex, whiteX, PNG_FP_1, dwhite)) return 1;
   if (!png_muldiv(&xy->whitey, whiteY, PNG_FP_1, dwhite)) return 1;

   return 0;
}

int png_XYZ_from_xy(png_XYZ *XYZ, png_xy xy)
{
   png_fixed_point red_inverse, green_inverse, blue_scale;
   png_fixed_point left, right, denominator;

   /* Check xy and, implicitly, z.  Note that wide gamut color spaces typically
    * have end points with 0 tristimulus values (these are impossible end
    * points, but they are used to cover the possible colors.)
    */
   if (xy.redx < 0 || xy.redx > PNG_FP_1) return 1;
   if (xy.redy < 0 || xy.redy > PNG_FP_1-xy.redx) return 1;
   if (xy.greenx < 0 || xy.greenx > PNG_FP_1) return 1;
   if (xy.greeny < 0 || xy.greeny > PNG_FP_1-xy.greenx) return 1;
   if (xy.bluex < 0 || xy.bluex > PNG_FP_1) return 1;
   if (xy.bluey < 0 || xy.bluey > PNG_FP_1-xy.bluex) return 1;
   if (xy.whitex < 0 || xy.whitex > PNG_FP_1) return 1;
   if (xy.whitey < 0 || xy.whitey > PNG_FP_1-xy.whitex) return 1;

   /* The reverse calculation is more difficult because the original tristimulus
    * value had 9 independent values (red,green,blue)x(X,Y,Z) however only 8
    * derived values were recorded in the cHRM chunk;
    * (red,green,blue,white)x(x,y).  This loses one degree of freedom and
    * therefore an arbitrary ninth value has to be introduced to undo the
    * original transformations.
    *
    * Think of the original end-points as points in (X,Y,Z) space.  The
    * chromaticity values (c) have the property:
    *
    *           C
    *   c = ---------
    *       X + Y + Z
    *
    * For each c (x,y,z) from the corresponding original C (X,Y,Z).  Thus the
    * three chromaticity values (x,y,z) for each end-point obey the
    * relationship:
    *
    *   x + y + z = 1
    *
    * This describes the plane in (X,Y,Z) space that intersects each axis at the
    * value 1.0; call this the chromaticity plane.  Thus the chromaticity
    * calculation has scaled each end-point so that it is on the x+y+z=1 plane
    * and chromaticity is the intersection of the vector from the origin to the
    * (X,Y,Z) value with the chromaticity plane.
    *
    * To fully invert the chromaticity calculation we would need the three
    * end-point scale factors, (red-scale, green-scale, blue-scale), but these
    * were not recorded.  Instead we calculated the reference white (X,Y,Z) and
    * recorded the chromaticity of this.  The reference white (X,Y,Z) would have
    * given all three of the scale factors since:
    *
    *    color-C = color-c * color-scale
    *    white-C = red-C + green-C + blue-C
    *            = red-c*red-scale + green-c*green-scale + blue-c*blue-scale
    *
    * But cHRM records only white-x and white-y, so we have lost the white scale
    * factor:
    *
    *    white-C = white-c*white-scale
    *
    * To handle this the inverse transformation makes an arbitrary assumption
    * about white-scale:
    *
    *    Assume: white-Y = 1.0
    *    Hence:  white-scale = 1/white-y
    *    Or:     red-Y + green-Y + blue-Y = 1.0
    *
    * Notice the last statement of the assumption gives an equation in three of
    * the nine values we want to calculate.  8 more equations come from the
    * above routine as summarised at the top above (the chromaticity
    * calculation):
    *
    *    Given: color-x = color-X / (color-X + color-Y + color-Z)
    *    Hence: (color-x - 1)*color-X + color.x*color-Y + color.x*color-Z = 0
    *
    * This is 9 simultaneous equations in the 9 variables "color-C" and can be
    * solved by Cramer's rule.  Cramer's rule requires calculating 10 9x9 matrix
    * determinants, however this is not as bad as it seems because only 28 of
    * the total of 90 terms in the various matrices are non-zero.  Nevertheless
    * Cramer's rule is notoriously numerically unstable because the determinant
    * calculation involves the difference of large, but similar, numbers.  It is
    * difficult to be sure that the calculation is stable for real world values
    * and it is certain that it becomes unstable where the end points are close
    * together.
    *
    * So this code uses the perhaps slighly less optimal but more understandable
    * and totally obvious approach of calculating color-scale.
    *
    * This algorithm depends on the precision in white-scale and that is
    * (1/white-y), so we can immediately see that as white-y approaches 0 the
    * accuracy inherent in the cHRM chunk drops off substantially.
    *
    * libpng arithmetic: a simple invertion of the above equations
    * ------------------------------------------------------------
    *
    *    white_scale = 1/white-y
    *    white-X = white-x * white-scale
    *    white-Y = 1.0
    *    white-Z = (1 - white-x - white-y) * white_scale
    *
    *    white-C = red-C + green-C + blue-C
    *            = red-c*red-scale + green-c*green-scale + blue-c*blue-scale
    *
    * This gives us three equations in (red-scale,green-scale,blue-scale) where
    * all the coefficients are now known:
    *
    *    red-x*red-scale + green-x*green-scale + blue-x*blue-scale
    *       = white-x/white-y
    *    red-y*red-scale + green-y*green-scale + blue-y*blue-scale = 1
    *    red-z*red-scale + green-z*green-scale + blue-z*blue-scale
    *       = (1 - white-x - white-y)/white-y
    *
    * In the last equation color-z is (1 - color-x - color-y) so we can add all
    * three equations together to get an alternative third:
    *
    *    red-scale + green-scale + blue-scale = 1/white-y = white-scale
    *
    * So now we have a Cramer's rule solution where the determinants are just
    * 3x3 - far more tractible.  Unfortunately 3x3 determinants still involve
    * multiplication of three coefficients so we can't guarantee to avoid
    * overflow in the libpng fixed point representation.  Using Cramer's rule in
    * floating point is probably a good choice here, but it's not an option for
    * fixed point.  Instead proceed to simplify the first two equations by
    * eliminating what is likely to be the largest value, blue-scale:
    *
    *    blue-scale = white-scale - red-scale - green-scale
    *
    * Hence:
    *
    *    (red-x - blue-x)*red-scale + (green-x - blue-x)*green-scale =
    *                (white-x - blue-x)*white-scale
    *
    *    (red-y - blue-y)*red-scale + (green-y - blue-y)*green-scale =
    *                1 - blue-y*white-scale
    *
    * And now we can trivially solve for (red-scale,green-scale):
    *
    *    green-scale =
    *                (white-x - blue-x)*white-scale - (red-x - blue-x)*red-scale
    *                -----------------------------------------------------------
    *                                  green-x - blue-x
    *
    *    red-scale =
    *                1 - blue-y*white-scale - (green-y - blue-y) * green-scale
    *                ---------------------------------------------------------
    *                                  red-y - blue-y
    *
    * Hence:
    *
    *    red-scale =
    *          ( (green-x - blue-x) * (white-y - blue-y) -
    *            (green-y - blue-y) * (white-x - blue-x) ) / white-y
    * -------------------------------------------------------------------------
    *  (green-x - blue-x)*(red-y - blue-y)-(green-y - blue-y)*(red-x - blue-x)
    *
    *    green-scale =
    *          ( (red-y - blue-y) * (white-x - blue-x) -
    *            (red-x - blue-x) * (white-y - blue-y) ) / white-y
    * -------------------------------------------------------------------------
    *  (green-x - blue-x)*(red-y - blue-y)-(green-y - blue-y)*(red-x - blue-x)
    *
    * Accuracy:
    * The input values have 5 decimal digits of accuracy.  The values are all in
    * the range 0 < value < 1, so simple products are in the same range but may
    * need up to 10 decimal digits to preserve the original precision and avoid
    * underflow.  Because we are using a 32-bit signed representation we cannot
    * match this; the best is a little over 9 decimal digits, less than 10.
    *
    * The approach used here is to preserve the maximum precision within the
    * signed representation.  Because the red-scale calculation above uses the
    * difference between two products of values that must be in the range -1..+1
    * it is sufficient to divide the product by 7; ceil(100,000/32767*2).  The
    * factor is irrelevant in the calculation because it is applied to both
    * numerator and denominator.
    *
    * Note that the values of the differences of the products of the
    * chromaticities in the above equations tend to be small, for example for
    * the sRGB chromaticities they are:
    *
    * red numerator:    -0.04751
    * green numerator:  -0.08788
    * denominator:      -0.2241 (without white-y multiplication)
    *
    *  The resultant Y coefficients from the chromaticities of some widely used
    *  color space definitions are (to 15 decimal places):
    *
    *  sRGB
    *    0.212639005871510 0.715168678767756 0.072192315360734
    *  Kodak ProPhoto
    *    0.288071128229293 0.711843217810102 0.000085653960605
    *  Adobe RGB
    *    0.297344975250536 0.627363566255466 0.075291458493998
    *  Adobe Wide Gamut RGB
    *    0.258728243040113 0.724682314948566 0.016589442011321
    */
   /* By the argument, above overflow should be impossible here. The return
    * value of 2 indicates an internal error to the caller.
    */
   if (!png_muldiv(&left, xy.greenx-xy.bluex, xy.redy - xy.bluey, 7)) return 2;
   if (!png_muldiv(&right, xy.greeny-xy.bluey, xy.redx - xy.bluex, 7)) return 2;
   denominator = left - right;

   /* Now find the red numerator. */
   if (!png_muldiv(&left, xy.greenx-xy.bluex, xy.whitey-xy.bluey, 7)) return 2;
   if (!png_muldiv(&right, xy.greeny-xy.bluey, xy.whitex-xy.bluex, 7)) return 2;

   /* Overflow is possible here and it indicates an extreme set of PNG cHRM
    * chunk values.  This calculation actually returns the reciprocal of the
    * scale value because this allows us to delay the multiplication of white-y
    * into the denominator, which tends to produce a small number.
    */
   if (!png_muldiv(&red_inverse, xy.whitey, denominator, left-right) ||
       red_inverse <= xy.whitey /* r+g+b scales = white scale */)
      return 1;

   /* Similarly for green_inverse: */
   if (!png_muldiv(&left, xy.redy-xy.bluey, xy.whitex-xy.bluex, 7)) return 2;
   if (!png_muldiv(&right, xy.redx-xy.bluex, xy.whitey-xy.bluey, 7)) return 2;
   if (!png_muldiv(&green_inverse, xy.whitey, denominator, left-right) ||
       green_inverse <= xy.whitey)
      return 1;

   /* And the blue scale, the checks above guarantee this can't overflow but it
    * can still produce 0 for extreme cHRM values.
    */
   blue_scale = png_reciprocal(xy.whitey) - png_reciprocal(red_inverse) -
      png_reciprocal(green_inverse);
   if (blue_scale <= 0) return 1;


   /* And fill in the png_XYZ: */
   if (!png_muldiv(&XYZ->redX, xy.redx, PNG_FP_1, red_inverse)) return 1;
   if (!png_muldiv(&XYZ->redY, xy.redy, PNG_FP_1, red_inverse)) return 1;
   if (!png_muldiv(&XYZ->redZ, PNG_FP_1 - xy.redx - xy.redy, PNG_FP_1,
      red_inverse))
      return 1;

   if (!png_muldiv(&XYZ->greenX, xy.greenx, PNG_FP_1, green_inverse)) return 1;
   if (!png_muldiv(&XYZ->greenY, xy.greeny, PNG_FP_1, green_inverse)) return 1;
   if (!png_muldiv(&XYZ->greenZ, PNG_FP_1 - xy.greenx - xy.greeny, PNG_FP_1,
      green_inverse))
      return 1;

   if (!png_muldiv(&XYZ->blueX, xy.bluex, blue_scale, PNG_FP_1)) return 1;
   if (!png_muldiv(&XYZ->blueY, xy.bluey, blue_scale, PNG_FP_1)) return 1;
   if (!png_muldiv(&XYZ->blueZ, PNG_FP_1 - xy.bluex - xy.bluey, blue_scale,
      PNG_FP_1))
      return 1;

   return 0; /*success*/
}

int png_XYZ_from_xy_checked(png_const_structrp png_ptr, png_XYZ *XYZ, png_xy xy)
{
   switch (png_XYZ_from_xy(XYZ, xy))
   {
      case 0: /* success */
         return 1;

      case 1:
         /* The chunk may be technically valid, but we got png_fixed_point
          * overflow while trying to get XYZ values out of it.  This is
          * entirely benign - the cHRM chunk is pretty extreme.
          */
         png_warning(png_ptr,
            "extreme cHRM chunk cannot be converted to tristimulus values");
         break;

      default:
         /* libpng is broken; this should be a warning but if it happens we
          * want error reports so for the moment it is an error.
          */
         png_error(png_ptr, "internal error in png_XYZ_from_xy");
         break;
   }

   /* ERROR RETURN */
   return 0;
}
#endif

void /* PRIVATE */
png_check_IHDR(png_const_structrp png_ptr,
   png_uint_32 width, png_uint_32 height, int bit_depth,
   int color_type, int interlace_type, int compression_type,
   int filter_type)
{
   int error = 0;

   /* Check for width and height valid values */
   if (width == 0)
   {
      png_warning(png_ptr, "Image width is zero in IHDR");
      error = 1;
   }

   if (height == 0)
   {
      png_warning(png_ptr, "Image height is zero in IHDR");
      error = 1;
   }

#  ifdef PNG_SET_USER_LIMITS_SUPPORTED
   if (width > png_ptr->user_width_max)

#  else
   if (width > PNG_USER_WIDTH_MAX)
#  endif
   {
      png_warning(png_ptr, "Image width exceeds user limit in IHDR");
      error = 1;
   }

#  ifdef PNG_SET_USER_LIMITS_SUPPORTED
   if (height > png_ptr->user_height_max)
#  else
   if (height > PNG_USER_HEIGHT_MAX)
#  endif
   {
      png_warning(png_ptr, "Image height exceeds user limit in IHDR");
      error = 1;
   }

   if (width > PNG_UINT_31_MAX)
   {
      png_warning(png_ptr, "Invalid image width in IHDR");
      error = 1;
   }

   if (height > PNG_UINT_31_MAX)
   {
      png_warning(png_ptr, "Invalid image height in IHDR");
      error = 1;
   }

   if (width > (PNG_UINT_32_MAX
                 >> 3)      /* 8-byte RGBA pixels */
                 - 48       /* bigrowbuf hack */
                 - 1        /* filter byte */
                 - 7*8      /* rounding of width to multiple of 8 pixels */
                 - 8)       /* extra max_pixel_depth pad */
      png_warning(png_ptr, "Width is too large for libpng to process pixels");

   /* Check other values */
   if (bit_depth != 1 && bit_depth != 2 && bit_depth != 4 &&
       bit_depth != 8 && bit_depth != 16)
   {
      png_warning(png_ptr, "Invalid bit depth in IHDR");
      error = 1;
   }

   if (color_type < 0 || color_type == 1 ||
       color_type == 5 || color_type > 6)
   {
      png_warning(png_ptr, "Invalid color type in IHDR");
      error = 1;
   }

   if (((color_type == PNG_COLOR_TYPE_PALETTE) && bit_depth > 8) ||
       ((color_type == PNG_COLOR_TYPE_RGB ||
         color_type == PNG_COLOR_TYPE_GRAY_ALPHA ||
         color_type == PNG_COLOR_TYPE_RGB_ALPHA) && bit_depth < 8))
   {
      png_warning(png_ptr, "Invalid color type/bit depth combination in IHDR");
      error = 1;
   }

   if (interlace_type >= PNG_INTERLACE_LAST)
   {
      png_warning(png_ptr, "Unknown interlace method in IHDR");
      error = 1;
   }

   if (compression_type != PNG_COMPRESSION_TYPE_BASE)
   {
      png_warning(png_ptr, "Unknown compression method in IHDR");
      error = 1;
   }

#  ifdef PNG_MNG_FEATURES_SUPPORTED
   /* Accept filter_method 64 (intrapixel differencing) only if
    * 1. Libpng was compiled with PNG_MNG_FEATURES_SUPPORTED and
    * 2. Libpng did not read a PNG signature (this filter_method is only
    *    used in PNG datastreams that are embedded in MNG datastreams) and
    * 3. The application called png_permit_mng_features with a mask that
    *    included PNG_FLAG_MNG_FILTER_64 and
    * 4. The filter_method is 64 and
    * 5. The color_type is RGB or RGBA
    */
   if ((png_ptr->mode & PNG_HAVE_PNG_SIGNATURE) &&
       png_ptr->mng_features_permitted)
      png_warning(png_ptr, "MNG features are not allowed in a PNG datastream");

   if (filter_type != PNG_FILTER_TYPE_BASE)
   {
      if (!((png_ptr->mng_features_permitted & PNG_FLAG_MNG_FILTER_64) &&
          (filter_type == PNG_INTRAPIXEL_DIFFERENCING) &&
          ((png_ptr->mode & PNG_HAVE_PNG_SIGNATURE) == 0) &&
          (color_type == PNG_COLOR_TYPE_RGB ||
          color_type == PNG_COLOR_TYPE_RGB_ALPHA)))
      {
         png_warning(png_ptr, "Unknown filter method in IHDR");
         error = 1;
      }

      if (png_ptr->mode & PNG_HAVE_PNG_SIGNATURE)
      {
         png_warning(png_ptr, "Invalid filter method in IHDR");
         error = 1;
      }
   }

#  else
   if (filter_type != PNG_FILTER_TYPE_BASE)
   {
      png_warning(png_ptr, "Unknown filter method in IHDR");
      error = 1;
   }
#  endif

   if (error == 1)
      png_error(png_ptr, "Invalid IHDR data");
}

#if defined(PNG_sCAL_SUPPORTED) || defined(PNG_pCAL_SUPPORTED)
/* ASCII to fp functions */
/* Check an ASCII formated floating point value, see the more detailed
 * comments in pngpriv.h
 */
/* The following is used internally to preserve the sticky flags */
#define png_fp_add(state, flags) ((state) |= (flags))
#define png_fp_set(state, value) ((state) = (value) | ((state) & PNG_FP_STICKY))

int /* PRIVATE */
png_check_fp_number(png_const_charp string, png_size_t size, int *statep,
   png_size_tp whereami)
{
   int state = *statep;
   png_size_t i = *whereami;

   while (i < size)
   {
      int type;
      /* First find the type of the next character */
      switch (string[i])
      {
      case 43:  type = PNG_FP_SAW_SIGN;                   break;
      case 45:  type = PNG_FP_SAW_SIGN + PNG_FP_NEGATIVE; break;
      case 46:  type = PNG_FP_SAW_DOT;                    break;
      case 48:  type = PNG_FP_SAW_DIGIT;                  break;
      case 49: case 50: case 51: case 52:
      case 53: case 54: case 55: case 56:
      case 57:  type = PNG_FP_SAW_DIGIT + PNG_FP_NONZERO; break;
      case 69:
      case 101: type = PNG_FP_SAW_E;                      break;
      default:  goto PNG_FP_End;
      }

      /* Now deal with this type according to the current
       * state, the type is arranged to not overlap the
       * bits of the PNG_FP_STATE.
       */
      switch ((state & PNG_FP_STATE) + (type & PNG_FP_SAW_ANY))
      {
      case PNG_FP_INTEGER + PNG_FP_SAW_SIGN:
         if (state & PNG_FP_SAW_ANY)
            goto PNG_FP_End; /* not a part of the number */

         png_fp_add(state, type);
         break;

      case PNG_FP_INTEGER + PNG_FP_SAW_DOT:
         /* Ok as trailer, ok as lead of fraction. */
         if (state & PNG_FP_SAW_DOT) /* two dots */
            goto PNG_FP_End;

         else if (state & PNG_FP_SAW_DIGIT) /* trailing dot? */
            png_fp_add(state, type);

         else
            png_fp_set(state, PNG_FP_FRACTION | type);

         break;

      case PNG_FP_INTEGER + PNG_FP_SAW_DIGIT:
         if (state & PNG_FP_SAW_DOT) /* delayed fraction */
            png_fp_set(state, PNG_FP_FRACTION | PNG_FP_SAW_DOT);

         png_fp_add(state, type | PNG_FP_WAS_VALID);

         break;

      case PNG_FP_INTEGER + PNG_FP_SAW_E:
         if ((state & PNG_FP_SAW_DIGIT) == 0)
            goto PNG_FP_End;

         png_fp_set(state, PNG_FP_EXPONENT);

         break;

   /* case PNG_FP_FRACTION + PNG_FP_SAW_SIGN:
         goto PNG_FP_End; ** no sign in fraction */

   /* case PNG_FP_FRACTION + PNG_FP_SAW_DOT:
         goto PNG_FP_End; ** Because SAW_DOT is always set */

      case PNG_FP_FRACTION + PNG_FP_SAW_DIGIT:
         png_fp_add(state, type | PNG_FP_WAS_VALID);
         break;

      case PNG_FP_FRACTION + PNG_FP_SAW_E:
         /* This is correct because the trailing '.' on an
          * integer is handled above - so we can only get here
          * with the sequence ".E" (with no preceding digits).
          */
         if ((state & PNG_FP_SAW_DIGIT) == 0)
            goto PNG_FP_End;

         png_fp_set(state, PNG_FP_EXPONENT);

         break;

      case PNG_FP_EXPONENT + PNG_FP_SAW_SIGN:
         if (state & PNG_FP_SAW_ANY)
            goto PNG_FP_End; /* not a part of the number */

         png_fp_add(state, PNG_FP_SAW_SIGN);

         break;

   /* case PNG_FP_EXPONENT + PNG_FP_SAW_DOT:
         goto PNG_FP_End; */

      case PNG_FP_EXPONENT + PNG_FP_SAW_DIGIT:
         png_fp_add(state, PNG_FP_SAW_DIGIT | PNG_FP_WAS_VALID);

         break;

   /* case PNG_FP_EXPONEXT + PNG_FP_SAW_E:
         goto PNG_FP_End; */

      default: goto PNG_FP_End; /* I.e. break 2 */
      }

      /* The character seems ok, continue. */
      ++i;
   }

PNG_FP_End:
   /* Here at the end, update the state and return the correct
    * return code.
    */
   *statep = state;
   *whereami = i;

   return (state & PNG_FP_SAW_DIGIT) != 0;
}


/* The same but for a complete string. */
int
png_check_fp_string(png_const_charp string, png_size_t size)
{
   int        state=0;
   png_size_t char_index=0;

   if (png_check_fp_number(string, size, &state, &char_index) &&
      (char_index == size || string[char_index] == 0))
      return state /* must be non-zero - see above */;

   return 0; /* i.e. fail */
}
#endif /* pCAL or sCAL */

#ifdef PNG_READ_sCAL_SUPPORTED
#  ifdef PNG_FLOATING_POINT_SUPPORTED
/* Utility used below - a simple accurate power of ten from an integral
 * exponent.
 */
static double
png_pow10(int power)
{
   int recip = 0;
   double d = 1;

   /* Handle negative exponent with a reciprocal at the end because
    * 10 is exact whereas .1 is inexact in base 2
    */
   if (power < 0)
   {
      if (power < DBL_MIN_10_EXP) return 0;
      recip = 1, power = -power;
   }

   if (power > 0)
   {
      /* Decompose power bitwise. */
      double mult = 10;
      do
      {
         if (power & 1) d *= mult;
         mult *= mult;
         power >>= 1;
      }
      while (power > 0);

      if (recip) d = 1/d;
   }
   /* else power is 0 and d is 1 */

   return d;
}

/* Function to format a floating point value in ASCII with a given
 * precision.
 */
void /* PRIVATE */
png_ascii_from_fp(png_const_structrp png_ptr, png_charp ascii, png_size_t size,
    double fp, unsigned int precision)
{
   /* We use standard functions from math.h, but not printf because
    * that would require stdio.  The caller must supply a buffer of
    * sufficient size or we will png_error.  The tests on size and
    * the space in ascii[] consumed are indicated below.
    */
   if (precision < 1)
      precision = DBL_DIG;

   /* Enforce the limit of the implementation precision too. */
   if (precision > DBL_DIG+1)
      precision = DBL_DIG+1;

   /* Basic sanity checks */
   if (size >= precision+5) /* See the requirements below. */
   {
      if (fp < 0)
      {
         fp = -fp;
         *ascii++ = 45; /* '-'  PLUS 1 TOTAL 1 */
         --size;
      }

      if (fp >= DBL_MIN && fp <= DBL_MAX)
      {
         int exp_b10;       /* A base 10 exponent */
         double base;   /* 10^exp_b10 */

         /* First extract a base 10 exponent of the number,
          * the calculation below rounds down when converting
          * from base 2 to base 10 (multiply by log10(2) -
          * 0.3010, but 77/256 is 0.3008, so exp_b10 needs to
          * be increased.  Note that the arithmetic shift
          * performs a floor() unlike C arithmetic - using a
          * C multiply would break the following for negative
          * exponents.
          */
         (void)frexp(fp, &exp_b10); /* exponent to base 2 */

         exp_b10 = (exp_b10 * 77) >> 8; /* <= exponent to base 10 */

         /* Avoid underflow here. */
         base = png_pow10(exp_b10); /* May underflow */

         while (base < DBL_MIN || base < fp)
         {
            /* And this may overflow. */
            double test = png_pow10(exp_b10+1);

            if (test <= DBL_MAX)
               ++exp_b10, base = test;

            else
               break;
         }

         /* Normalize fp and correct exp_b10, after this fp is in the
          * range [.1,1) and exp_b10 is both the exponent and the digit
          * *before* which the decimal point should be inserted
          * (starting with 0 for the first digit).  Note that this
          * works even if 10^exp_b10 is out of range because of the
          * test on DBL_MAX above.
          */
         fp /= base;
         while (fp >= 1) fp /= 10, ++exp_b10;

         /* Because of the code above fp may, at this point, be
          * less than .1, this is ok because the code below can
          * handle the leading zeros this generates, so no attempt
          * is made to correct that here.
          */

         {
            int czero, clead, cdigits;
            char exponent[10];

            /* Allow up to two leading zeros - this will not lengthen
             * the number compared to using E-n.
             */
            if (exp_b10 < 0 && exp_b10 > -3) /* PLUS 3 TOTAL 4 */
            {
               czero = -exp_b10; /* PLUS 2 digits: TOTAL 3 */
               exp_b10 = 0;      /* Dot added below before first output. */
            }
            else
               czero = 0;    /* No zeros to add */

            /* Generate the digit list, stripping trailing zeros and
             * inserting a '.' before a digit if the exponent is 0.
             */
            clead = czero; /* Count of leading zeros */
            cdigits = 0;   /* Count of digits in list. */

            do
            {
               double d;

               fp *= 10;
               /* Use modf here, not floor and subtract, so that
                * the separation is done in one step.  At the end
                * of the loop don't break the number into parts so
                * that the final digit is rounded.
                */
               if (cdigits+czero-clead+1 < (int)precision)
                  fp = modf(fp, &d);

               else
               {
                  d = floor(fp + .5);

                  if (d > 9)
                  {
                     /* Rounding up to 10, handle that here. */
                     if (czero > 0)
                     {
                        --czero, d = 1;
                        if (cdigits == 0) --clead;
                     }
                     else
                     {
                        while (cdigits > 0 && d > 9)
                        {
                           int ch = *--ascii;

                           if (exp_b10 != (-1))
                              ++exp_b10;

                           else if (ch == 46)
                           {
                              ch = *--ascii, ++size;
                              /* Advance exp_b10 to '1', so that the
                               * decimal point happens after the
                               * previous digit.
                               */
                              exp_b10 = 1;
                           }

                           --cdigits;
                           d = ch - 47;  /* I.e. 1+(ch-48) */
                        }

                        /* Did we reach the beginning? If so adjust the
                         * exponent but take into account the leading
                         * decimal point.
                         */
                        if (d > 9)  /* cdigits == 0 */
                        {
                           if (exp_b10 == (-1))
                           {
                              /* Leading decimal point (plus zeros?), if
                               * we lose the decimal point here it must
                               * be reentered below.
                               */
                              int ch = *--ascii;

                              if (ch == 46)
                                 ++size, exp_b10 = 1;

                              /* Else lost a leading zero, so 'exp_b10' is
                               * still ok at (-1)
                               */
                           }
                           else
                              ++exp_b10;

                           /* In all cases we output a '1' */
                           d = 1;
                        }
                     }
                  }
                  fp = 0; /* Guarantees termination below. */
               }

               if (d == 0)
               {
                  ++czero;
                  if (cdigits == 0) ++clead;
               }
               else
               {
                  /* Included embedded zeros in the digit count. */
                  cdigits += czero - clead;
                  clead = 0;

                  while (czero > 0)
                  {
                     /* exp_b10 == (-1) means we just output the decimal
                      * place - after the DP don't adjust 'exp_b10' any
                      * more!
                      */
                     if (exp_b10 != (-1))
                     {
                        if (exp_b10 == 0) *ascii++ = 46, --size;
                        /* PLUS 1: TOTAL 4 */
                        --exp_b10;
                     }
                     *ascii++ = 48, --czero;
                  }

                  if (exp_b10 != (-1))
                  {
                     if (exp_b10 == 0) *ascii++ = 46, --size; /* counted
                                                                 above */
                     --exp_b10;
                  }
                  *ascii++ = (char)(48 + (int)d), ++cdigits;
               }
            }
            while (cdigits+czero-clead < (int)precision && fp > DBL_MIN);

            /* The total output count (max) is now 4+precision */

            /* Check for an exponent, if we don't need one we are
             * done and just need to terminate the string.  At
             * this point exp_b10==(-1) is effectively if flag - it got
             * to '-1' because of the decrement after outputing
             * the decimal point above (the exponent required is
             * *not* -1!)
             */
            if (exp_b10 >= (-1) && exp_b10 <= 2)
            {
               /* The following only happens if we didn't output the
                * leading zeros above for negative exponent, so this
                * doest add to the digit requirement.  Note that the
                * two zeros here can only be output if the two leading
                * zeros were *not* output, so this doesn't increase
                * the output count.
                */
               while (--exp_b10 >= 0) *ascii++ = 48;

               *ascii = 0;

               /* Total buffer requirement (including the '\0') is
                * 5+precision - see check at the start.
                */
               return;
            }

            /* Here if an exponent is required, adjust size for
             * the digits we output but did not count.  The total
             * digit output here so far is at most 1+precision - no
             * decimal point and no leading or trailing zeros have
             * been output.
             */
            size -= cdigits;

            *ascii++ = 69, --size;    /* 'E': PLUS 1 TOTAL 2+precision */

            /* The following use of an unsigned temporary avoids ambiguities in
             * the signed arithmetic on exp_b10 and permits GCC at least to do
             * better optimization.
             */
            {
               unsigned int uexp_b10;

               if (exp_b10 < 0)
               {
                  *ascii++ = 45, --size; /* '-': PLUS 1 TOTAL 3+precision */
                  uexp_b10 = -exp_b10;
               }

               else
                  uexp_b10 = exp_b10;

               cdigits = 0;

               while (uexp_b10 > 0)
               {
                  exponent[cdigits++] = (char)(48 + uexp_b10 % 10);
                  uexp_b10 /= 10;
               }
            }

            /* Need another size check here for the exponent digits, so
             * this need not be considered above.
             */
            if ((int)size > cdigits)
            {
               while (cdigits > 0) *ascii++ = exponent[--cdigits];

               *ascii = 0;

               return;
            }
         }
      }
      else if (!(fp >= DBL_MIN))
      {
         *ascii++ = 48; /* '0' */
         *ascii = 0;
         return;
      }
      else
      {
         *ascii++ = 105; /* 'i' */
         *ascii++ = 110; /* 'n' */
         *ascii++ = 102; /* 'f' */
         *ascii = 0;
         return;
      }
   }

   /* Here on buffer too small. */
   png_error(png_ptr, "ASCII conversion buffer too small");
}

#  endif /* FLOATING_POINT */

#  ifdef PNG_FIXED_POINT_SUPPORTED
/* Function to format a fixed point value in ASCII.
 */
void /* PRIVATE */
png_ascii_from_fixed(png_const_structrp png_ptr, png_charp ascii,
    png_size_t size, png_fixed_point fp)
{
   /* Require space for 10 decimal digits, a decimal point, a minus sign and a
    * trailing \0, 13 characters:
    */
   if (size > 12)
   {
      png_uint_32 num;

      /* Avoid overflow here on the minimum integer. */
      if (fp < 0)
         *ascii++ = 45, --size, num = -fp;
      else
         num = fp;

      if (num <= 0x80000000) /* else overflowed */
      {
         unsigned int ndigits = 0, first = 16 /* flag value */;
         char digits[10];

         while (num)
         {
            /* Split the low digit off num: */
            unsigned int tmp = num/10;
            num -= tmp*10;
            digits[ndigits++] = (char)(48 + num);
            /* Record the first non-zero digit, note that this is a number
             * starting at 1, it's not actually the array index.
             */
            if (first == 16 && num > 0)
               first = ndigits;
            num = tmp;
         }

         if (ndigits > 0)
         {
            while (ndigits > 5) *ascii++ = digits[--ndigits];
            /* The remaining digits are fractional digits, ndigits is '5' or
             * smaller at this point.  It is certainly not zero.  Check for a
             * non-zero fractional digit:
             */
            if (first <= 5)
            {
               unsigned int i;
               *ascii++ = 46; /* decimal point */
               /* ndigits may be <5 for small numbers, output leading zeros
                * then ndigits digits to first:
                */
               i = 5;
               while (ndigits < i) *ascii++ = 48, --i;
               while (ndigits >= first) *ascii++ = digits[--ndigits];
               /* Don't output the trailing zeros! */
            }
         }
         else
            *ascii++ = 48;

         /* And null terminate the string: */
         *ascii = 0;
         return;
      }
   }

   /* Here on buffer too small. */
   png_error(png_ptr, "ASCII conversion buffer too small");
}
#   endif /* FIXED_POINT */
#endif /* READ_SCAL */

#if defined(PNG_FLOATING_POINT_SUPPORTED) && \
   !defined(PNG_FIXED_POINT_MACRO_SUPPORTED)
png_fixed_point
png_fixed(png_const_structrp png_ptr, double fp, png_const_charp text)
{
   double r = floor(100000 * fp + .5);

   if (r > 2147483647. || r < -2147483648.)
      png_fixed_error(png_ptr, text);

   return (png_fixed_point)r;
}
#endif

#if defined(PNG_READ_GAMMA_SUPPORTED) || \
    defined(PNG_INCH_CONVERSIONS_SUPPORTED) || defined(PNG__READ_pHYs_SUPPORTED)
/* muldiv functions */
/* This API takes signed arguments and rounds the result to the nearest
 * integer (or, for a fixed point number - the standard argument - to
 * the nearest .00001).  Overflow and divide by zero are signalled in
 * the result, a boolean - true on success, false on overflow.
 */
int
png_muldiv(png_fixed_point_p res, png_fixed_point a, png_int_32 times,
    png_int_32 divisor)
{
   /* Return a * times / divisor, rounded. */
   if (divisor != 0)
   {
      if (a == 0 || times == 0)
      {
         *res = 0;
         return 1;
      }
      else
      {
#ifdef PNG_FLOATING_ARITHMETIC_SUPPORTED
         double r = a;
         r *= times;
         r /= divisor;
         r = floor(r+.5);

         /* A png_fixed_point is a 32-bit integer. */
         if (r <= 2147483647. && r >= -2147483648.)
         {
            *res = (png_fixed_point)r;
            return 1;
         }
#else
         int negative = 0;
         png_uint_32 A, T, D;
         png_uint_32 s16, s32, s00;

         if (a < 0)
            negative = 1, A = -a;
         else
            A = a;

         if (times < 0)
            negative = !negative, T = -times;
         else
            T = times;

         if (divisor < 0)
            negative = !negative, D = -divisor;
         else
            D = divisor;

         /* Following can't overflow because the arguments only
          * have 31 bits each, however the result may be 32 bits.
          */
         s16 = (A >> 16) * (T & 0xffff) +
                           (A & 0xffff) * (T >> 16);
         /* Can't overflow because the a*times bit is only 30
          * bits at most.
          */
         s32 = (A >> 16) * (T >> 16) + (s16 >> 16);
         s00 = (A & 0xffff) * (T & 0xffff);

         s16 = (s16 & 0xffff) << 16;
         s00 += s16;

         if (s00 < s16)
            ++s32; /* carry */

         if (s32 < D) /* else overflow */
         {
            /* s32.s00 is now the 64-bit product, do a standard
             * division, we know that s32 < D, so the maximum
             * required shift is 31.
             */
            int bitshift = 32;
            png_fixed_point result = 0; /* NOTE: signed */

            while (--bitshift >= 0)
            {
               png_uint_32 d32, d00;

               if (bitshift > 0)
                  d32 = D >> (32-bitshift), d00 = D << bitshift;

               else
                  d32 = 0, d00 = D;

               if (s32 > d32)
               {
                  if (s00 < d00) --s32; /* carry */
                  s32 -= d32, s00 -= d00, result += 1<<bitshift;
               }

               else
                  if (s32 == d32 && s00 >= d00)
                     s32 = 0, s00 -= d00, result += 1<<bitshift;
            }

            /* Handle the rounding. */
            if (s00 >= (D >> 1))
               ++result;

            if (negative)
               result = -result;

            /* Check for overflow. */
            if ((negative && result <= 0) || (!negative && result >= 0))
            {
               *res = result;
               return 1;
            }
         }
#endif
      }
   }

   return 0;
}
#endif /* READ_GAMMA || INCH_CONVERSIONS */

#if defined(PNG_READ_GAMMA_SUPPORTED) || defined(PNG_INCH_CONVERSIONS_SUPPORTED)
/* The following is for when the caller doesn't much care about the
 * result.
 */
png_fixed_point
png_muldiv_warn(png_const_structrp png_ptr, png_fixed_point a, png_int_32 times,
    png_int_32 divisor)
{
   png_fixed_point result;

   if (png_muldiv(&result, a, times, divisor))
      return result;

   png_warning(png_ptr, "fixed point overflow ignored");
   return 0;
}
#endif

#ifdef PNG_READ_GAMMA_SUPPORTED /* more fixed point functions for gammma */
/* Calculate a reciprocal, return 0 on div-by-zero or overflow. */
png_fixed_point
png_reciprocal(png_fixed_point a)
{
#ifdef PNG_FLOATING_ARITHMETIC_SUPPORTED
   double r = floor(1E10/a+.5);

   if (r <= 2147483647. && r >= -2147483648.)
      return (png_fixed_point)r;
#else
   png_fixed_point res;

   if (png_muldiv(&res, 100000, 100000, a))
      return res;
#endif

   return 0; /* error/overflow */
}

/* A local convenience routine. */
static png_fixed_point
png_product2(png_fixed_point a, png_fixed_point b)
{
   /* The required result is 1/a * 1/b; the following preserves accuracy. */
#ifdef PNG_FLOATING_ARITHMETIC_SUPPORTED
   double r = a * 1E-5;
   r *= b;
   r = floor(r+.5);

   if (r <= 2147483647. && r >= -2147483648.)
      return (png_fixed_point)r;
#else
   png_fixed_point res;

   if (png_muldiv(&res, a, b, 100000))
      return res;
#endif

   return 0; /* overflow */
}

/* The inverse of the above. */
png_fixed_point
png_reciprocal2(png_fixed_point a, png_fixed_point b)
{
   /* The required result is 1/a * 1/b; the following preserves accuracy. */
#ifdef PNG_FLOATING_ARITHMETIC_SUPPORTED
   double r = 1E15/a;
   r /= b;
   r = floor(r+.5);

   if (r <= 2147483647. && r >= -2147483648.)
      return (png_fixed_point)r;
#else
   /* This may overflow because the range of png_fixed_point isn't symmetric,
    * but this API is only used for the product of file and screen gamma so it
    * doesn't matter that the smallest number it can produce is 1/21474, not
    * 1/100000
    */
   png_fixed_point res = png_product2(a, b);

   if (res != 0)
      return png_reciprocal(res);
#endif

   return 0; /* overflow */
}
#endif /* READ_GAMMA */

#ifdef PNG_CHECK_cHRM_SUPPORTED
/* Added at libpng version 1.2.34 (Dec 8, 2008) and 1.4.0 (Jan 2,
 * 2010: moved from pngset.c) */
/*
 *    Multiply two 32-bit numbers, V1 and V2, using 32-bit
 *    arithmetic, to produce a 64-bit result in the HI/LO words.
 *
 *                  A B
 *                x C D
 *               ------
 *              AD || BD
 *        AC || CB || 0
 *
 *    where A and B are the high and low 16-bit words of V1,
 *    C and D are the 16-bit words of V2, AD is the product of
 *    A and D, and X || Y is (X << 16) + Y.
*/

void /* PRIVATE */
png_64bit_product (long v1, long v2, unsigned long *hi_product,
    unsigned long *lo_product)
{
   int a, b, c, d;
   long lo, hi, x, y;

   a = (v1 >> 16) & 0xffff;
   b = v1 & 0xffff;
   c = (v2 >> 16) & 0xffff;
   d = v2 & 0xffff;

   lo = b * d;                   /* BD */
   x = a * d + c * b;            /* AD + CB */
   y = ((lo >> 16) & 0xffff) + x;

   lo = (lo & 0xffff) | ((y & 0xffff) << 16);
   hi = (y >> 16) & 0xffff;

   hi += a * c;                  /* AC */

   *hi_product = (unsigned long)hi;
   *lo_product = (unsigned long)lo;
}
#endif /* CHECK_cHRM */

#ifdef PNG_READ_GAMMA_SUPPORTED /* gamma table code */
#ifndef PNG_FLOATING_ARITHMETIC_SUPPORTED
/* Fixed point gamma.
 *
 * The code to calculate the tables used below can be found in the shell script
 * contrib/tools/intgamma.sh
 *
 * To calculate gamma this code implements fast log() and exp() calls using only
 * fixed point arithmetic.  This code has sufficient precision for either 8-bit
 * or 16-bit sample values.
 *
 * The tables used here were calculated using simple 'bc' programs, but C double
 * precision floating point arithmetic would work fine.  The programs are given
 * at the head of each table.
 *
 * 8-bit log table
 *   This is a table of -log(value/255)/log(2) for 'value' in the range 128 to
 *   255, so it's the base 2 logarithm of a normalized 8-bit floating point
 *   mantissa.  The numbers are 32-bit fractions.
 */
static const png_uint_32
png_8bit_l2[128] =
{
   4270715492U, 4222494797U, 4174646467U, 4127164793U, 4080044201U, 4033279239U,
   3986864580U, 3940795015U, 3895065449U, 3849670902U, 3804606499U, 3759867474U,
   3715449162U, 3671346997U, 3627556511U, 3584073329U, 3540893168U, 3498011834U,
   3455425220U, 3413129301U, 3371120137U, 3329393864U, 3287946700U, 3246774933U,
   3205874930U, 3165243125U, 3124876025U, 3084770202U, 3044922296U, 3005329011U,
   2965987113U, 2926893432U, 2888044853U, 2849438323U, 2811070844U, 2772939474U,
   2735041326U, 2697373562U, 2659933400U, 2622718104U, 2585724991U, 2548951424U,
   2512394810U, 2476052606U, 2439922311U, 2404001468U, 2368287663U, 2332778523U,
   2297471715U, 2262364947U, 2227455964U, 2192742551U, 2158222529U, 2123893754U,
   2089754119U, 2055801552U, 2022034013U, 1988449497U, 1955046031U, 1921821672U,
   1888774511U, 1855902668U, 1823204291U, 1790677560U, 1758320682U, 1726131893U,
   1694109454U, 1662251657U, 1630556815U, 1599023271U, 1567649391U, 1536433567U,
   1505374214U, 1474469770U, 1443718700U, 1413119487U, 1382670639U, 1352370686U,
   1322218179U, 1292211689U, 1262349810U, 1232631153U, 1203054352U, 1173618059U,
   1144320946U, 1115161701U, 1086139034U, 1057251672U, 1028498358U, 999877854U,
   971388940U, 943030410U, 914801076U, 886699767U, 858725327U, 830876614U,
   803152505U, 775551890U, 748073672U, 720716771U, 693480120U, 666362667U,
   639363374U, 612481215U, 585715177U, 559064263U, 532527486U, 506103872U,
   479792461U, 453592303U, 427502463U, 401522014U, 375650043U, 349885648U,
   324227938U, 298676034U, 273229066U, 247886176U, 222646516U, 197509248U,
   172473545U, 147538590U, 122703574U, 97967701U, 73330182U, 48790236U,
   24347096U, 0U

#if 0
   /* The following are the values for 16-bit tables - these work fine for the
    * 8-bit conversions but produce very slightly larger errors in the 16-bit
    * log (about 1.2 as opposed to 0.7 absolute error in the final value).  To
    * use these all the shifts below must be adjusted appropriately.
    */
   65166, 64430, 63700, 62976, 62257, 61543, 60835, 60132, 59434, 58741, 58054,
   57371, 56693, 56020, 55352, 54689, 54030, 53375, 52726, 52080, 51439, 50803,
   50170, 49542, 48918, 48298, 47682, 47070, 46462, 45858, 45257, 44661, 44068,
   43479, 42894, 42312, 41733, 41159, 40587, 40020, 39455, 38894, 38336, 37782,
   37230, 36682, 36137, 35595, 35057, 34521, 33988, 33459, 32932, 32408, 31887,
   31369, 30854, 30341, 29832, 29325, 28820, 28319, 27820, 27324, 26830, 26339,
   25850, 25364, 24880, 24399, 23920, 23444, 22970, 22499, 22029, 21562, 21098,
   20636, 20175, 19718, 19262, 18808, 18357, 17908, 17461, 17016, 16573, 16132,
   15694, 15257, 14822, 14390, 13959, 13530, 13103, 12678, 12255, 11834, 11415,
   10997, 10582, 10168, 9756, 9346, 8937, 8531, 8126, 7723, 7321, 6921, 6523,
   6127, 5732, 5339, 4947, 4557, 4169, 3782, 3397, 3014, 2632, 2251, 1872, 1495,
   1119, 744, 372
#endif
};

PNG_STATIC png_int_32
png_log8bit(unsigned int x)
{
   unsigned int lg2 = 0;
   /* Each time 'x' is multiplied by 2, 1 must be subtracted off the final log,
    * because the log is actually negate that means adding 1.  The final
    * returned value thus has the range 0 (for 255 input) to 7.994 (for 1
    * input), return 7.99998 for the overflow (log 0) case - so the result is
    * always at most 19 bits.
    */
   if ((x &= 0xff) == 0)
      return 0xffffffff;

   if ((x & 0xf0) == 0)
      lg2  = 4, x <<= 4;

   if ((x & 0xc0) == 0)
      lg2 += 2, x <<= 2;

   if ((x & 0x80) == 0)
      lg2 += 1, x <<= 1;

   /* result is at most 19 bits, so this cast is safe: */
   return (png_int_32)((lg2 << 16) + ((png_8bit_l2[x-128]+32768)>>16));
}

/* The above gives exact (to 16 binary places) log2 values for 8-bit images,
 * for 16-bit images we use the most significant 8 bits of the 16-bit value to
 * get an approximation then multiply the approximation by a correction factor
 * determined by the remaining up to 8 bits.  This requires an additional step
 * in the 16-bit case.
 *
 * We want log2(value/65535), we have log2(v'/255), where:
 *
 *    value = v' * 256 + v''
 *          = v' * f
 *
 * So f is value/v', which is equal to (256+v''/v') since v' is in the range 128
 * to 255 and v'' is in the range 0 to 255 f will be in the range 256 to less
 * than 258.  The final factor also needs to correct for the fact that our 8-bit
 * value is scaled by 255, whereas the 16-bit values must be scaled by 65535.
 *
 * This gives a final formula using a calculated value 'x' which is value/v' and
 * scaling by 65536 to match the above table:
 *
 *   log2(x/257) * 65536
 *
 * Since these numbers are so close to '1' we can use simple linear
 * interpolation between the two end values 256/257 (result -368.61) and 258/257
 * (result 367.179).  The values used below are scaled by a further 64 to give
 * 16-bit precision in the interpolation:
 *
 * Start (256): -23591
 * Zero  (257):      0
 * End   (258):  23499
 */
PNG_STATIC png_int_32
png_log16bit(png_uint_32 x)
{
   unsigned int lg2 = 0;

   /* As above, but now the input has 16 bits. */
   if ((x &= 0xffff) == 0)
      return 0xffffffff;

   if ((x & 0xff00) == 0)
      lg2  = 8, x <<= 8;

   if ((x & 0xf000) == 0)
      lg2 += 4, x <<= 4;

   if ((x & 0xc000) == 0)
      lg2 += 2, x <<= 2;

   if ((x & 0x8000) == 0)
      lg2 += 1, x <<= 1;

   /* Calculate the base logarithm from the top 8 bits as a 28-bit fractional
    * value.
    */
   lg2 <<= 28;
   lg2 += (png_8bit_l2[(x>>8)-128]+8) >> 4;

   /* Now we need to interpolate the factor, this requires a division by the top
    * 8 bits.  Do this with maximum precision.
    */
   x = ((x << 16) + (x >> 9)) / (x >> 8);

   /* Since we divided by the top 8 bits of 'x' there will be a '1' at 1<<24,
    * the value at 1<<16 (ignoring this) will be 0 or 1; this gives us exactly
    * 16 bits to interpolate to get the low bits of the result.  Round the
    * answer.  Note that the end point values are scaled by 64 to retain overall
    * precision and that 'lg2' is current scaled by an extra 12 bits, so adjust
    * the overall scaling by 6-12.  Round at every step.
    */
   x -= 1U << 24;

   if (x <= 65536U) /* <= '257' */
      lg2 += ((23591U * (65536U-x)) + (1U << (16+6-12-1))) >> (16+6-12);

   else
      lg2 -= ((23499U * (x-65536U)) + (1U << (16+6-12-1))) >> (16+6-12);

   /* Safe, because the result can't have more than 20 bits: */
   return (png_int_32)((lg2 + 2048) >> 12);
}

/* The 'exp()' case must invert the above, taking a 20-bit fixed point
 * logarithmic value and returning a 16 or 8-bit number as appropriate.  In
 * each case only the low 16 bits are relevant - the fraction - since the
 * integer bits (the top 4) simply determine a shift.
 *
 * The worst case is the 16-bit distinction between 65535 and 65534, this
 * requires perhaps spurious accuracty in the decoding of the logarithm to
 * distinguish log2(65535/65534.5) - 10^-5 or 17 bits.  There is little chance
 * of getting this accuracy in practice.
 *
 * To deal with this the following exp() function works out the exponent of the
 * frational part of the logarithm by using an accurate 32-bit value from the
 * top four fractional bits then multiplying in the remaining bits.
 */
static const png_uint_32
png_32bit_exp[16] =
{
   /* NOTE: the first entry is deliberately set to the maximum 32-bit value. */
   4294967295U, 4112874773U, 3938502376U, 3771522796U, 3611622603U, 3458501653U,
   3311872529U, 3171459999U, 3037000500U, 2908241642U, 2784941738U, 2666869345U,
   2553802834U, 2445529972U, 2341847524U, 2242560872U
};

/* Adjustment table; provided to explain the numbers in the code below. */
#if 0
for (i=11;i>=0;--i){ print i, " ", (1 - e(-(2^i)/65536*l(2))) * 2^(32-i), "\n"}
   11 44937.64284865548751208448
   10 45180.98734845585101160448
    9 45303.31936980687359311872
    8 45364.65110595323018870784
    7 45395.35850361789624614912
    6 45410.72259715102037508096
    5 45418.40724413220722311168
    4 45422.25021786898173001728
    3 45424.17186732298419044352
    2 45425.13273269940811464704
    1 45425.61317555035558641664
    0 45425.85339951654943850496
#endif

PNG_STATIC png_uint_32
png_exp(png_fixed_point x)
{
   if (x > 0 && x <= 0xfffff) /* Else overflow or zero (underflow) */
   {
      /* Obtain a 4-bit approximation */
      png_uint_32 e = png_32bit_exp[(x >> 12) & 0xf];

      /* Incorporate the low 12 bits - these decrease the returned value by
       * multiplying by a number less than 1 if the bit is set.  The multiplier
       * is determined by the above table and the shift. Notice that the values
       * converge on 45426 and this is used to allow linear interpolation of the
       * low bits.
       */
      if (x & 0x800)
         e -= (((e >> 16) * 44938U) +  16U) >> 5;

      if (x & 0x400)
         e -= (((e >> 16) * 45181U) +  32U) >> 6;

      if (x & 0x200)
         e -= (((e >> 16) * 45303U) +  64U) >> 7;

      if (x & 0x100)
         e -= (((e >> 16) * 45365U) + 128U) >> 8;

      if (x & 0x080)
         e -= (((e >> 16) * 45395U) + 256U) >> 9;

      if (x & 0x040)
         e -= (((e >> 16) * 45410U) + 512U) >> 10;

      /* And handle the low 6 bits in a single block. */
      e -= (((e >> 16) * 355U * (x & 0x3fU)) + 256U) >> 9;

      /* Handle the upper bits of x. */
      e >>= x >> 16;
      return e;
   }

   /* Check for overflow */
   if (x <= 0)
      return png_32bit_exp[0];

   /* Else underflow */
   return 0;
}

PNG_STATIC png_byte
png_exp8bit(png_fixed_point lg2)
{
   /* Get a 32-bit value: */
   png_uint_32 x = png_exp(lg2);

   /* Convert the 32-bit value to 0..255 by multiplying by 256-1, note that the
    * second, rounding, step can't overflow because of the first, subtraction,
    * step.
    */
   x -= x >> 8;
   return (png_byte)((x + 0x7fffffU) >> 24);
}

PNG_STATIC png_uint_16
png_exp16bit(png_fixed_point lg2)
{
   /* Get a 32-bit value: */
   png_uint_32 x = png_exp(lg2);

   /* Convert the 32-bit value to 0..65535 by multiplying by 65536-1: */
   x -= x >> 16;
   return (png_uint_16)((x + 32767U) >> 16);
}
#endif /* FLOATING_ARITHMETIC */

png_byte
png_gamma_8bit_correct(unsigned int value, png_fixed_point gamma_val)
{
   if (value > 0 && value < 255)
   {
#     ifdef PNG_FLOATING_ARITHMETIC_SUPPORTED
         double r = floor(255*pow(value/255.,gamma_val*.00001)+.5);
         return (png_byte)r;
#     else
         png_int_32 lg2 = png_log8bit(value);
         png_fixed_point res;

         if (png_muldiv(&res, gamma_val, lg2, PNG_FP_1))
            return png_exp8bit(res);

         /* Overflow. */
         value = 0;
#     endif
   }

   return (png_byte)value;
}

png_uint_16
png_gamma_16bit_correct(unsigned int value, png_fixed_point gamma_val)
{
   if (value > 0 && value < 65535)
   {
#     ifdef PNG_FLOATING_ARITHMETIC_SUPPORTED
         double r = floor(65535*pow(value/65535.,gamma_val*.00001)+.5);
         return (png_uint_16)r;
#     else
         png_int_32 lg2 = png_log16bit(value);
         png_fixed_point res;

         if (png_muldiv(&res, gamma_val, lg2, PNG_FP_1))
            return png_exp16bit(res);

         /* Overflow. */
         value = 0;
#     endif
   }

   return (png_uint_16)value;
}

/* This does the right thing based on the bit_depth field of the
 * png_struct, interpreting values as 8-bit or 16-bit.  While the result
 * is nominally a 16-bit value if bit depth is 8 then the result is
 * 8-bit (as are the arguments.)
 */
png_uint_16 /* PRIVATE */
png_gamma_correct(png_structrp png_ptr, unsigned int value,
    png_fixed_point gamma_val)
{
   if (png_ptr->bit_depth == 8)
      return png_gamma_8bit_correct(value, gamma_val);

   else
      return png_gamma_16bit_correct(value, gamma_val);
}

/* This is the shared test on whether a gamma value is 'significant' - whether
 * it is worth doing gamma correction.
 */
int /* PRIVATE */
png_gamma_significant(png_fixed_point gamma_val)
{
   return gamma_val < PNG_FP_1 - PNG_GAMMA_THRESHOLD_FIXED ||
       gamma_val > PNG_FP_1 + PNG_GAMMA_THRESHOLD_FIXED;
}

/* Internal function to build a single 16-bit table - the table consists of
 * 'num' 256 entry subtables, where 'num' is determined by 'shift' - the amount
 * to shift the input values right (or 16-number_of_signifiant_bits).
 *
 * The caller is responsible for ensuring that the table gets cleaned up on
 * png_error (i.e. if one of the mallocs below fails) - i.e. the *table argument
 * should be somewhere that will be cleaned.
 */
static void
png_build_16bit_table(png_structrp png_ptr, png_uint_16pp *ptable,
   PNG_CONST unsigned int shift, PNG_CONST png_fixed_point gamma_val)
{
   /* Various values derived from 'shift': */
   PNG_CONST unsigned int num = 1U << (8U - shift);
   PNG_CONST unsigned int max = (1U << (16U - shift))-1U;
   PNG_CONST unsigned int max_by_2 = 1U << (15U-shift);
   unsigned int i;

   png_uint_16pp table = *ptable =
       (png_uint_16pp)png_calloc(png_ptr, num * png_sizeof(png_uint_16p));

   for (i = 0; i < num; i++)
   {
      png_uint_16p sub_table = table[i] =
          (png_uint_16p)png_malloc(png_ptr, 256 * png_sizeof(png_uint_16));

      /* The 'threshold' test is repeated here because it can arise for one of
       * the 16-bit tables even if the others don't hit it.
       */
      if (png_gamma_significant(gamma_val))
      {
         /* The old code would overflow at the end and this would cause the
          * 'pow' function to return a result >1, resulting in an
          * arithmetic error.  This code follows the spec exactly; ig is
          * the recovered input sample, it always has 8-16 bits.
          *
          * We want input * 65535/max, rounded, the arithmetic fits in 32
          * bits (unsigned) so long as max <= 32767.
          */
         unsigned int j;
         for (j = 0; j < 256; j++)
         {
            png_uint_32 ig = (j << (8-shift)) + i;
#           ifdef PNG_FLOATING_ARITHMETIC_SUPPORTED
               /* Inline the 'max' scaling operation: */
               double d = floor(65535*pow(ig/(double)max, gamma_val*.00001)+.5);
               sub_table[j] = (png_uint_16)d;
#           else
               if (shift)
                  ig = (ig * 65535U + max_by_2)/max;

               sub_table[j] = png_gamma_16bit_correct(ig, gamma_val);
#           endif
         }
      }
      else
      {
         /* We must still build a table, but do it the fast way. */
         unsigned int j;

         for (j = 0; j < 256; j++)
         {
            png_uint_32 ig = (j << (8-shift)) + i;

            if (shift)
               ig = (ig * 65535U + max_by_2)/max;

            sub_table[j] = (png_uint_16)ig;
         }
      }
   }
}

/* NOTE: this function expects the *inverse* of the overall gamma transformation
 * required.
 */
static void
png_build_16to8_table(png_structrp png_ptr, png_uint_16pp *ptable,
   PNG_CONST unsigned int shift, PNG_CONST png_fixed_point gamma_val)
{
   PNG_CONST unsigned int num = 1U << (8U - shift);
   PNG_CONST unsigned int max = (1U << (16U - shift))-1U;
   unsigned int i;
   png_uint_32 last;

   png_uint_16pp table = *ptable =
       (png_uint_16pp)png_calloc(png_ptr, num * png_sizeof(png_uint_16p));

   /* 'num' is the number of tables and also the number of low bits of low
    * bits of the input 16-bit value used to select a table.  Each table is
    * itself index by the high 8 bits of the value.
    */
   for (i = 0; i < num; i++)
      table[i] = (png_uint_16p)png_malloc(png_ptr,
          256 * png_sizeof(png_uint_16));

   /* 'gamma_val' is set to the reciprocal of the value calculated above, so
    * pow(out,g) is an *input* value.  'last' is the last input value set.
    *
    * In the loop 'i' is used to find output values.  Since the output is
    * 8-bit there are only 256 possible values.  The tables are set up to
    * select the closest possible output value for each input by finding
    * the input value at the boundary between each pair of output values
    * and filling the table up to that boundary with the lower output
    * value.
    *
    * The boundary values are 0.5,1.5..253.5,254.5.  Since these are 9-bit
    * values the code below uses a 16-bit value in i; the values start at
    * 128.5 (for 0.5) and step by 257, for a total of 254 values (the last
    * entries are filled with 255).  Start i at 128 and fill all 'last'
    * table entries <= 'max'
    */
   last = 0;
   for (i = 0; i < 255; ++i) /* 8-bit output value */
   {
      /* Find the corresponding maximum input value */
      png_uint_16 out = (png_uint_16)(i * 257U); /* 16-bit output value */

      /* Find the boundary value in 16 bits: */
      png_uint_32 bound = png_gamma_16bit_correct(out+128U, gamma_val);

      /* Adjust (round) to (16-shift) bits: */
      bound = (bound * max + 32768U)/65535U + 1U;

      while (last < bound)
      {
         table[last & (0xffU >> shift)][last >> (8U - shift)] = out;
         last++;
      }
   }

   /* And fill in the final entries. */
   while (last < (num << 8))
   {
      table[last & (0xff >> shift)][last >> (8U - shift)] = 65535U;
      last++;
   }
}

/* Build a single 8-bit table: same as the 16-bit case but much simpler (and
 * typically much faster).  Note that libpng currently does no sBIT processing
 * (apparently contrary to the spec) so a 256 entry table is always generated.
 */
static void
png_build_8bit_table(png_structrp png_ptr, png_bytepp ptable,
   PNG_CONST png_fixed_point gamma_val)
{
   unsigned int i;
   png_bytep table = *ptable = (png_bytep)png_malloc(png_ptr, 256);

   if (png_gamma_significant(gamma_val)) for (i=0; i<256; i++)
      table[i] = png_gamma_8bit_correct(i, gamma_val);

   else for (i=0; i<256; ++i)
      table[i] = (png_byte)i;
}

/* Used from png_read_destroy and below to release the memory used by the gamma
 * tables.
 */
void /* PRIVATE */
png_destroy_gamma_table(png_structrp png_ptr)
{
   png_free(png_ptr, png_ptr->gamma_table);
   png_ptr->gamma_table = NULL;

   if (png_ptr->gamma_16_table != NULL)
   {
      int i;
      int istop = (1 << (8 - png_ptr->gamma_shift));
      for (i = 0; i < istop; i++)
      {
         png_free(png_ptr, png_ptr->gamma_16_table[i]);
      }
   png_free(png_ptr, png_ptr->gamma_16_table);
   png_ptr->gamma_16_table = NULL;
   }

#if defined(PNG_READ_BACKGROUND_SUPPORTED) || \
   defined(PNG_READ_ALPHA_MODE_SUPPORTED) || \
   defined(PNG_READ_RGB_TO_GRAY_SUPPORTED)
   png_free(png_ptr, png_ptr->gamma_from_1);
   png_ptr->gamma_from_1 = NULL;
   png_free(png_ptr, png_ptr->gamma_to_1);
   png_ptr->gamma_to_1 = NULL;

   if (png_ptr->gamma_16_from_1 != NULL)
   {
      int i;
      int istop = (1 << (8 - png_ptr->gamma_shift));
      for (i = 0; i < istop; i++)
      {
         png_free(png_ptr, png_ptr->gamma_16_from_1[i]);
      }
   png_free(png_ptr, png_ptr->gamma_16_from_1);
   png_ptr->gamma_16_from_1 = NULL;
   }
   if (png_ptr->gamma_16_to_1 != NULL)
   {
      int i;
      int istop = (1 << (8 - png_ptr->gamma_shift));
      for (i = 0; i < istop; i++)
      {
         png_free(png_ptr, png_ptr->gamma_16_to_1[i]);
      }
   png_free(png_ptr, png_ptr->gamma_16_to_1);
   png_ptr->gamma_16_to_1 = NULL;
   }
#endif /* READ_BACKGROUND || READ_ALPHA_MODE || RGB_TO_GRAY */
}

/* We build the 8- or 16-bit gamma tables here.  Note that for 16-bit
 * tables, we don't make a full table if we are reducing to 8-bit in
 * the future.  Note also how the gamma_16 tables are segmented so that
 * we don't need to allocate > 64K chunks for a full 16-bit table.
 */
void /* PRIVATE */
png_build_gamma_table(png_structrp png_ptr, int bit_depth)
{
  png_debug(1, "in png_build_gamma_table");

  /* Remove any existing table; this copes with multiple calls to
   * png_read_update_info.  The warning is because building the gamma tables
   * multiple times is a performance hit - it's harmless but the ability to call
   * png_read_update_info() multiple times is new in 1.5.6 so it seems sensible
   * to warn if the app introduces such a hit.
   */
  if (png_ptr->gamma_table != NULL || png_ptr->gamma_16_table != NULL)
  {
    png_warning(png_ptr, "gamma table being rebuilt");
    png_destroy_gamma_table(png_ptr);
  }

  if (bit_depth <= 8)
  {
     png_build_8bit_table(png_ptr, &png_ptr->gamma_table,
         png_ptr->screen_gamma > 0 ?  png_reciprocal2(png_ptr->gamma,
         png_ptr->screen_gamma) : PNG_FP_1);

#if defined(PNG_READ_BACKGROUND_SUPPORTED) || \
   defined(PNG_READ_ALPHA_MODE_SUPPORTED) || \
   defined(PNG_READ_RGB_TO_GRAY_SUPPORTED)
     if (png_ptr->transformations & (PNG_COMPOSE | PNG_RGB_TO_GRAY))
     {
        png_build_8bit_table(png_ptr, &png_ptr->gamma_to_1,
            png_reciprocal(png_ptr->gamma));

        png_build_8bit_table(png_ptr, &png_ptr->gamma_from_1,
            png_ptr->screen_gamma > 0 ?  png_reciprocal(png_ptr->screen_gamma) :
            png_ptr->gamma/* Probably doing rgb_to_gray */);
     }
#endif /* READ_BACKGROUND || READ_ALPHA_MODE || RGB_TO_GRAY */
  }
  else
  {
     png_byte shift, sig_bit;

     if (png_ptr->color_type & PNG_COLOR_MASK_COLOR)
     {
        sig_bit = png_ptr->sig_bit.red;

        if (png_ptr->sig_bit.green > sig_bit)
           sig_bit = png_ptr->sig_bit.green;

        if (png_ptr->sig_bit.blue > sig_bit)
           sig_bit = png_ptr->sig_bit.blue;
     }
     else
        sig_bit = png_ptr->sig_bit.gray;

     /* 16-bit gamma code uses this equation:
      *
      *   ov = table[(iv & 0xff) >> gamma_shift][iv >> 8]
      *
      * Where 'iv' is the input color value and 'ov' is the output value -
      * pow(iv, gamma).
      *
      * Thus the gamma table consists of up to 256 256 entry tables.  The table
      * is selected by the (8-gamma_shift) most significant of the low 8 bits of
      * the color value then indexed by the upper 8 bits:
      *
      *   table[low bits][high 8 bits]
      *
      * So the table 'n' corresponds to all those 'iv' of:
      *
      *   <all high 8-bit values><n << gamma_shift>..<(n+1 << gamma_shift)-1>
      *
      */
     if (sig_bit > 0 && sig_bit < 16U)
        shift = (png_byte)(16U - sig_bit); /* shift == insignificant bits */

     else
        shift = 0; /* keep all 16 bits */

     if (png_ptr->transformations & (PNG_16_TO_8 | PNG_SCALE_16_TO_8))
     {
        /* PNG_MAX_GAMMA_8 is the number of bits to keep - effectively
         * the significant bits in the *input* when the output will
         * eventually be 8 bits.  By default it is 11.
         */
        if (shift < (16U - PNG_MAX_GAMMA_8))
           shift = (16U - PNG_MAX_GAMMA_8);
     }

     if (shift > 8U)
        shift = 8U; /* Guarantees at least one table! */

     png_ptr->gamma_shift = shift;

#ifdef PNG_16BIT_SUPPORTED
     /* NOTE: prior to 1.5.4 this test used to include PNG_BACKGROUND (now
      * PNG_COMPOSE).  This effectively smashed the background calculation for
      * 16-bit output because the 8-bit table assumes the result will be reduced
      * to 8 bits.
      */
     if (png_ptr->transformations & (PNG_16_TO_8 | PNG_SCALE_16_TO_8))
#endif
         png_build_16to8_table(png_ptr, &png_ptr->gamma_16_table, shift,
         png_ptr->screen_gamma > 0 ? png_product2(png_ptr->gamma,
         png_ptr->screen_gamma) : PNG_FP_1);

#ifdef PNG_16BIT_SUPPORTED
     else
         png_build_16bit_table(png_ptr, &png_ptr->gamma_16_table, shift,
         png_ptr->screen_gamma > 0 ? png_reciprocal2(png_ptr->gamma,
         png_ptr->screen_gamma) : PNG_FP_1);
#endif

#if defined(PNG_READ_BACKGROUND_SUPPORTED) || \
   defined(PNG_READ_ALPHA_MODE_SUPPORTED) || \
   defined(PNG_READ_RGB_TO_GRAY_SUPPORTED)
     if (png_ptr->transformations & (PNG_COMPOSE | PNG_RGB_TO_GRAY))
     {
        png_build_16bit_table(png_ptr, &png_ptr->gamma_16_to_1, shift,
            png_reciprocal(png_ptr->gamma));

        /* Notice that the '16 from 1' table should be full precision, however
         * the lookup on this table still uses gamma_shift, so it can't be.
         * TODO: fix this.
         */
        png_build_16bit_table(png_ptr, &png_ptr->gamma_16_from_1, shift,
            png_ptr->screen_gamma > 0 ? png_reciprocal(png_ptr->screen_gamma) :
            png_ptr->gamma/* Probably doing rgb_to_gray */);
     }
#endif /* READ_BACKGROUND || READ_ALPHA_MODE || RGB_TO_GRAY */
  }
}
#endif /* READ_GAMMA */

/* sRGB support */
#if defined PNG_SIMPLIFIED_READ_SUPPORTED ||\
   defined PNG_SIMPLIFIED_WRITE_SUPPORTED
/* sRGB conversion tables; these are machine generated with the code in
 * contrib/tools/makesRGB.c.  The sRGB to linear table is exact (to the
 * nearest 16 bit linear fraction).  The inverse (linear to sRGB) table has
 * accuracies as follows:
 *
 * For all possible (255*65535+1) input values:
 *
 *    error: -0.515566 - 0.625971, 79441 (0.475369%) of readings inexact
 *
 * For the input values corresponding to the 65536 16-bit values:
 *
 *    error: -0.513727 - 0.607759, 308 (0.469978%) of readings inexact
 *
 * In all cases the inexact readings are off by one.
 */

#ifdef PNG_SIMPLIFIED_READ_SUPPORTED
/* The convert-to-sRGB table is only currently required for read. */
const png_uint_16 png_sRGB_table[256] =
{
   0,20,40,60,80,99,119,139,
   159,179,199,219,241,264,288,313,
   340,367,396,427,458,491,526,562,
   599,637,677,718,761,805,851,898,
   947,997,1048,1101,1156,1212,1270,1330,
   1391,1453,1517,1583,1651,1720,1790,1863,
   1937,2013,2090,2170,2250,2333,2418,2504,
   2592,2681,2773,2866,2961,3058,3157,3258,
   3360,3464,3570,3678,3788,3900,4014,4129,
   4247,4366,4488,4611,4736,4864,4993,5124,
   5257,5392,5530,5669,5810,5953,6099,6246,
   6395,6547,6700,6856,7014,7174,7335,7500,
   7666,7834,8004,8177,8352,8528,8708,8889,
   9072,9258,9445,9635,9828,10022,10219,10417,
   10619,10822,11028,11235,11446,11658,11873,12090,
   12309,12530,12754,12980,13209,13440,13673,13909,
   14146,14387,14629,14874,15122,15371,15623,15878,
   16135,16394,16656,16920,17187,17456,17727,18001,
   18277,18556,18837,19121,19407,19696,19987,20281,
   20577,20876,21177,21481,21787,22096,22407,22721,
   23038,23357,23678,24002,24329,24658,24990,25325,
   25662,26001,26344,26688,27036,27386,27739,28094,
   28452,28813,29176,29542,29911,30282,30656,31033,
   31412,31794,32179,32567,32957,33350,33745,34143,
   34544,34948,35355,35764,36176,36591,37008,37429,
   37852,38278,38706,39138,39572,40009,40449,40891,
   41337,41785,42236,42690,43147,43606,44069,44534,
   45002,45473,45947,46423,46903,47385,47871,48359,
   48850,49344,49841,50341,50844,51349,51858,52369,
   52884,53401,53921,54445,54971,55500,56032,56567,
   57105,57646,58190,58737,59287,59840,60396,60955,
   61517,62082,62650,63221,63795,64372,64952,65535
};

#endif /* simplified read only */

/* The base/delta tables are required for both read and write (but currently
 * only the simplified versions.)
 */
const png_uint_16 png_sRGB_base[512] =
{
   128,1782,3383,4644,5675,6564,7357,8074,
   8732,9346,9921,10463,10977,11466,11935,12384,
   12816,13233,13634,14024,14402,14769,15125,15473,
   15812,16142,16466,16781,17090,17393,17690,17981,
   18266,18546,18822,19093,19359,19621,19879,20133,
   20383,20630,20873,21113,21349,21583,21813,22041,
   22265,22487,22707,22923,23138,23350,23559,23767,
   23972,24175,24376,24575,24772,24967,25160,25352,
   25542,25730,25916,26101,26284,26465,26645,26823,
   27000,27176,27350,27523,27695,27865,28034,28201,
   28368,28533,28697,28860,29021,29182,29341,29500,
   29657,29813,29969,30123,30276,30429,30580,30730,
   30880,31028,31176,31323,31469,31614,31758,31902,
   32045,32186,32327,32468,32607,32746,32884,33021,
   33158,33294,33429,33564,33697,33831,33963,34095,
   34226,34357,34486,34616,34744,34873,35000,35127,
   35253,35379,35504,35629,35753,35876,35999,36122,
   36244,36365,36486,36606,36726,36845,36964,37083,
   37201,37318,37435,37551,37668,37783,37898,38013,
   38127,38241,38354,38467,38580,38692,38803,38915,
   39026,39136,39246,39356,39465,39574,39682,39790,
   39898,40005,40112,40219,40325,40431,40537,40642,
   40747,40851,40955,41059,41163,41266,41369,41471,
   41573,41675,41777,41878,41979,42079,42179,42279,
   42379,42478,42577,42676,42775,42873,42971,43068,
   43165,43262,43359,43456,43552,43648,43743,43839,
   43934,44028,44123,44217,44311,44405,44499,44592,
   44685,44778,44870,44962,45054,45146,45238,45329,
   45420,45511,45601,45692,45782,45872,45961,46051,
   46140,46229,46318,46406,46494,46583,46670,46758,
   46846,46933,47020,47107,47193,47280,47366,47452,
   47538,47623,47709,47794,47879,47964,48048,48133,
   48217,48301,48385,48468,48552,48635,48718,48801,
   48884,48966,49048,49131,49213,49294,49376,49458,
   49539,49620,49701,49782,49862,49943,50023,50103,
   50183,50263,50342,50422,50501,50580,50659,50738,
   50816,50895,50973,51051,51129,51207,51285,51362,
   51439,51517,51594,51671,51747,51824,51900,51977,
   52053,52129,52205,52280,52356,52432,52507,52582,
   52657,52732,52807,52881,52956,53030,53104,53178,
   53252,53326,53400,53473,53546,53620,53693,53766,
   53839,53911,53984,54056,54129,54201,54273,54345,
   54417,54489,54560,54632,54703,54774,54845,54916,
   54987,55058,55129,55199,55269,55340,55410,55480,
   55550,55620,55689,55759,55828,55898,55967,56036,
   56105,56174,56243,56311,56380,56448,56517,56585,
   56653,56721,56789,56857,56924,56992,57059,57127,
   57194,57261,57328,57395,57462,57529,57595,57662,
   57728,57795,57861,57927,57993,58059,58125,58191,
   58256,58322,58387,58453,58518,58583,58648,58713,
   58778,58843,58908,58972,59037,59101,59165,59230,
   59294,59358,59422,59486,59549,59613,59677,59740,
   59804,59867,59930,59993,60056,60119,60182,60245,
   60308,60370,60433,60495,60558,60620,60682,60744,
   60806,60868,60930,60992,61054,61115,61177,61238,
   61300,61361,61422,61483,61544,61605,61666,61727,
   61788,61848,61909,61969,62030,62090,62150,62211,
   62271,62331,62391,62450,62510,62570,62630,62689,
   62749,62808,62867,62927,62986,63045,63104,63163,
   63222,63281,63340,63398,63457,63515,63574,63632,
   63691,63749,63807,63865,63923,63981,64039,64097,
   64155,64212,64270,64328,64385,64443,64500,64557,
   64614,64672,64729,64786,64843,64900,64956,65013,
   65070,65126,65183,65239,65296,65352,65409,65465
};

const png_byte png_sRGB_delta[512] =
{
   207,201,158,129,113,100,90,82,77,72,68,64,61,59,56,54,
   52,50,49,47,46,45,43,42,41,40,39,39,38,37,36,36,
   35,34,34,33,33,32,32,31,31,30,30,30,29,29,28,28,
   28,27,27,27,27,26,26,26,25,25,25,25,24,24,24,24,
   23,23,23,23,23,22,22,22,22,22,22,21,21,21,21,21,
   21,20,20,20,20,20,20,20,20,19,19,19,19,19,19,19,
   19,18,18,18,18,18,18,18,18,18,18,17,17,17,17,17,
   17,17,17,17,17,17,16,16,16,16,16,16,16,16,16,16,
   16,16,16,16,15,15,15,15,15,15,15,15,15,15,15,15,
   15,15,15,15,14,14,14,14,14,14,14,14,14,14,14,14,
   14,14,14,14,14,14,14,13,13,13,13,13,13,13,13,13,
   13,13,13,13,13,13,13,13,13,13,13,13,13,13,12,12,
   12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
   12,12,12,12,12,12,12,12,12,12,12,12,11,11,11,11,
   11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,
   11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,
   11,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
   10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
   10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
   10,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
   9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
   9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
   9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
   9,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
   8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
   8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
   8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
   8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
   8,8,8,8,8,8,8,8,8,7,7,7,7,7,7,7,
   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
};
#endif /* SIMPLIFIED READ/WRITE sRGB support */

/* SIMPLIFIED READ/WRITE SUPPORT */
#if defined PNG_SIMPLIFIED_READ_SUPPORTED ||\
   defined PNG_SIMPLIFIED_WRITE_SUPPORTED
static int
png_image_free_function(png_voidp argument)
{
   png_imagep image = png_voidcast(png_imagep, argument);
   png_controlp cp = image->opaque;
   png_control c;

   /* Double check that we have a png_ptr - it should be impossible to get here
    * without one.
    */
   if (cp->png_ptr == NULL)
      return 0;

   /* First free any data held in the control structure. */
#  ifdef PNG_STDIO_SUPPORTED
      if (cp->owned_file)
      {
         FILE *fp = png_voidcast(FILE*, cp->png_ptr->io_ptr);
         cp->owned_file = 0;

         /* Ignore errors here. */
         if (fp != NULL)
         {
            cp->png_ptr->io_ptr = NULL;
            (void)fclose(fp);
         }
      }
#  endif

   /* Copy the control structure so that the original, allocated, version can be
    * safely freed.  Notice that a png_error here stops the remainder of the
    * cleanup, but this is probably fine because that would indicate bad memory
    * problems anyway.
    */
   c = *cp;
   image->opaque = &c;
   png_free(c.png_ptr, cp);

   /* Then the structures, calling the correct API. */
   if (c.for_write)
   {
#     ifdef PNG_SIMPLIFIED_WRITE_SUPPORTED
         png_destroy_write_struct(&c.png_ptr, &c.info_ptr);
#     else
         png_error(c.png_ptr, "simplified write not supported");
#     endif
   }
   else
   {
#     ifdef PNG_SIMPLIFIED_READ_SUPPORTED
         png_destroy_read_struct(&c.png_ptr, &c.info_ptr, NULL);
#     else
         png_error(c.png_ptr, "simplified read not supported");
#     endif
   }

   /* Success. */
   return 1;
}

void PNGAPI
png_image_free(png_imagep image)
{
   /* Safely call the real function, but only if doing so is safe at this point
    * (if not inside an error handling context).  Otherwise assume
    * png_safe_execute will call this API after the return.
    */
   if (image != NULL && image->opaque != NULL &&
      image->opaque->error_buf == NULL)
   {
      /* Ignore errors here: */
      (void)png_safe_execute(image, png_image_free_function, image);
      image->opaque = NULL;
   }
}

int /* PRIVATE */
png_image_error(png_imagep image, png_const_charp error_message)
{
   /* Utility to log an error. */
   png_safecat(image->message, sizeof image->message, 0, error_message);
   image->warning_or_error |= PNG_IMAGE_ERROR;
   png_image_free(image);
   return 0;
}

#endif /* SIMPLIFIED READ/WRITE */
#endif /* defined(PNG_READ_SUPPORTED) || defined(PNG_WRITE_SUPPORTED) */
