/* minwrpngconf.h: headers to make a minimal png-write-only library
 *
 * Copyright (c) 2007, 2010 Glenn Randers-Pehrson
 *
 * This code is released under the libpng license.
 * For conditions of distribution and use, see the disclaimer
 * and license in png.h
 *
 * Derived from pngcrush.h, Copyright 1998-2007, Glenn Randers-Pehrson
 */

#ifndef MINWRPNGCONF_H
#define MINWRPNGCONF_H

/* If pngusr.h is included during the build the following must
 * be defined either here or in the .dfa file (pngusr.dfa in
 * this case).  To include pngusr.h set -DPNG_USER_CONFIG in
 * CPPFLAGS
 */
#define PNG_USER_PRIVATEBUILD "libpng minimal conformant PNG encoder"
#define PNG_USER_DLLFNAME_POSTFIX "me"

/* List options to turn off features of the build that do not
 * affect the API (so are not recorded in pnglibconf.h)
 */
#define PNG_NO_WARNINGS

#endif /* MINWRPNGCONF_H */
