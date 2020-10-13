/*
 * fr-command-lrzip.h
 *
 *  Created on: 10.04.2010
 *      Author: Alexander Saprykin
 */

#ifndef FRCOMMANDLRZIP_H_
#define FRCOMMANDLRZIP_H_

#include <glib.h>
#include "fr-command.h"
#include "fr-process.h"

#define FR_TYPE_COMMAND_LRZIP            (fr_command_lrzip_get_type ())
#define FR_COMMAND_LRZIP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), FR_TYPE_COMMAND_LRZIP, FrCommandLrzip))
#define FR_COMMAND_LRZIP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), FR_TYPE_COMMAND_LRZIP, FrCommandLrzipClass))
#define FR_IS_COMMAND_LRZIP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FR_TYPE_COMMAND_LRZIP))
#define FR_IS_COMMAND_LRZIP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), FR_TYPE_COMMAND_LRZIP))
#define FR_COMMAND_LRZIP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), FR_TYPE_COMMAND_LRZIP, FrCommandLrzipClass))

typedef struct _FrCommandLrzip       FrCommandLrzip;
typedef struct _FrCommandLrzipClass  FrCommandLrzipClass;

struct _FrCommandLrzip
{
	FrCommand  __parent;
};

struct _FrCommandLrzipClass
{
	FrCommandClass __parent_class;
};

GType fr_command_lrzip_get_type (void);

#endif /* FRCOMMANDLRZIP_H_ */
