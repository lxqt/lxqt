set(PLUGIN "dom")

set(HEADERS
    domplugin.h
    treewindow.h
    domtreeitem.h
)

set(SOURCES
    domplugin.cpp
    treewindow.cpp
    domtreeitem.cpp
)

set(UIS
    treewindow.ui
)

set(RESOURCES
    resources.qrc
)

BUILD_LXQT_PLUGIN(${PLUGIN})
