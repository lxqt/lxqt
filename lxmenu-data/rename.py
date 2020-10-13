#!/usr/bin/env python
import os

f = open("layout/applications.menu", "r")
app_menu = f.read()
f.close()

f = open("layout/settings.menu", "r")
settings_menu = f.read()
f.close()

d = os.listdir("desktop-directories")
for l in d:
    if not l.endswith(".in"):
        continue
    r = ""
    n = len(l)

    for i in range(0, n):
        c = l[i]
        if l[i].isupper():
            if i > 0 and l[i-1].islower():
                r += "-"
            r += c.lower()
        else:
            r += c
    if not r.startswith("lxde-"):
        r = "lxde-%s" % r
    if "x-gnome" in r:
        r = "lxde%s" % r[12:]
    print(r)

    old_name = ">%s<" % l[:-3]
    new_name = ">%s<" % r[:-3]

    if old_name in app_menu:
        app_menu = app_menu.replace(old_name, new_name)
    if old_name in settings_menu:
        settings_menu = settings_menu.replace(old_name, new_name)

    if not l[0].isupper():
        continue
    os.rename("desktop-directories/%s" % l, "desktop-directories/%s" % r)

f = open("layout/applications.menu", "w")
f.write(app_menu)
f.close()

f = open("layout/settings.menu", "w")
f.write(settings_menu)
f.close()
