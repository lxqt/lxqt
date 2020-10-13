/*
 *      fm-module.c
 *
 *      This file is a part of the Libfm project.
 *
 *      Copyright 2013-2014 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2.1 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * SECTION:fm-module
 * @short_description: Simple external modules handler.
 * @title: FmModule
 *
 * @include: libfm/fm.h
 *
 * This implementation allows applications to use external modules and
 * select which ones application wants to use (from none to all).
 *
 * The naming scheme in examples below is strict. Replace "dummy" part in
 * them with your real name when you trying to use those examples. Those
 * strict example names are:
 * - FM_MODULE_dummy_VERSION
 * - fm_module_init_dummy
 * - fm_module_callback_dummy
 * - fm_module_register_dummy
 *
 * To use modules application should make few things. Let say, there is
 * some FmDummyWidget which wants to use "dummy" type of modules. First
 * thing application should do is create a header file which all modules
 * of that type should include:
 * <example id="example-fm-dummy-h">
 * <title>Sample of fm-dummy.h</title>
 * <programlisting><![CDATA[
 * #include <libfm/fm.h>
 *
 * #define FM_MODULE_dummy_VERSION 1
 *
 * typedef struct _FmDummyInit {
 *     int (*get_new)(const char *name);
 * } FmDummyInit;
 *
 * extern FmDummyInit fm_module_init_dummy;
 * ]]></programlisting>
 * </example>
 * The FM_MODULE_dummy_VERSION is a number which should be increased each
 * time something in FmDummyInit structure is changed. The FmDummyInit
 * represents an interface to module. It is specific for said module type.
 * The fm_module_init_dummy usage see below.
 *
 * Second thing application should do is to create implementation of the
 * module handling in your code:
 * <example id="example-fm-dummy-widget-c">
 * <title>Sample of fm-dummy-widget.c</title>
 * <programlisting><![CDATA[
 * #include "fm-dummy.h"
 *
 * FM_MODULE_DEFINE_TYPE(dummy, FmDummyInit, 1)
 *
 * static gboolean fm_module_callback_dummy(const char *name, gpointer init, int ver)
 * {
 *     /&ast; add module callbacks into own data list &ast;/
 *     .......
 * }
 *
 * .......
 * {
 *     FmDummyInit *module;
 *     int result = -1;
 *
 *     CHECK_MODULES();
 *     module = _find_module("test");
 *     if (module)
 *         result = module->get_new("test sample");
 *     return result;
 * }
 * ]]></programlisting>
 * </example>
 *
 * Third thing application should do is to register module type on the
 * application start, the same way as application calls fm_init() on the
 * start:
 * |[
 *     fm_module_register_dummy();
 * ]|
 * On application terminate it is adviced to unregister module type by
 * calling API fm_module_unregister_type() the same way as application
 * calls fm_finalize() on exit:
 * |[
 *     fm_module_unregister_type("dummy");
 * ]|
 *
 * The module itself will be easy to make. All you should do is to use
 * FM_DEFINE_MODULE() macro and implement callbacks for the module
 * interface (see the fm-dummy.h header example above):
 * <example id="example-fm-dummy-test-c">
 * <title>Sample of module dummy/test</title>
 * <programlisting><![CDATA[
 * #include "fm-dummy.h"
 *
 * FM_DEFINE_MODULE(dummy, test)
 *
 * static int fm_dummy_test_get_new(const char *name)
 * {
 *     /&ast; implementation &ast;/
 * }
 *
 * FmDummyInit fm_module_init_dummy = {
 *     fm_dummy_test_get_new;
 * };
 * ]]></programlisting>
 * </example>
 * The fm_module_init_dummy should be exactly the same structure that is
 * defined in the header file above.
 *
 * Note that modules are scanned and loaded only once per application
 * run for simplicity and reliability reasons (in fact, deletion of a
 * module that have some code running in another thread may lead to some
 * unpredictable problems). Therefore if you have any module changed you
 * have to restart the application before it see your change.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "fm-module.h"
#include "fm-config.h"
#include "fm-utils.h"

#include <string.h>
#include <dlfcn.h>

#include "../glib-compat.h"

volatile gint fm_modules_loaded = 0;

static guint idle_handler = 0;
G_LOCK_DEFINE_STATIC(idle_handler);

static GSList *m_dirs = NULL;

static gboolean _fm_modules_on_idle(gpointer user_data)
{
    /* check if it is destroyed already */
    if(g_source_is_destroyed(g_main_current_source()))
        return FALSE;
    G_LOCK(idle_handler);
    idle_handler = 0;
    G_UNLOCK(idle_handler);
    fm_modules_load();
    return FALSE;
}

#define FM_MODULE_TYPE             (fm_module_get_type())
#define FM_IS_MODULE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), FM_MODULE_TYPE))

typedef struct _FmModule            FmModule;
typedef struct _FmModuleClass       FmModuleClass;

struct _FmModule
{
    GObject parent;
    void *handle;
};

struct _FmModuleClass
{
    GObjectClass parent_class;
};

static GType fm_module_get_type(void);
static void fm_module_class_init(FmModuleClass *klass);

G_DEFINE_TYPE(FmModule, fm_module, G_TYPE_OBJECT);

static void fm_module_finalize(GObject *object)
{
    FmModule *self;

    g_return_if_fail(FM_IS_MODULE(object));
    self = (FmModule*)object;
    dlclose(self->handle);

    G_OBJECT_CLASS(fm_module_parent_class)->finalize(object);
}

static void fm_module_class_init(FmModuleClass *klass)
{
    GObjectClass *g_object_class;

    g_object_class = G_OBJECT_CLASS(klass);
    g_object_class->finalize = fm_module_finalize;
}

static void fm_module_init(FmModule *self)
{
}

static FmModule* fm_module_new(void)
{
    return (FmModule*)g_object_new(FM_MODULE_TYPE, NULL);
}


typedef struct _FmModuleType FmModuleType;
struct _FmModuleType
{
    FmModuleType *next;
    char *type;
    int minver;
    int maxver;
    FmModuleInitCallback cb;
    GSList *modules; /* members are FmModule */
};

static FmModuleType *modules_types = NULL;


/**
 * fm_module_register_type
 * @type: module type, unique for the application
 * @minver: minimum supported module version
 * @maxver: maximum supported module version
 * @cb: the callback used to inform about found module
 *
 * Registers @type into the modules loader. The modules loader will call
 * @cb routine when module that supports the @type was found within the
 * libfm modules directory. The scanning for modules will be done after
 * some timeout after last call to fm_module_register_type() so this API
 * should be used at application start before any possible modules usage
 * may appear.
 *
 * Since: 1.2.0
 */
void fm_module_register_type(const char *type, int minver, int maxver,
                             FmModuleInitCallback cb)
{
    FmModuleType *mtype;

    g_return_if_fail(type != NULL && cb != NULL);
    G_LOCK(idle_handler);
    if (fm_modules_loaded) /* it's too late... */
        goto _finish;
    for (mtype = modules_types; mtype; mtype = mtype->next)
        if (strcmp(type, mtype->type) == 0) /* already registered??? */
            goto _finish;
    mtype = g_slice_new(FmModuleType);
    mtype->next = modules_types;
    mtype->type = g_strdup(type);
    mtype->minver = minver;
    mtype->maxver = maxver;
    mtype->cb = cb;
    mtype->modules = NULL;
    modules_types = mtype;
    if (idle_handler > 0)
        g_source_remove(idle_handler);
    /* if not requested right away then delay loading for 3 seconds */
    idle_handler = g_timeout_add_seconds(3, _fm_modules_on_idle, NULL);

_finish:
    G_UNLOCK(idle_handler);
}

/**
 * fm_module_unregister_type
 * @type: module type, unique for the application
 *
 * Frees any resources that were allocated previously on the call to
 * fm_module_register_type() API, including code of the modules. After
 * this call any usage of data from callbacks will be invalid and the
 * most possibly lead to crash so it might be called only on finalizing
 * the application data.
 *
 * Since: 1.2.0
 */
void fm_module_unregister_type(const char *type)
{
    FmModuleType *mtype, *last;

    g_return_if_fail(type != NULL);
    G_LOCK(idle_handler);
    for (mtype = modules_types, last = NULL; mtype; mtype = mtype->next)
        if (strcmp(type, mtype->type) == 0)
            break;
        else
            last = mtype;
    g_assert(mtype != NULL);
    if (last)
        last->next = mtype->next;
    else /* it was first one */
        modules_types = mtype->next;
    if (modules_types == NULL)
        fm_modules_loaded = 0; /* FIXME: is this correct? */
    g_slist_free_full(mtype->modules, g_object_unref);
    G_UNLOCK(idle_handler);
    g_free(mtype->type);
    g_slice_free(FmModuleType, mtype);
}

static gboolean _name_matches(const char *name, const char *mask, const char *end)
{
    if (mask == end) /* end of pattern should match only to empty string */
        return (*name == '\0');
    if (*mask == '*') /* try to match any - zero to all */
    {
        mask++;
        while (1)
        {
            /* try to match first, rest of pattern still may match empty string */
            /* g_debug("matching %s to %.*s", name, (int)(end-mask), mask); */
            if (_name_matches(name, mask, end))
                return TRUE;
            if (*name == '\0') /* no matches so far */
                return FALSE;
            name++;
        }
        /* never reached */
    }
    if (*mask == '\\' && mask + 1 < end) /* next char is escaped */
        mask++;
    if (*name != *mask)
        return FALSE;
    return _name_matches(name + 1, mask + 1, end);
}

static gboolean _module_matches(const char *type, const char *name, const char *mask)
{
    const char *delimiter = strchr(mask, ':');
    if (delimiter)
    {
        /* g_debug("matching type %s to %.*s", type, (int)(delimiter-mask), mask); */
        if (!_name_matches(type, mask, delimiter))
            return FALSE;
        mask = delimiter + 1;
        delimiter = mask + strlen(mask);
        /* g_debug("matching name %s to %s", name, mask); */
        return _name_matches(name, mask, delimiter);
    }
    delimiter = mask + strlen(mask);
    return _name_matches(type, mask, delimiter);
}

static gboolean _fm_modules_load(gpointer unused)
{
    GDir *dir;
    const char *file;
    char **exp_list;
    const char *name;
    GString *str;
    void *handle;
    gint version;
    void *ptr;
    FmModule *module;
    FmModuleType *mtype;
    const char *dir_name;
    GSList *dir_l;

    g_debug("starting modules initialization");
    G_LOCK(idle_handler);
    dir_name = PACKAGE_MODULES_DIR;
    dir_l = m_dirs;
    str = g_string_sized_new(128);
    do
    {
        dir = g_dir_open(dir_name, 0, NULL);
        if (dir == NULL)
        {
            g_warning("modules directory is not accessible");
            goto _next_dir;
        }
        g_debug("scanning modules directory %s", dir_name);
        while ((file = g_dir_read_name(dir)) != NULL)
        {
            if (!g_str_has_suffix(file, ".so")) /* ignore other files */
                continue;
            g_debug("found module file: %s", file);
            g_string_printf(str, "%s/%s", dir_name, file);
            handle = dlopen(str->str, RTLD_NOW);
            if (handle == NULL) /* broken file */
                continue;
            name = dlsym(handle, "module_name");
            if (name == NULL) /* no name found */
                continue;
            module = fm_module_new();
            module->handle = handle;
            for (mtype = modules_types; mtype; mtype = mtype->next)
            {
                /* test each file name - whitelist and blacklist */
                if (fm_config->system_modules_blacklist || fm_config->modules_blacklist)
                {
                    exp_list = fm_config->system_modules_blacklist;
                    if (exp_list)
                        for ( ; *exp_list; exp_list++)
                            if (_module_matches(mtype->type, name, *exp_list))
                                break;
                    if (!exp_list || (!*exp_list && fm_config->modules_blacklist))
                        for (exp_list = fm_config->modules_blacklist; *exp_list; exp_list++)
                            if (_module_matches(mtype->type, name, *exp_list))
                                break;
                    if (*exp_list) /* found in blacklist */
                    {
                        if (!fm_config->modules_whitelist) /* no whitelist */
                            continue;
                        for (exp_list = fm_config->modules_whitelist; *exp_list; exp_list++)
                            if (_module_matches(mtype->type, name, *exp_list))
                                break;
                        if (*exp_list == NULL) /* not matches whitelist */
                            continue;
                    }
                }
                /* test version */
                g_string_printf(str, "module_%s_version", mtype->type);
                ptr = dlsym(handle, str->str);
                if (ptr == NULL)
                    continue;
                version = *(int *)ptr;
                if (version < mtype->minver || version > mtype->maxver)
                    /* version mismatched */
                    continue;
                /* test interface */
                g_string_printf(str, "fm_module_init_%s", mtype->type);
                ptr = dlsym(handle, str->str);
                if (ptr == NULL) /* no interface found */
                    continue;
                g_debug("found handler %s:%s", mtype->type, name);
                /* if everything is ok then add to list */
                if (mtype->cb(name, ptr, version))
                    mtype->modules = g_slist_prepend(mtype->modules, g_object_ref(module));
            }
            g_object_unref(module);
        }
        g_dir_close(dir);
_next_dir:
        if (dir_l)
        {
            dir_name = dir_l->data;
            dir_l = dir_l->next;
            continue;
        }
        break;
    } while(1);
    g_slist_free_full(m_dirs, g_free);
    m_dirs = NULL;
    G_UNLOCK(idle_handler);
    g_string_free(str, TRUE);
    g_debug("done with modules");
    return FALSE;
}

/**
 * fm_modules_load
 *
 * Forces scanning the libfm modules for existing modules. Any calls to
 * fm_module_register_type() after this will have no effect.
 *
 * Since: 1.2.0
 */
void fm_modules_load(void)
{
    if (!g_atomic_int_compare_and_exchange(&fm_modules_loaded, 0, 1))
        return;
    fm_run_in_default_main_context(_fm_modules_load, NULL);
}

/**
 * fm_module_is_in_use
 * @type: module type
 * @name: (allow-none): module key
 *
 * Checks if specified module is found and successfully loaded.
 *
 * Returns: %TRUE if module is in use.
 *
 * Since: 1.2.0
 */
gboolean fm_module_is_in_use(const char *type, const char *name)
{
    FmModuleType *mtype;
    GSList *l;

    if (type == NULL)
        return FALSE;
    CHECK_MODULES();
    for (mtype = modules_types; mtype; mtype = mtype->next)
        if (strcmp(mtype->type, type) == 0)
            break;
    if (!mtype)
        return FALSE;
    if (!name)
        return TRUE;
    for (l = mtype->modules; l; l = l->next)
    {
        FmModule *module = l->data;
        if (g_strcmp0(name, dlsym(module->handle, "module_name")) == 0)
            break;
    }
    return (l != NULL);
}

/**
 * fm_modules_add_directory
 * @path: absolute path to modules directory
 *
 * Adds an application-specific directory @path to be used later for
 * scanning for modules. The @path should be absolute UNIX path.
 *
 * Returns: %TRUE if @path was added successfully.
 *
 * Since: 1.2.0
 */
gboolean fm_modules_add_directory(const char *path)
{
    GSList *l;
    gboolean res = FALSE;

    g_return_val_if_fail(path != NULL && path[0] == '/', FALSE);
    G_LOCK(idle_handler);
    if (fm_modules_loaded) /* it's too late... */
        goto _finish;
    for (l = m_dirs; l; l = l->next)
        if (strcmp(l->data, path) == 0)
            break;
    if (l == NULL)
        m_dirs = g_slist_append(m_dirs, g_strdup(path));
    res = TRUE;
_finish:
    G_UNLOCK(idle_handler);
    return res;
}
