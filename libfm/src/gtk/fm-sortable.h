/*
 *      fm-sortable.h
 *
 *      Copyright 2012 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *      Copyright 2012 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

#ifndef _FM_SORTABLE_H_
#define _FM_SORTABLE_H_ 1
G_BEGIN_DECLS

/**
 * FmSortMode:
 * @FM_SORT_ASCENDING: sort ascending, mutually exclusive with FM_SORT_DESCENDING
 * @FM_SORT_DESCENDING: sort descending, mutually exclusive with FM_SORT_ASCENDING
 * @FM_SORT_CASE_SENSITIVE: case sensitive file names sort
 * @FM_SORT_NO_FOLDER_FIRST: (since 1.2.0) don't sort folders before files
 * @FM_SORT_ORDER_MASK: (FM_SORT_ASCENDING|FM_SORT_DESCENDING)
 *
 * Sort mode flags supported by FmFolderModel
 *
 * Since: 1.0.2
 */
typedef enum{
    FM_SORT_ASCENDING = 0,
    FM_SORT_DESCENDING = 1 << 0,
    FM_SORT_CASE_SENSITIVE = 1 << 1,
    FM_SORT_NO_FOLDER_FIRST = 1 << 2,
    FM_SORT_ORDER_MASK = (FM_SORT_ASCENDING|FM_SORT_DESCENDING)
} FmSortMode;

/**
 * FM_SORT_DEFAULT:
 *
 * value which means do not change sorting mode flags.
 */
#define FM_SORT_DEFAULT ((FmSortMode)-1)

#define FM_SORT_IS_ASCENDING(mode) ((mode & FM_SORT_ORDER_MASK) == FM_SORT_ASCENDING)

G_END_DECLS
#endif /* _FM_SORTABLE_H_ */
