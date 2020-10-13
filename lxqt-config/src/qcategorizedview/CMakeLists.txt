project(qcategorizedview)

set(SOURCES
    qcategorizedview.cpp
    qcategorydrawer.cpp
    qcategorizedsortfilterproxymodel.cpp
)

add_library(qcategorizedview STATIC ${SOURCES})
target_link_libraries(qcategorizedview Qt5::Widgets)
