#ifndef TR_WRAPPER_H
#define TR_WRAPPER_H

#include <glib.h>
#include <glib/gi18n.h>

G_BEGIN_DECLS

/* This is a function pointer point to a gettext replacement which
 * wraps QTraslator and calls QApplication::translate() internally. */

extern const char* (*qt_gettext)(const char* msg);

#undef _
#undef N_

#define _(x)    qt_gettext(x)
#define N_(x)   (x)

G_END_DECLS

#endif /* TR_WRAPPER_H */
