LXQt
====

LXQt is the next generation of LXDE, the Lightweight Desktop Environment. It is the product of the
merge between Razor-qt and LXDE-Qt.


## About this repository
This is a superproject which contains all lxde-qt components.
After checking out this repo, please do the following to initialize git submodules.
Please note, this repo works best with git version >= 1.8.2.
With older versions, you have to do many manual operations. Please use the latest git if you can.

> git submodule init
> git submodule update --remote --rebase

Note:
1. The command line option --remote requires git >= 1.8.2. If you're using git version >= 1.8.2, then it's done.
2. If you're unfortunately using an older version of git, omit the "--remote" part and please read the following notes.
3. Adding the --rebase option is suggested. Without --rebase, the submodules will all be detached commits and not in "master" branches.

====================================================================================

### Notes for git with version < 1.8.2:

There are some limitations of older versions of git.
Because git submodule does not track the latest HEAD automatically, the submodules always stick to specific commits
unless manually changed. Besides, after initial checkout, the submodule repos are detached from master branch.

Consider doing this manually to checkout master branches for every module.
> git submodule foreach git checkout master

To pull the latest changes for all modules at once, try this:
> git submodule foreach git pull --rebase

These restrictions no long applies to the latest git 1.8.x since it can be configured to track the latest changes in
submodules automatically.


Contributing
============


Code contributions
------------------

LXDE uses Git as a version control system. All our code is available online:
  http://git.lxde.org/

Aspiring contributors may choose to use our Github mirrors instead.
Pull Requests directly from Github are accepted.
  https://github.com/LXDE

For more information on git, see the official website:
  http://git-scm.com/

Please make sure you adhere to the project's styleguide when proposing changes.


Bug reports
-----------

All our issues are tracked on Github.
Please file all general LXQt bug reports on the general LXDE-Qt bug tracker:
  https://github.com/lxde/lxde-qt.

Some components, such as QtXDG, have their own issue tracker on Github:
  https://github.com/lxde/libqtxdg


Translations
------------

We use Pootle for all our translations.
  http://pootle.lxde.org/

For more information on Pootle, please see the official documentation:
  http://docs.translatehouse.org/projects/pootle/


Other
-----

For anything else, you can always drop by our mailing list and say hello!
  https://lists.sourceforge.net/lists/listinfo/lxde-list
