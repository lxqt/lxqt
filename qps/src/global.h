/*
 * stable.h
 * This file is part of qps -- Qt-based visual process status monitor
 *
 * Copyright 2014 dae hyun, yang <daehyun.yang@gmail.com>
 * Copyright 2015 Paulo Lieuthier <paulolieuthier@gmail.com>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#ifndef GLOBAL_H
#define GLOBAL_H

#include "qps.h"
#include "misc.h"

extern QList<Command *> commands;
extern ControlBar *controlbar;
extern int default_font_height;
extern bool flag_show_thread;
extern int flag_thread_ok;
extern bool previous_flag_show_thread;
extern int num_opened_files;

extern Qps *qps;
extern SearchBox *search_box;
extern TFrame *infobox;

#endif // GLOBAL_H
