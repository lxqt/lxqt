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

#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "libbacklight_backend.h"
#include "libbacklight_backend.c"

#define True 1
#define False 0

/**bl_power turn off or turn on backlight.
 * @param driver is the driver to use
 * @param value 0 turns on backlight, 4 turns off backlight
 */
static void set_bl_power(const char *driver, int value)
{
    FILE *out = open_driver_file("bl_power", driver, "w");
    if( out != NULL ) {
        fprintf(out, "%d", value);
        fclose(out);
    }
}


static void set_backlight(const char *driver, int value)
{
    if(value>0) {
        FILE *out = open_driver_file("brightness", driver, "w");
        if( out != NULL ) {
            fprintf(out, "%d", value);
            fclose(out);
        }
        if(read_bl_power(driver) > 0)
            set_bl_power(driver, 0);
    } else {
        set_bl_power(driver, 4);
    }
}

static char *get_driver()
{
    return lxqt_backlight_backend_get_driver();
}


static void show_blacklight()
{
    char *driver = get_driver();
    if( driver == NULL ) {
        return;
    }
    int max_value = read_max_backlight(driver);
    int actual = read_backlight(driver);
    printf("%s %d %d\n", driver, max_value, actual);
    free(driver);
}

static void change_blacklight(int value, int percent_ok)
{
    char *driver = get_driver();
    if( driver == NULL ) {
        return;
    }
    int max_value = read_max_backlight(driver);
    if(percent_ok) {
        value = (float)(max_value*value)/100.0;
        if( value == 0 ) {
            // avoid switching off backlight but support zero as lowest value
            value = 1;
        }
    }
    if(value<=max_value && value>0) {
        set_backlight(driver, value);
    }
    free(driver);
}

static void increases_blacklight()
{
    char *driver = get_driver();
    if( driver == NULL ) {
        return;
    }
    int max_value = read_max_backlight(driver);
    int actual = read_backlight(driver);
    int incr = max_value/10;
    if( incr == 0 )
        incr = 1;
    int value = actual + incr;
    if( value > max_value)
        value = max_value;
    if(value<max_value && value>0) {
        set_backlight(driver, value);
    }
    free(driver);
}

static void decreases_blacklight()
{
    char *driver = get_driver();
    if( driver == NULL ) {
        return;
    }
    int max_value = read_max_backlight(driver);
    int actual = read_backlight(driver);
    int decr = max_value/10;
    if( decr == 0 )
        decr = 1;
    int value = actual - decr;
    if( value <= 0 )
        value = 1;
    if(value<max_value) {
        set_backlight(driver, value);
    }
    free(driver);
}

static void set_backlight_from_stdin()
{
    char *driver = get_driver();
    int ok = True, value;
    int max_value = read_max_backlight(driver);
    if( driver == NULL ) {
        return;
    }
    while(ok && !feof(stdin)) {
        ok = scanf("%d", &value);
        if( ok != EOF && value > 0 && value <= max_value) {
            set_backlight(driver, value);
        }
    }
    free(driver);
}

static void help(char *argv0)
{
    printf("%s [backlight-level [ %% ]] [--help]\n"
        "--help             Shows this message.\n"
        "--show             Shows actual brightness level.\n"
        "--inc              Increases actual brightness level.\n"
        "--dec              Decreases actual brightness level.\n"
        "--stdin            Read backlight value from stdin\n"
        "backlight-level    Sets backlight\n"
        "backlight-level %%  Sets backlight from 1%% to 100%%\n"
        "This tool changes screen backlight.\n"
        "Example:\n"
        "%s 10 %%       Sets backlight level until 10%%.\n"
        , argv0, argv0
    );
}


int main(int argc, char *argv[])
{
    int value = -1, value_percent_ok = False;
    int n;
    for(n=1; n<argc; n++) {
        if( !strcmp(argv[n], "--help") ) {
            help(argv[0]);
            return 0;
        } if( !strcmp(argv[n], "--show") ) {
            show_blacklight();
            return 0;
        } if( !strcmp(argv[n], "--inc") ) {
            increases_blacklight();
            return 0;
        } if( !strcmp(argv[n], "--dec") ) {
            decreases_blacklight();
            return 0;
        } if( !strcmp(argv[n], "--stdin") ) {
            set_backlight_from_stdin();
            return 0;
        } else if ( !strcmp(argv[n], "%") ) {
            value_percent_ok = True;
        } else if ( isdigit(argv[n][0]) ) {
            value = atoi(argv[n]);
        } else {
            help(argv[0]);
            return 0;
        }
    }

    if( argc == 1 ) {
        help(argv[0]);
        return 0;
    }

    change_blacklight(value, value_percent_ok);

    return 0;
}
