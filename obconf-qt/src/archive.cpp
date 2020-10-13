#include <QMessageBox>

#include <glib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

static gchar *get_theme_dir();
static gboolean change_dir(const gchar *dir);
static gchar* name_from_dir(const gchar *dir);
static gchar* install_theme_to(const gchar *file, const gchar *to);
static gboolean create_theme_archive(const gchar *dir, const gchar *name,
                                     const gchar *to);

gchar* archive_install(const gchar *path)
{
    gchar *dest;
    gchar *name;

    if (!(dest = get_theme_dir()))
        return NULL;

    if ((name = install_theme_to(path, dest))) {
      QMessageBox::information(NULL, QString(), QObject::tr("\"%1\" was installed to %2")
        .arg(QString::fromUtf8(name))
        .arg(QString::fromUtf8(dest)));
    }

    g_free(dest);

    return name;
}

void archive_create(const gchar *path)
{
    gchar *name;
    gchar *dest;

    if (!(name = name_from_dir(path)))
        return;

    {
        gchar *file;
        file = g_strdup_printf("%s.obt", name);
        dest = g_build_path(G_DIR_SEPARATOR_S,
                            g_get_current_dir(), file, NULL);
        g_free(file);
    }

    if (create_theme_archive(path, name, dest))
      QMessageBox::information(NULL, QString(), QObject::tr("\"%1\" was successfully created")
        .arg(QString::fromUtf8(dest)));

    g_free(dest);
    g_free(name);
}

static gboolean create_theme_archive(const gchar *dir, const gchar *name,
                                     const gchar *to)
{
    gchar *glob;
    gchar **argv;
    gchar *errtxt = NULL;
    gchar *parentdir;
    gint exitcode;
    GError *e = NULL;

    glob = g_strdup_printf("%s/openbox-3/", name);

    parentdir = g_build_path(G_DIR_SEPARATOR_S, dir, "..", NULL);

    argv = g_new(gchar*, 9);
    argv[0] = g_strdup("tar");
    argv[1] = g_strdup("-c");
    argv[2] = g_strdup("-z");
    argv[3] = g_strdup("-f");
    argv[4] = g_strdup(to);
    argv[5] = g_strdup("-C");
    argv[6] = g_strdup(parentdir);
    argv[7] = g_strdup(glob);
    argv[8] = NULL;
    if (g_spawn_sync(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL,
                     NULL, &errtxt, &exitcode, &e))
    {
        if (exitcode != EXIT_SUCCESS)
            QMessageBox::critical(NULL, QString(),
                    QObject::tr("Unable to create the theme archive \"%1\".\nThe following errors were reported:\n%2")
                      .arg(QString::fromUtf8(to))
                      .arg(QString::fromUtf8(errtxt)));

    }
    else
      QMessageBox::critical(NULL, QString(),QObject::tr("Unable to run the \"tar\" command: %1")
        .arg(QString::fromUtf8(e->message)));

    g_strfreev(argv);
    if (e) g_error_free(e);
    g_free(errtxt);
    g_free(parentdir);
    g_free(glob);
    return exitcode == EXIT_SUCCESS;
}

static gchar *get_theme_dir()
{
    gchar *dir;
    gint r;

    dir = g_build_path(G_DIR_SEPARATOR_S, g_get_home_dir(), ".themes", NULL);
    r = mkdir(dir, 0777);
    if (r == -1 && errno != EEXIST) {
      QMessageBox::critical(NULL, QString(),
                QObject::tr("Unable to create directory \"%1\": %2")
                .arg(QString::fromUtf8(dir))
                .arg(QString::fromUtf8(strerror(errno))));
        g_free(dir);
        dir = NULL;
    }

    return dir;
}

static gchar* name_from_dir(const gchar *dir)
{
    gchar *rc;
    struct stat st;
    gboolean r;

    rc = g_build_path(G_DIR_SEPARATOR_S, dir, "openbox-3", "themerc", NULL);

    r = (stat(rc, &st) == 0 && S_ISREG(st.st_mode));
    g_free(rc);

    if (!r) {
        QMessageBox::critical(NULL, QString(),
                QObject::tr("\"%1\" does not appear to be a valid Openbox theme directory")
                  .arg(QString::fromUtf8(dir)));
        return NULL;
    }
    return g_path_get_basename(dir);
}

static gboolean change_dir(const gchar *dir)
{
    if (chdir(dir) == -1) {
        QMessageBox::critical(NULL, QString(), QObject::tr("Unable to move to directory \"%1\": %2")
          .arg(QString::fromUtf8(dir))
          .arg(QString::fromUtf8(strerror(errno))));
        return FALSE;
    }
    return TRUE;
}

static gchar* install_theme_to(const gchar *file, const gchar *to)
{
    gchar **argv;
    gchar *errtxt = NULL, *outtxt = NULL;
    gint exitcode;
    GError *e = NULL;
    gchar *name = NULL;

    argv = g_new(gchar*, 11);
    argv[0] = g_strdup("tar");
    argv[1] = g_strdup("-x");
    argv[2] = g_strdup("-v");
    argv[3] = g_strdup("-z");
    argv[4] = g_strdup("--wildcards");
    argv[5] = g_strdup("-f");
    argv[6] = g_strdup(file);
    argv[7] = g_strdup("-C");
    argv[8] = g_strdup(to);
    argv[9] = g_strdup("*/openbox-3/");
    argv[10] = NULL;
    if (!g_spawn_sync(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL,
                      &outtxt, &errtxt, &exitcode, &e))
        QMessageBox::critical(NULL, QString(), QObject::tr("Unable to run the \"tar\" command: %1")
            .arg(QString::fromUtf8(e->message)));
    g_strfreev(argv);
    if (e) g_error_free(e);

    if (exitcode != EXIT_SUCCESS)
        QMessageBox::critical(NULL, QString(),
                QObject::tr("Unable to extract the file \"%1\".\nPlease ensure that \"%2\" is writable and that the file is a valid Openbox theme archive.\nThe following errors were reported:\n%3")
                .arg(QString::fromUtf8(file))
                .arg(QString::fromUtf8(to))
                .arg(QString::fromUtf8(errtxt)));

    if (exitcode == EXIT_SUCCESS) {
        gchar **lines = g_strsplit(outtxt, "\n", 0);
        gchar **it;
        for (it = lines; *it; ++it) {
            gchar *l = *it;
            gboolean found = FALSE;

            while (*l && !found) {
                if (!strcmp(l, "/openbox-3/")) {
                    *l = '\0'; /* cut the string */
                    found = TRUE;
                }
                ++l;
            }

            if (found) {
                name = g_strdup(*it);
                break;
            }
        }
        g_strfreev(lines);
    }

    g_free(outtxt);
    g_free(errtxt);
    return name;
}
