# Automatic LXDE-Qt builds with JHBuild

This repository includes a [JHBuild](https://wiki.gnome.org/Jhbuild) configuration file and module set for building, installing and running every part of the LXDE-Qt desktop environment.

JHBuild can be used for many different tasks. For example, the entire desktop environment can be built completely automatically, or only specific modules can be built. JHBuild automatically handles module dependencies and build orders, installation into a prefix, running with the correct environment variables, and more.

## Configuration

This repository includes a default configuration for building the entire desktop environment. All sources will be cloned into a `src` subdirectory. Builds will happen out of tree in a `build` subdirectory (with some exceptions). All modules will be installed in `install`.

To modify the modules being built or the directories used, modify the `jhbuildrc` file. It is fairly self-explanatory.

## Usage

Install JHBuild by following the [instructions](https://developer.gnome.org/jhbuild/unstable/getting-started.html.en#getting-started-install). Stop at "Configuring JHBuild".

Clone this repository:

    $ git clone https://github.com/lxde/jhbuild.git
    $ cd jhbuild

Build the entire DE:

    $ jhbuild -f jhbuildrc

Build a specific module (and dependencies):

    $ jhbuild -f jhbuildrc build pcmanfm-qt

Run a module inside the installation:

    $ jhbuild -f jhbuildrc run install/bin/pcmanfm-qt

(Using the `run` command will automatically set the correct library directories, etc.)

You can also move the `jhbuildrc` configuration file to `~/.config/jhbuildrc`. This way the `-f` parameter is not required. However, you should modify the source, build and installation paths in the configuration before doing so.

## Other commands

See `jhbuild help` for more commands.
