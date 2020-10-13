/*
 * Copyright (C) 2016  P.L. Lucas
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef __LXQTBACKLIGHT_BACKEND_H__
#define __LXQTBACKLIGHT_BACKEND_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

/**Returns actual value of backlight. 
 * -1 will be returned if backlight can not be changed.
 */
int lxqt_backlight_backend_get();

/**Returns maximum value of backlight.
 * -1 will be returned if backlight can not be changed.
 */
int lxqt_backlight_backend_get_max();

/**Returns a FILE pointer to stream which can be used to write values 
 * of backlight.
 *      int max_backlight = lxqt_backlight_backend_get_max();
 *      if(max_backlight<0)
 *          return; // Backlight can not be controlled.
 *      FILE *fout = lxqt_backlight_backend_write_stream();
 *      fprintf(fout, "%d\n", 3);
 *      fflush(fout);
 *      // ... Do something ...
 *      fprintf(fout, "%d\n", 7);
 *      fclose(fout);
 * Under Qt you can use QTextStream class:
 *      FILE *fout = lxqt_backlight_backend_write_stream();
 *      QTextStream backlightStream(fout);
 *      backlightStream << 3 << endl;
 */
FILE *lxqt_backlight_backend_get_write_stream();

/**Returns if backlight power is turned off.
 * @ return 0 backlight off, bigger then 0 backlight on.
 */
int lxqt_backlight_is_backlight_off();

/**Returns the driver. Backlight values are read from /sys/class/backlight/driver/.
 * Example:
 *      char *driver = lxqt_backlight_backend_get_driver();
 *      printf("%s\n", driver);
 *      free(driver);
 */
char *lxqt_backlight_backend_get_driver();

#ifdef __cplusplus
}
#endif


#endif