/*-
 * sRGB.h
 *
 * Last changed in libpng 1.5.7 [(PENDING RELEASE)]
 * Copyright (c) 2011 John Cunningham Bowler
 *
 * This code is released under the libpng license.
 * For conditions of distribution and use, see the disclaimer
 * and license in png.h
 *
 * Utility file; not actually a header, this contains definitions of sRGB
 * calculation functions for inclusion in those test programs that need them.
 *
 * All routines take and return a floating point value in the range
 * 0 to 1.0, doing a calculation according to the sRGB specification
 * (in fact the source of the numbers is the wikipedia article at
 * http://en.wikipedia.org/wiki/SRGB).
 */
static double
sRGB_from_linear(double l)
{
   if (l <= 0.0031308)
      l *= 12.92;

   else
      l = 1.055 * pow(l, 1/2.4) - 0.055;

   return l;
}

static double
linear_from_sRGB(double s)
{
   if (s <= 0.04045)
      return s / 12.92;

   else
      return pow((s+0.055)/1.055, 2.4);
}

static double
YfromRGB(double r, double g, double b)
{
   /* Use the sRGB (rounded) coefficients for Rlinear, Glinear, Blinear to get
    * the CIE Y value (also linear).
    */
   return 0.2126 * r + 0.7152 * g + 0.0722 * b;
}
