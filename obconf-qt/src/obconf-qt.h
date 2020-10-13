/* -*- indent-tabs-mode: nil; tab-width: 4; c-basic-offset: 4; -*-
 * 
 * main.h for ObConf, the configuration tool for Openbox
 * Copyright (c) 2003        Dana Jansens
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * See the COPYING file for a copy of the GNU General Public License.
 */

#ifndef obconf_qt_H
#define obconf_qt_H

#include <obrender/render.h>
#include <obrender/instance.h>
#include <obt/xml.h>
#include <obt/paths.h>

extern RrInstance *rrinst;
extern gchar *obc_config_file;
extern ObtPaths *paths;
extern ObtXmlInst *parse_i;

#endif // obconf_qt_H
